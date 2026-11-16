# Following RetroPie/EmulationStation

ClassicStation is a fork of [RetroPie/EmulationStation](https://github.com/RetroPie/EmulationStation), pruned and
adapted for the NES and SNES Classic. EmulationStation is by Alec Lofquist and the RetroPie project, under the MIT
license: `LICENSE.md` and `CREDITS.md` stay as they are. The remote `upstream` is the RetroPie repository and I follow
its `stable` branch.

Last integrated: `stable` at `a31c36a` (v2.12.0).

## Branches

| Branch | Role |
| --- | --- |
| `stable` | Mirror of `upstream/stable`. Fast-forward only, no commits of mine. |
| `classicstation` | Where I work and remove what the minis do not need. Receives upstream with `git merge stable`. |
| `main` | The release branch. Gets `classicstation` merged when it is tested; tags come from here. |

```text
upstream/stable -> stable (ff) -> classicstation -> main -> tag
```

## Updating

```sh
sh classicmini/tools/sync_upstream.sh
git merge stable
```

The script fast-forwards `stable` to `upstream/stable` and lists what upstream changed in the files I still have and in
the files I deleted. It does not merge, and it does not push. `git rerere` is enabled in this clone so a conflict
resolved once is resolved again automatically. After the merge and a build, `classicstation` goes into `main`.

## Rules

- Merge, never rebase: the branches are published.
- A file I deleted that upstream changed: keep it deleted (`git rm <file>`).
- A line I removed from a kept file that upstream changed: resolve by hand and keep only what the minis need.
- ES source files and classes keep their upstream names. Only my own things carry the ClassicStation name: folder,
  hmod, scripts and docs. The binary is still `emulationstation`, and the on-console paths and the hmod id
  (`emulationstation_classicmini`) are unchanged so existing installs upgrade in place.
- Each removed feature is its own commit, so it can be reverted or re-checked against upstream.

## Removed from upstream

- Renderers: `Renderer_GL14`, `Renderer_GL21` and `Renderer_GLES10`, with the `GL`, `USE_GL21` and `USE_GLES1` build
  options and `FindOpenGLES.cmake`. The Mali on the minis runs GLES2, so the build only has `Renderer_GLES20`.
- Raspberry Pi, OMX and Vero4K: the `_RPI_`, `_OMX_` and `_VERO4K_` code, `VideoPlayerComponent` (the OMX player), the
  OMX menu entries, the Broadcom VideoCore parts of `CECInput` and the `RPI`/`OMX` build options with the
  Broadcom/Vero4K detection in `CMakeLists.txt`. CEC itself stays, because a driver for it could be added to the kernel.
  The settings keys stay in `Settings.cpp` so an old `es_settings.cfg` loads without warnings.
- Windows packaging and CI: `EmulationStation.rc`, `EmulationStation.aps`, `es_icon.ico`, the MSVC blocks of
  `es-app/CMakeLists.txt` and the manual `ccpp.yml` and `win32.yml` workflows. The build and release workflow is
  `build.yml`.
