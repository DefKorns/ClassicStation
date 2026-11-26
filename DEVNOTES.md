# Developer notes

Notes for working on ClassicStation. The first part is about this port, which lives in `classicmini/` and in a few hooks
in `es-app` and `es-core`. The last part is upstream's own notes on EmulationStation's code.

## Layout of `classicmini/`

- `device/`: everything that runs on the console. `es-*.sh` are the scripts (`es-run.sh` starts ES, `es-game.sh`
  launches games, `es-theme.sh` generates the classicstation theme, `es-tools.sh` is what the menus call), `es-library`
  is the native scanner that builds the library (`classicmini/tools/es_library.cpp`), `S8100es` is the init script,
  `preinit/` the preinit hook, `resources/` and `themes/` what ships on the card.
- `hmod/`: the hakchi mod. `stage.sh` collects the files into `mod/`, `files/install` and `files/uninstall` run under
  hakchi, and `docker-make.sh` (from `vendor/hmod-build`) packs the `.hmod` into `hmod/out/`.
- `tools/`: C++ built for the console (`padstate`, `theme_layers`, `crtfx`) and the Python and shell scripts that
  generate the artwork and the theme.
- `tests/`: see below.
- `hakchi-app/`: the pack for the hakchi app.

## Building the hmod

```sh
classicmini/build.sh
sh classicmini/build-tools.sh
cd classicmini/hmod
sh stage.sh
sh ../../vendor/hmod-build/docker-make.sh
```

