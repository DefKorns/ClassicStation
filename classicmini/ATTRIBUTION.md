# Attribution

## Fonts

All licensed under the [SIL Open Font License 1.1](https://openfontlicense.org).

- Noto Sans, by The Noto Project Authors (<https://github.com/notofonts/latin-greek-cyrillic>):
  `device/resources/fallback_latin.ttf`, reduced to Latin Extended-A/B and Latin Extended Additional.
- Noto Sans JP, by Adobe and The Noto Project Authors (<https://github.com/notofonts/noto-cjk>):
  `device/resources/fallback_ja.ttf`, weight 400 reduced to kana, punctuation and the JIS X 0208 kanji.
- Fredoka, by The Fredoka Project Authors (<https://github.com/hafontia/Fredoka-One>): the lettering of
  `device/favorites.svg` and `device/other.svg`.

## Third-party code and data

- minimp3 by lieff (<https://github.com/lieff/minimp3>), CC0 1.0: `es-core/src/minimp3/`.
- LodePNG by Lode Vandevenne (<https://github.com/lvandeve/lodepng>), zlib licence: git submodule in `vendor/lodepng`.
- Mozilla CA certificate bundle as extracted by the curl project (<https://curl.se/docs/caextract.html>),
  [MPL 2.0](https://www.mozilla.org/MPL/2.0/): downloaded by the hmod build as `device/resources/cacert.pem`.
- [es-theme-simple](https://github.com/RetroPie/es-theme-simple) by Nils Bonenberger,
  [CC BY-NC-SA](https://creativecommons.org/licenses/by-nc-sa/4.0/): downloaded by `hmod/stage.sh`.
- The single `awk` pass that resolves covers in `device/es-library.sh` comes from code shared by DNA64
  (<https://github.com/dna64>).

## Trademarks

"Super Nintendo Entertainment System" and the logo are trademarks of Nintendo. This project is not affiliated with
or endorsed by Nintendo.
