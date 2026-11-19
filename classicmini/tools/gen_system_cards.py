import argparse
import os

from PIL import Image, ImageDraw, ImageFont

FONT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "resources", "opensans_hebrew_condensed_regular.ttf")
HEART = [
    "..XX...XX..",
    ".XXXX.XXXX.",
    "XXXXXXXXXXX",
    "XXXXXXXXXXX",
    "XXXXXXXXXXX",
    ".XXXXXXXXX.",
    "..XXXXXXX..",
    "...XXXXX...",
    "....XXX....",
    ".....X.....",
]

CARDS = {
    "nes": "Nintendo NES 1",
    "snes": "Nintendo SNES 1",
    "famicom": "Nintendo Famicom",
    "sfc": "Nintendo Super Famicom 1",
    "fds": "Nintendo Famicom Disk System",
    "gb": "Nintendo Game Boy",
    "gbc": "Nintendo GBC 1",
    "gba": "Nintendo Game Boy Advance 1",
    "n64": "Nintendo N64 1",
    "gc": "Nintendo Game Cube 1",
    "virtualboy": "Nintendo Virtual Boy",
    "gameandwatch": "Nintendo Game & Watch",
    "sgb": "Nintendo Super Game Boy",
    "megadrive": "Sega Mega Drive 1",
    "genesis": "Sega Genesis 1",
    "sega32x": "Sega 32x",
    "segacd": "Sega CD",
    "megacd": "Sega CD",
    "mastersystem": "Sega Master System 1",
    "gamegear": "Sega Game Gear",
    "sg-1000": "Sega SG 1000",
    "saturn": "Sega Saturn 1",
    "pcengine": "NEC PC Engine",
    "pcenginecd": "NEC PC Engine CD",
    "tg16": "NEC TG16",
    "tg-cd": "NEC TG CD",
    "supergrafx": "NEC Super Grafx",
    "neogeo": "NEO GEO",
    "neogeocd": "NEO GEO CD",
    "ngp": "NEO GEO POCKET",
    "ngpc": "NEO GEO POCKET COLOR 1",
    "psx": "Sony PSX 1",
    "psp": "Sony PSP 1000a",
    "atari2600": "Atari 2600 1",
    "atari5200": "Atari 5200",
    "atari7800": "Atari 7800",
    "atari800": "Atari 800",
    "atarilynx": "Atari Lynx",
    "atarijaguar": "Atari Jaguar",
    "atarist": "Atari ST",
    "amiga": "Commodore Amiga",
    "c64": "Commodore 64",
    "msx": "Philips MSX",
    "msx1": "MSX1 Sony",
    "msx2": "MSX2 Sony",
    "amstradcpc": "Amstrad CPC",
    "apple2": "Apple II",
    "colecovision": "Colecovision",
    "intellivision": "Intellivision I",
    "vectrex": "Vectrex",
    "odyssey2": "Odyssey 2",
    "wonderswan": "Wonder Swan 1",
    "wonderswancolor": "Wonder Swan Color 1",
    "arcade": "Arcade",
    "cps1": "Capcom CPS1",
    "cps2": "Capcom CPS2",
    "cps3": "Capcom CPS3",
    "dos": "DOS",
    "zxspectrum": "Sinclair ZX Spectrum",
    "ti99": "Texas Instruments",
}

SPRITES = {
    "3do": ("3DO", "3DO Panasonic_Z"),
    "dreamcast": ("DREAMCAST", "Sega Dreamcast_Z"),
    "nds": ("NINTENDO DS", "Nintendo DS_Z"),
    "pcfx": ("PC-FX", "NEC PC-FX_Z"),
    "pc98": ("PC-98", "NEC PC-9801VX_Z"),
    "x68000": ("X68000", "Sharp X68000_Z"),
    "scummvm": ("SCUMMVM", "ScummVM Maniac_Z"),
}

BEZELS = {
    "auto-allgames": "pacman",
    "auto-lastplayed": "time pilot SD",
    "auto-random": "tetris SD",
}


def frame(size, title):
    image = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    points = int(size * 0.2)
    font = ImageFont.truetype(FONT, points)
    while draw.textlength(title, font=font) > size * 0.9:
        points -= 1
        font = ImageFont.truetype(FONT, points)
    width = draw.textlength(title, font=font)
    draw.text(((size - width) / 2, size * 0.02), title, font=font, fill=(74, 77, 78, 255))
    top = int(size * 0.28)
    draw.rounded_rectangle((size * 0.06, top, size * 0.94, size * 0.97), radius=size * 0.1, fill=(17, 17, 17, 255))
    draw.rounded_rectangle((size * 0.09, top + size * 0.03, size * 0.91, size * 0.94), radius=size * 0.08, fill=(43, 43, 58, 255))
    return image, draw, top


