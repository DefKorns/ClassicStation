import re
import sys

INFO = {
    "snes": ("Nintendo", "1991"),
    "nes": ("Nintendo", "1985"),
    "fds": ("Nintendo", "1986"),
    "gb": ("Nintendo", "1989"),
    "gbc": ("Nintendo", "1998"),
    "gba": ("Nintendo", "2001"),
    "nds": ("Nintendo", "2004"),
    "n64": ("Nintendo", "1996"),
    "virtualboy": ("Nintendo", "1995"),
    "pokemini": ("Nintendo", "2001"),
    "gameandwatch": ("Nintendo", "1980"),
    "megadrive": ("Sega", "1988"),
    "sega32x": ("Sega", "1994"),
    "segacd": ("Sega", "1991"),
    "mastersystem": ("Sega", "1985"),
    "gamegear": ("Sega", "1990"),
    "sg-1000": ("Sega", "1983"),
    "saturn": ("Sega", "1994"),
    "dreamcast": ("Sega", "1998"),
    "pcengine": ("NEC", "1987"),
    "pcfx": ("NEC", "1994"),
    "pc98": ("NEC", "1982"),
    "pc88": ("NEC", "1981"),
    "x68000": ("Sharp", "1987"),
    "x1": ("Sharp", "1982"),
    "neogeo": ("SNK", "1990"),
    "ngp": ("SNK", "1998"),
    "psx": ("Sony", "1994"),
    "psp": ("Sony", "2004"),
    "atari2600": ("Atari", "1977"),
    "atari5200": ("Atari", "1982"),
    "atari7800": ("Atari", "1986"),
    "atari800": ("Atari", "1979"),
    "atarilynx": ("Atari", "1989"),
    "atarijaguar": ("Atari", "1993"),
    "atarist": ("Atari", "1985"),
    "3do": ("Panasonic", "1993"),
    "amiga": ("Commodore", "1985"),
    "c64": ("Commodore", "1982"),
    "msx": ("Microsoft", "1983"),
    "amstradcpc": ("Amstrad", "1984"),
    "zxspectrum": ("Sinclair", "1982"),
    "colecovision": ("Coleco", "1982"),
    "intellivision": ("Mattel", "1979"),
    "channelf": ("Fairchild", "1976"),
    "videopac": ("Philips", "1978"),
    "vectrex": ("GCE", "1982"),
    "wonderswan": ("Bandai", "1999"),
    "pc": ("IBM", "1981"),
    "moto": ("Thomson", "1984"),
}


def main():
    path = sys.argv[1]
    text = open(path, newline="").read()

    def patch(match):
        block = match.group(0)
        theme = re.search(r"<theme>([^<]*)</theme>", block)
        if not theme or theme.group(1) not in INFO or "<manufacturer>" in block:
            return block
        maker, year = INFO[theme.group(1)]
        add = "    <manufacturer>%s</manufacturer>\n    <year>%s</year>\n" % (maker, year)
        return block.replace("  </system>", add + "  </system>")

    patched = re.sub(r"  <system>.*?  </system>", patch, text, flags=re.S)
    open(path, "w", newline="").write(patched)
    print(patched.count("<manufacturer>"), "systems with info")


if __name__ == "__main__":
    main()
