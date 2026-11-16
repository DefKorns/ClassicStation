# Following RetroPie/EmulationStation

ClassicStation is a pruned fork of [RetroPie/EmulationStation](https://github.com/RetroPie/EmulationStation) for the
NES and SNES Classic. Branch `classicstation`; the remote `upstream` is the RetroPie repository and I follow its
`stable` branch.

Last integrated: `stable` at `a31c36a` (v2.12.0).

## Updating

```sh
sh classicmini/tools/sync_upstream.sh
git merge upstream/stable
```

The script fetches, lists what upstream changed in files I still have and in files I deleted, and does not merge by
itself. `git rerere` is enabled in this clone so a conflict resolved once is resolved again automatically.

## Rules

- Merge, never rebase: the branch is published.
- A file I deleted that upstream changed: keep it deleted (`git rm <file>`).
- A line I removed from a kept file that upstream changed: resolve by hand and keep only what the minis need.
- ES source files and classes keep their upstream names. Only my own things carry the ClassicStation name: binary
  folder, hmod, scripts and docs. The binary is still `emulationstation`, and the on-console paths and the hmod id
  (`emulationstation_classicmini`) are unchanged so existing installs upgrade in place.
- Each removed feature is its own commit, so it can be reverted or re-checked against upstream.

## Removed from upstream

Nothing yet.
