import argparse
import json
import sys

WIDTH = 1280.0
HEIGHT = 720.0
IDENTITY = [1, 0, 0, 0, 1, 0]


def multiply(a, b):
    return [
        a[0] * b[0] + a[1] * b[3], a[0] * b[1] + a[1] * b[4], a[0] * b[2] + a[1] * b[5] + a[2],
        a[3] * b[0] + a[4] * b[3], a[3] * b[1] + a[4] * b[4], a[3] * b[2] + a[4] * b[5] + a[5],
    ]


def local(node):
    transform = node.get("transform", IDENTITY)
    return transform if len(transform) == 6 else IDENTITY


def find(node, names, parent):
    if not names:
        return node, parent
    here = multiply(parent, local(node))
    for child in node.get("children", []):
        if child["name"] == names[0]:
            found = find(child, names[1:], here)
            if found:
                return found
    return None


def sprites_of(node):
    for component in node.get("components", []):
        sprite = component.get("spriteId", "")
        if sprite:
            yield sprite.split(".", 1)[-1], component.get("size")


def walk(node, parent, depth, limit, rows):
    transform = multiply(parent, local(node))
    sprite = next(sprites_of(node), None)
    rows.append((depth, node, transform, sprite))
    if depth < limit:
        for child in node.get("children", []):
            walk(child, transform, depth + 1, limit, rows)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("scene")
    parser.add_argument("path", help="node path below the root, for example homemenu/resume_floating")
    parser.add_argument("--depth", type=int, default=2)
    parser.add_argument("--atlas", help="atlas json, used for sprites without a size in the scene")
    parser.add_argument("--theme", action="store_true", help="print image elements in screen fractions")
    args = parser.parse_args()

    scene = json.load(open(args.scene, encoding="utf-8"))
    sizes = {}
    if args.atlas:
        frames = json.load(open(args.atlas, encoding="utf-8")).get("sprites", {})
        for name, frame in frames.items():
            sizes[name] = frame.get("sourceSize") or frame.get("textureFrame", [0, 0, 0, 0])[2:4]

    names = [part for part in args.path.split("/") if part]
    if names and names[0] == "Main":
        names = names[1:]
    found = find(scene["rootWorldNode"], names, IDENTITY)
    if not found:
        sys.exit("node not found")
    node, parent = found

    rows = []
    walk(node, parent, 0, args.depth, rows)
    for depth, current, current_transform, sprite in rows:
        x = WIDTH / 2 + current_transform[2]
        y = HEIGHT / 2 - current_transform[5]
        label = "  " * depth + current["name"]
        extra = ""
        if sprite:
            name, size = sprite
            if not size and name in sizes:
                size = sizes[name]
            extra = "  %s %s" % (name, "x".join(str(int(value)) for value in size) if size else "")
        enabled = "" if current.get("enabled", True) else "  (disabled)"
        print("%-44s centre %7.1f,%7.1f scale %.2f/%.2f%s%s" % (label, x, y, current_transform[0], current_transform[4], extra, enabled))
        if args.theme and sprite:
            name, size = sprite
            if not size and name in sizes:
                size = sizes[name]
            if size:
                width = size[0] * abs(current_transform[0])
                height = size[1] * abs(current_transform[4])
                print('<image name="%s"><path>./%s.png</path><pos>%.6f %.6f</pos><size>%.6f %.6f</size><origin>0.5 0.5</origin></image>' % (
                    current["name"], name, x / WIDTH, y / HEIGHT, width / WIDTH, height / HEIGHT))


if __name__ == "__main__":
    main()
