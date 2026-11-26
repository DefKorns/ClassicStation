import argparse
import os

from PIL import Image

TEXT_TOP = 190


def clean(source, target):
    image = Image.open(source).convert("RGB")
    pixels = image.load()
    for y in range(TEXT_TOP, image.height):
        colour = pixels[2, y]
        for x in range(image.width):
            pixels[x, y] = colour
    os.makedirs(os.path.dirname(target), exist_ok=True)
    image.convert("P", palette=Image.ADAPTIVE, colors=64).save(target, optimize=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source")
    parser.add_argument("out")
    parser.add_argument("--systems", nargs="+")
    args = parser.parse_args()

    written = 0
    for name in sorted(args.systems or os.listdir(args.source)):
        path = os.path.join(args.source, name, "launching.png")
        if os.path.exists(path):
            clean(path, os.path.join(args.out, name, "launching.png"))
            written += 1
    print("written", written)


if __name__ == "__main__":
    main()