def favorites(size):
    image, draw, top = frame(size, "FAVORITES")
    cell = size * 0.052
    left = (size - cell * len(HEART[0])) / 2
    above = top + (size * 0.97 - top - cell * len(HEART)) / 2
    for y, row in enumerate(HEART):
        for x, mark in enumerate(row):
            if mark == "X":
                colour = (255, 120, 140, 255) if (x, y) in ((2, 1), (3, 1), (2, 2)) else (232, 33, 58, 255)
                draw.rectangle((left + x * cell, above + y * cell, left + (x + 1) * cell - 1, above + (y + 1) * cell - 1), fill=colour)
    return image


def other(size):
    image, draw, top = frame(size, "OTHER")
    centre_x, centre_y = size / 2, top + (size * 0.97 - top) / 2
    draw.rounded_rectangle((centre_x - size * 0.34, centre_y - size * 0.17, centre_x + size * 0.34, centre_y + size * 0.17), radius=size * 0.09, fill=(200, 200, 205, 255))
    arm = size * 0.05
    draw.rectangle((centre_x - size * 0.24 - arm, centre_y - arm * 2.4, centre_x - size * 0.24 + arm, centre_y + arm * 2.4), fill=(43, 43, 58, 255))
    draw.rectangle((centre_x - size * 0.24 - arm * 2.4, centre_y - arm, centre_x - size * 0.24 + arm * 2.4, centre_y + arm), fill=(43, 43, 58, 255))
    for dx, colour in ((0.16, (232, 33, 58, 255)), (0.27, (232, 33, 58, 255))):
        draw.ellipse((centre_x + size * dx - arm, centre_y - arm * 0.2, centre_x + size * dx + arm, centre_y + arm * 1.8), fill=colour)
    return image


def sprite(size, title, path):
    image, _, top = frame(size, title)
    art = Image.open(path).convert("RGBA")
    box_width, box_height = size * 0.76, (size * 0.94 - top) - size * 0.06
    scale = min(box_width / art.width, box_height / art.height)
    art = art.resize((max(1, round(art.width * scale)), max(1, round(art.height * scale))), Image.LANCZOS)
    middle = top + size * 0.03 + (size * 0.91 - top - size * 0.03) / 2
    image.alpha_composite(art, (round((size - art.width) / 2), round(middle - art.height / 2)))
    return image


def bezel(size, path):
    image = Image.open(path).convert("RGBA").resize((size, size), Image.LANCZOS)
    radius = size * 0.1
    mask = Image.new("L", (size * 4, size * 4), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, size * 4 - 1, size * 4 - 1), radius=radius * 4, fill=255)
    image.putalpha(mask.resize((size, size), Image.LANCZOS))
    outline = Image.new("RGBA", (size * 4, size * 4), (0, 0, 0, 0))
    ImageDraw.Draw(outline).rounded_rectangle((0, 0, size * 4 - 1, size * 4 - 1), radius=radius * 4, outline=(17, 17, 17, 255), width=max(8, size // 15))
    image.alpha_composite(outline.resize((size, size), Image.LANCZOS))
    return image


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source")
    parser.add_argument("out")
    parser.add_argument("--sprites")
    parser.add_argument("--bezels")
    parser.add_argument("--size", type=int, default=204)
    args = parser.parse_args()

    os.makedirs(args.out, exist_ok=True)
    missing = []
    written = 0
    if args.sprites:
        for theme, (title, name) in sorted(SPRITES.items()):
            path = os.path.join(args.sprites, name + ".png")
            if not os.path.exists(path):
                missing.append(theme)
                continue
            sprite(args.size, title, path).save(os.path.join(args.out, theme + ".png"), optimize=True)
            written += 1
    if args.bezels:
        for theme, name in sorted(BEZELS.items()):
            path = os.path.join(args.bezels, name + ".png")
            if not os.path.exists(path):
                missing.append(theme)
                continue
            bezel(args.size, path).save(os.path.join(args.out, theme + ".png"), optimize=True)
            written += 1
    for theme, name in sorted(CARDS.items()):
        path = os.path.join(args.source, "__" + name + " LOGO HD.png")
        if not os.path.exists(path):
            missing.append(theme)
            continue
        image = Image.open(path).convert("RGBA")
        image = image.resize((args.size, args.size), Image.LANCZOS)
        image.save(os.path.join(args.out, theme + ".png"), optimize=True)
        written += 1
    favorites(args.size).save(os.path.join(args.out, "auto-favorites.png"), optimize=True)
    other(args.size).save(os.path.join(args.out, "other.png"), optimize=True)
    print("written", written + 2, "missing", missing)


if __name__ == "__main__":
    main()
