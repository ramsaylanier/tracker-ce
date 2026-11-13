# Song Row Mutes

Per-playlist-row sample and MIDI track mutes on the Song arrange grid.

| | |
| :--- | :--- |
| Sidecar | `songrowmutes.bin` (workspace + project) |
| UI | Song arrange blocks (no playlist column) |
| Build order | `MODULE_ORDER` 30 |

## What it does

Song Mode playlist rows normally share one global mute state. This module stores a mute mask for each playlist row so the same pattern can play with different tracks muted on different rows.

Muted blocks for a row render greyed on the arrange grid. When song playback enters a row, that row’s mute mask is applied through the Performance mute APIs (absolute replace — previous row mutes do not leak).

## How to use it on the device

1. Open **Song**.
2. Enable **Rec** (edit mode).
3. Focus the arrange grid (track blocks) and select one or more blocks.
4. Hold **Shift** — softkey **Mute Blocks** appears (where Undo normally is).
5. Press it to toggle mute for every selected cell on its playlist row(s).

Grey blocks are muted for that row only. Other rows keep their own masks.

## On disk

| Location | File |
| :--- | :--- |
| Workspace | `Workspace/songrowmutes.bin` |
| Project | `Projects/<path>/<name>/songrowmutes.bin` |

## Build

```bash
make select   # enable Song Row Mutes
```

Or include the folder path in `FEATURE_MODULE_DIRS` when building.