Only docker is needed on the host. Everything runs in `classicmini-es-toolchain`, built from `Dockerfile.es-armhf` in
[classicmini-cross-toolchain](https://github.com/DefKorns/classicmini-cross-toolchain) on top of the shared toolchain
image (GCC 4.9.4 / glibc 2.21 for Cortex-A7, the console's SDL2 2.0.4, FreeType, ALSA, static libcurl on mbedTLS). The
ES image adds CMake 3.27, static FreeImage, RapidJSON and link stubs for `libGLESv2`; nothing is taken from a console.
`ES_TOOLCHAIN_IMAGE` selects another image (the default is `ghcr.io/defkorns/classicmini-es-toolchain:v0.1.0`, pulled
if it is missing).

`classicmini/build.sh` runs `classicmini/scripts/build-es.sh` inside the container with the repository mounted: it
fetches the `pugixml` submodule, configures with `-DGLES=ON`, builds and strips. The build tree stays in
`classicmini/out/.build` so rebuilds are incremental. ES's CMake writes the `emulationstation` binary into the
repository root; it is ignored by git.

`classicmini/build-tools.sh` builds the other binaries: `padstate`, `theme_layers` and `libcrtfx.so`. `stage.sh` stops
if the ES binary is missing and takes the tools from `out/` too, so run both builds first.

In `classicmini/out/` you get `emulationstation` (stripped ARM hard-float ELF, about 8 MB), `libfreetype.so.6`,
`padstate`, `theme_layers` and `libcrtfx.so`. At run time it needs the console's `libSDL2.so`, `libasound.so.2` and
`libGLESv2.so`. `stage.sh` puts the binaries, scripts and resources into `mod/`, and the hmod lands in `hmod/out/`.
`stage.sh` also downloads the CA bundle and the `simple` theme (at a pinned revision) when they are not there yet.

## Tests

```sh
sh classicmini/tests/run.sh
```

It runs the two theme checks (`theme_variables_test.sh` and `theme_properties_test.sh`) on the host, then builds and
runs the C++ tests inside the toolchain container: suspend points, the theme layers and the metadata overlay. Docker has
to be running.

Before each commit, `pre-commit` runs ShellCheck, markdownlint and the whitespace and line ending checks on
`classicmini/`, the README and the workflows. Set it up once with `pre-commit install`. The ShellCheck rules for the
console's ash are in `.shellcheckrc`.

## Trying changes on the console

Copy the new files over the live ones in `/var/lib/hakchi/emulationstation` and restart ES with `es-run.sh`. First
check which console it is: after `source /etc/preinit`, `$sftype` and `$sfregion` tell you the console (`nes` or `snes`, and the region). A NES and a SNES look the
same over ssh, but the files are not interchangeable. Keep a backup of what you replace.

- `ES_INJECT` is read by the `emulationstation` process and presses buttons for you, which is handy to put the UI in a
  known state for a screenshot. It is a comma list of `milliseconds:button`, for example
  `ES_INJECT=4000:start,5500:a`. The buttons are `a`, `b`, `x`, `y`, `select`, `start`, `left`, `right`, `up` and
  `down`. A `+` or `-` before the name holds or releases the button.
- The logs are `/var/lib/hakchi/emulationstation/home/.emulationstation/es_log.txt` and, after a crash,
  `/var/lib/hakchi/emulationstation/crash.log`. The video debug log is written to
  `/media/data/log/crtfx/journal.log` when it is switched on in Help, Developer.
- The Developer entry is hidden, like Android's developer options: open About and press Select seven times.
- The console is short on memory: do not run recursive `grep` or `find` over `/usr` on it.

## Shell scripts

The scripts run on the console's BusyBox ash, not bash.

- Use `source <path>`, never `. <path>`, and keep the paths relative to `$mountpoint` where the hakchi scripts do.
  `script_init` and `$rootfs` only exist after `/etc/preinit` has been sourced.
- At run time write through `/etc/...`, never through `$rootfs/etc/...`: the overlay does not show a replaced file
  until the next reboot.
- Functions are camelCase, variables and new file names snake_case. A new name must not exist in hakchi's
  `b0010_functions`, because they share one namespace. Variables inside a function are `local`.
- Files use LF endings, also on Windows.
- A guard like `[ ! -f "$cache" ]` protects generating the cache, never using it.

## Releases

1. Write `release-notes/<tag>.md`, with the changes and the known issues, and commit it.
2. Make a signed annotated tag named `v<version>`, for example `v0.1.0-alpha.5`, and push the branch and the tag.
3. The `Build` workflow builds the hmod on a tag. It publishes a release named `ClassicStation <tag>` with the
   `.hmod`, using the release notes file if there is one, and marks it as a pre-release when the tag has a `-`.

During the alpha the releases are made from `classicstation`. The workflow also runs on pushes to `classicstation` and
`main` that touch `classicmini/`, `es-app/`, `es-core/` or `resources/`, and keeps the `.hmod` as a workflow artifact.

Before tagging a release that changes compiled code, build locally and check that the binary really changed (size, hash
or a string you added). A build with a missing prerequisite can reuse the old binary without saying so.

## Notes from upstream

The rest of this file comes from EmulationStation's own notes, written by its original author. "I" and "my code" below
mean him.

> These are mostly notes I create as I go along, marking potential gotchas for people who might try to extend my code.
> Some day I'll try and add an overview of the code structure, what each class does, etc.

### Creating a new GuiComponent

You probably want to override:

- `bool input(InputConfig* config, Input input);`: check if some input is mapped to some action with
  `config->isMappedTo("a", input);`, and if an input is "pressed" with `input.value != 0` (`input.value` *can* be
  negative in the case of axes).
- `void update(int deltaTime);`: `deltaTime` is in milliseconds.
- `void render(const Transform4x4f& parentTrans);`: you probably want to do
  `Transform4x4f trans = parentTrans * getTransform();` to get your final "modelview" matrix. Apply it with
  `Renderer::setMatrix(const Transform4x4f&)`, and render any children the component may have with
  `renderChildren(parentTrans);`.

### Creating a new GameListView class

1. Don't allow the user to navigate to the root node's parent. If you use a stack of some sort to keep track of past cursor states this will be a natural side effect.

### Creating a new Component

If your component is not made up of other components, and you draw something to the screen with OpenGL, make sure:

- Your vertex positions are rounded before you render (you can use round(float) in Util.h to do this).
- Your transform matrix's translation is rounded (you can use roundMatrix(affine3f) in Util.h to do this).
