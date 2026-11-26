import argparse
import math
import os

from PIL import Image, ImageDraw, ImageFilter, ImageFont

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


PIXEL_FONT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "device", "themes", "classicstation", "assets", "fonts", "PressStart2P-Regular.ttf")
SCALE = 4


def smooth_mask(size, box, radius):
    mask = Image.new("L", (size * SCALE, size * SCALE), 0)
    ImageDraw.Draw(mask).rounded_rectangle(tuple(v * SCALE for v in box), radius=radius * SCALE, fill=255)
    return mask.resize((size, size), Image.LANCZOS)


def vertical_gradient(size, top, bottom):
    gradient = Image.new("RGBA", (size, size))
    draw = ImageDraw.Draw(gradient)
    for y in range(size):
        t = y / (size - 1)
        draw.line((0, y, size, y), fill=tuple(round(a + (b - a) * t) for a, b in zip(top, bottom)) + (255,))
    return gradient


def wordmark(size, text, points, face, highlight, edge):
    font = ImageFont.truetype(PIXEL_FONT, points)
    probe = Image.new("L", (size, size), 0)
    width = ImageDraw.Draw(probe).textlength(text, font=font)
    mask = Image.new("L", (size, size), 0)
    left = round((size - width) / 2)
    ImageDraw.Draw(mask).text((left, 12), text, font=font, fill=255)
    mask = mask.point(lambda v: 255 if v > 127 else 0)
    depth = max(2, points // 6)
    extrusion = Image.new("L", (size, size), 0)
    for step in range(1, depth + 1):
        extrusion.paste(255, (0, step), mask)
    extrusion = Image.composite(Image.new("L", (size, size), 255), extrusion, mask)
    outline = Image.new("L", (size, size), 0)
    for dx in (-2, -1, 0, 1, 2):
        for dy in (-2, -1, 0, 1, 2):
            if abs(dx) + abs(dy) <= 3:
                outline.paste(255, (dx, dy), extrusion)
    image = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    image.paste(Image.new("RGBA", (size, size), (17, 17, 17, 255)), (0, 0), outline)
    image.paste(Image.new("RGBA", (size, size), edge + (255,)), (0, 0), extrusion)
    bbox = mask.getbbox()
    face_layer = Image.new("RGBA", (size, size), face + (255,))
    ImageDraw.Draw(face_layer).rectangle((0, bbox[1], size, bbox[1] + (bbox[3] - bbox[1]) // 3), fill=highlight + (255,))
    image.paste(face_layer, (0, 0), mask)
    return image


def panel(size, top, bottom):
    image = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    image.paste(Image.new("RGBA", (size, size), (17, 17, 17, 255)), (0, 0), smooth_mask(size, (1, 52, size - 2, size - 6), 22))
    face = (5, 56, size - 6, size - 10)
    image.paste(vertical_gradient(size, top, bottom), (0, 0), smooth_mask(size, face, 18))
    return image, face


def pixel_sprite(image, rows, palette, cell, origin, shadow=None):
    draw = ImageDraw.Draw(image)
    for pass_shadow in (True, False):
        if pass_shadow and not shadow:
            continue
        offset = (cell // 2, cell // 2) if pass_shadow else (0, 0)
        for y, row in enumerate(rows):
            for x, mark in enumerate(row):
                if mark == ".":
                    continue
                colour = shadow if pass_shadow else palette[mark]
                left = origin[0] + x * cell + offset[0]
                top = origin[1] + y * cell + offset[1]
                draw.rectangle((left, top, left + cell - 1, top + cell - 1), fill=colour)


def sparkle(image, face, points, colour):
    draw = ImageDraw.Draw(image)
    for fx, fy, arm in points:
        x = round(face[0] + (face[2] - face[0]) * fx)
        y = round(face[1] + (face[3] - face[1]) * fy)
        draw.rectangle((x - arm, y, x + arm, y), fill=colour)
        draw.rectangle((x, y - arm, x, y + arm), fill=colour)


HEART_ART = [
    ".OOO...OOO.",
    "OHHHO.OHRRO",
    "OHHRRORRRRO",
    "ORRRRRRRRRO",
    "ORRRRRRRRRO",
    ".ORRRRRRRO.",
    "..ORRRRRO..",
    "...ORRRO...",
    "....ORO....",
    ".....O.....",
]

PAD_ART = [
    "OOOOOOOOOOOOOOOOOOOOOOO",
    "OGGGGGGGGGGGGGGGGGGGGGO",
    "OGGGGGGGGGGGGGGGGGGGGGO",
    "OGGGDDGGGGGGGGGGGGGGGGO",
    "OGGGDDGGGGGGGGGGGRRGRRO",
    "OGDDDDDDGGGGGGGGGRRGRRO",
    "OGDDDDDDGGGGGGGGGGGGGGO",
    "OGGGDDGGGGGGGGGGGGGGGGO",
    "OGGGDDGGGGSSGGSSGGGGGGO",
    "OGGGGGGGGGGGGGGGGGGGGGO",
    "OOOOOOOOOOOOOOOOOOOOOOO",
]


CLOCK_ART = [
    "....OOOOO....",
    "..OOWWWWWOO..",
    ".OWWWWBWWWWO.",
    ".OWWWWBWWWWO.",
    "OWWWWWBWWWWWO",
    "OWWWWWBWWWWWO",
    "OWWWBBBWWWWWO",
    "OWWWWWWWWWWWO",
    "OWWWWWWWWWWWO",
    ".OWWWWWWWWWO.",
    ".OWWWWWWWWWO.",
    "..OOWWWWWOO..",
    "....OOOOO....",
]


def title_mark(size, text, points, colours):
    if " " not in text:
        return wordmark(size, text, points, *colours)
    layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    for index, word in enumerate(text.split(" ")):
        line = wordmark(size, word, points, *colours)
        layer.alpha_composite(line, (0, index * (points + 2) - 2))
    return layer


def pixel_card(size, title, points, art, palette, cell, hue, colours, sparks):
    image, face = panel(size, *hue)
    sparkle(image, face, sparks, (255, 240, 250, 255))
    origin = (round((size - cell * len(art[0])) / 2), round(face[1] + (face[3] - face[1] - cell * len(art)) / 2))
    pixel_sprite(image, art, palette, cell, origin, shadow=(10, 6, 30, 190))
    image.alpha_composite(title_mark(size, title, points, colours))
    return image


def favorites(size):
    return pixel_card(size, "FAVORITES", 16, HEART_ART, {"O": (70, 8, 30, 255), "R": (240, 40, 72, 255), "H": (255, 150, 170, 255)}, 9,
                      ((88, 40, 128), (30, 14, 60)), ((255, 92, 120), (255, 158, 176), (150, 22, 62)),
                      [(0.12, 0.2, 3), (0.86, 0.3, 4), (0.2, 0.78, 2), (0.82, 0.82, 3)])


def other(size):
    return pixel_card(size, "OTHER", 24, PAD_ART, {"O": (17, 17, 17, 255), "G": (206, 206, 212, 255), "D": (60, 60, 82, 255), "R": (232, 33, 58, 255), "S": (112, 112, 134, 255)}, 7,
                      ((28, 112, 128), (10, 40, 64)), ((255, 206, 72), (255, 236, 150), (170, 104, 12)),
                      [(0.1, 0.22, 2), (0.9, 0.2, 3), (0.14, 0.82, 3), (0.88, 0.8, 2)])


def last_played(size):
    return pixel_card(size, "LAST PLAYED", 14, CLOCK_ART, {"O": (10, 20, 60, 255), "W": (240, 244, 255, 255), "B": (232, 33, 58, 255)}, 8,
                      ((52, 70, 170), (14, 20, 80)), ((120, 200, 255), (200, 236, 255), (30, 90, 170)),
                      [(0.12, 0.2, 3), (0.86, 0.3, 4), (0.2, 0.78, 2), (0.82, 0.82, 3)])


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


ORIGINALS = {
    "nes": ("nes", 30, "CLASSIC EDITION", (214, 176, 80)),
    "hvc": ("famicom", 30, "CLASSIC MINI", (255, 120, 120)),
    "snes_eur": ("snes", 21, "CLASSIC MINI", (190, 190, 200)),
    "snes_usa": ("snes", 21, "CLASSIC MINI", (190, 190, 200)),
    "snes_jpn": ("sfc", 21, "CLASSIC MINI", (190, 190, 200)),
}
SHONEN_GOLD = (179, 163, 134)


def card_logo(base):
    top = base.crop((0, 0, base.width, 50))
    box = top.getbbox()
    cleared = base.copy()
    cleared.paste(Image.new("RGBA", top.size, (0, 0, 0, 0)), (0, 0))
    return cleared, top.crop(box)


def caption(size, text, colour, y):
    mask = Image.new("L", (size, size), 0)
    font = ImageFont.truetype(PIXEL_FONT, 8)
    draw = ImageDraw.Draw(mask)
    draw.text(((size - draw.textlength(text, font=font)) / 2, y), text, font=font, fill=255)
    mask = mask.point(lambda v: 255 if v > 127 else 0)
    image = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    image.paste(Image.new("RGBA", (size, size), (17, 17, 17, 255)), (0, 0), mask.filter(ImageFilter.MaxFilter(3)))
    image.paste(Image.new("RGBA", (size, size), colour + (255,)), (0, 0), mask)
    return image


def games_burst(size, count):
    factor = 4
    layer = Image.new("RGBA", (size * factor, size * factor), (0, 0, 0, 0))
    draw = ImageDraw.Draw(layer)
    cx, cy = 160 * factor, 162 * factor

    def star(outer, inner):
        return [(cx + (outer if i % 2 == 0 else inner) * math.cos(0.1 + i * math.pi / 14), cy + (outer if i % 2 == 0 else inner) * math.sin(0.1 + i * math.pi / 14)) for i in range(28)]

    draw.polygon(star(41 * factor, 31 * factor), fill=(17, 17, 17, 255))
    draw.polygon(star(38 * factor, 29 * factor), fill=(214, 33, 33, 255))
    draw.polygon(star(35 * factor, 27 * factor), fill=(255, 228, 40, 255))
    layer = layer.resize((size, size), Image.LANCZOS)
    draw = ImageDraw.Draw(layer)
    big = ImageFont.truetype(PIXEL_FONT, 16)
    small = ImageFont.truetype(PIXEL_FONT, 8)
    draw.text((160 - draw.textlength(str(count), font=big) / 2, 148), str(count), font=big, fill=(17, 17, 17, 255))
    draw.text((160 - draw.textlength("GAMES", font=small) / 2, 168), "GAMES", font=small, fill=(17, 17, 17, 255))
    return layer


def originals_card(size, base_path, count, subtitle, colour):
    base, logo = card_logo(Image.open(base_path).convert("RGBA"))
    scale = min(176 / logo.width, 28 / logo.height)
    logo = logo.resize((max(1, round(logo.width * scale)), max(1, round(logo.height * scale))), Image.LANCZOS)
    base.alpha_composite(logo, ((size - logo.width) // 2, 3 + (28 - logo.height) // 2))
    base.alpha_composite(caption(size, subtitle, colour, 38))
    base.alpha_composite(games_burst(size, count))
    return base


def shonen_card(size, base_path, logo_path, count):
    base, famicom_logo = card_logo(Image.open(base_path).convert("RGBA"))
    pixels = base.load()
    for y in range(52, size - 4):
        for x in range(size):
            r, g, b, a = pixels[x, y]
            if a and r - b < 90:
                pixels[x, y] = (r * SHONEN_GOLD[0] // 255, g * SHONEN_GOLD[1] // 255, b * SHONEN_GOLD[2] // 255, a)
    jump = Image.open(logo_path).convert("RGBA")
    jump = jump.crop(jump.split()[3].point(lambda v: 255 if v > 8 else 0).getbbox())
    jump = jump.resize((round(jump.width * 48 / jump.height), 48), Image.LANCZOS)
    width = size - 4 - jump.width - 6 - 2
    famicom_logo = famicom_logo.resize((width, max(1, round(famicom_logo.height * width / famicom_logo.width))), Image.LANCZOS)
    left = (size - (jump.width + 6 + famicom_logo.width)) // 2
    base.alpha_composite(jump, (left, 2))
    base.alpha_composite(famicom_logo, (left + jump.width + 6, 2 + (48 - famicom_logo.height) // 2))
    base.alpha_composite(games_burst(size, count))
    return base


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source")
    parser.add_argument("out")
    parser.add_argument("--sprites")
    parser.add_argument("--bezels")
    parser.add_argument("--shonen-logo")
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
    last_played(args.size).save(os.path.join(args.out, "auto-lastplayed.png"), optimize=True)
    for theme, (base, count, subtitle, colour) in ORIGINALS.items():
        path = os.path.join(args.out, base + ".png")
        if os.path.exists(path):
            originals_card(args.size, path, count, subtitle, colour).save(os.path.join(args.out, "originals_" + theme + ".png"), optimize=True)
            written += 1
    if args.shonen_logo and os.path.exists(os.path.join(args.out, "famicom.png")):
        shonen_card(args.size, os.path.join(args.out, "famicom.png"), args.shonen_logo, 20).save(os.path.join(args.out, "originals_hvcj.png"), optimize=True)
        written += 1
    print("written", written + 3, "missing", missing)


if __name__ == "__main__":
    main()
