# Upstream

ClassicStation is a fork of [RetroPie/EmulationStation](https://github.com/RetroPie/EmulationStation). I follow its
`stable` branch, last merged at `a31c36a` (v2.12.0). The license and credits are upstream's and stay as they are.

## Branches

- `stable`: a copy of `upstream/stable`. Only fast-forwarded, never committed to.
- `classicstation`: where I work. It gets upstream with `git merge stable`. During the alpha, the releases and their
  tags are made here too.
- `main`: the main branch. `classicstation` goes in once it has been tested, and after the alpha the tags are made here.

## Updating

```sh
sh classicmini/tools/sync_upstream.sh
git merge stable
```

The script updates `stable` and lists what upstream changed, split between files I still have and files I deleted. It
doesn't merge or push. `git rerere` is on, so a conflict I already resolved once is resolved the same way next time.

## Rules

- Merge, don't rebase. The branches are published.
- If upstream changed a file I deleted, I keep it deleted.
- If upstream changed a line I removed from a file I kept, I resolve it by hand.
- I don't rename upstream's files or classes. The binary is still `emulationstation`, and the on-console paths and the
  hmod id (`emulationstation_classicmini`) stay, so an existing install updates in place.
- Every removal is its own commit.

## Removed

- The GL 1.4, GL 2.1 and GLES 1.0 renderers, with their build options. The Mali only does GLES 2.0.
- Raspberry Pi, OMX and Vero4K: the code behind `_RPI_`, `_OMX_` and `_VERO4K_`, the OMX player and its menu entries,
  and the matching CMake options. CEC stays, since a driver for it could be added to the kernel. The OMX settings keys
  stay in `Settings.cpp` so an old `es_settings.cfg` loads without warnings.
- Windows: the `.rc`, `.aps` and `.ico` files, the MSVC blocks in CMake, and the manual `ccpp.yml` and `win32.yml`
  workflows.
- `MameNames`, the three `mame*.xml` tables and `mameres.py`, and the arcade BIOS filter that used them. hakchi already
  has the game names, and the hmod never shipped the tables.
- The CPack and Debian packaging, the `USE_MESA_GLES` option, RetroPie's `README.md` and its issue template.
- Kiosk UI mode, with `--force-kiosk`. Kid mode stays.

## Changed to build and run on the console

- libvlc removed: the video component is a stub that shows the snapshot image.
- `std::put_time` replaced by `strftime` (libstdc++ 4.9 has none).
- `SDL_JoystickGetVendor/Product` guarded (SDL 2.0.4 on the console has none).

The rest of this repository is EmulationStation itself; the port is what lives in `classicmini/` and the small hooks it
needs in `es-app` and `es-core`.
