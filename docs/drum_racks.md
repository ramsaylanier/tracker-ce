# Drum Racks

**Drum Rack** is a first-class instrument type (global indices **67–74**, up to **8** active kits). Pads do not own audio; they point at existing sample slots `01–48`. Put the same kit on several sample tracks, use the note to pick a pad, and the kit’s filter / overdrive / bit depth / delay–reverb sends process those tracks together as one bus.

This is separate from the stock **Beat Slice** kits under `/Instruments/Drum Kits/` on the SD card (one long sample with slice markers). See [Drum Rack vs Beat Slice](#8-drum-rack-vs-beat-slice) below.

## 1. What it is

A Drum Rack is **not** a Beat Slice kit from `/Instruments/Drum Kits/`. Those stock kits are one sliced sample.

A Drum Rack is its own instrument in the rack pool:

1. Load kicks/snares/hats into sample slots as usual.
2. Create a rack from **Sample Loader → Racks** (soft key **New Rack**).
3. Wire pads to sample slots in the Drum Rack Kit editor.
4. Assign the rack instrument (listed above samples in the Instr popup) to pattern tracks.

New projects start with **zero** active racks until you create one.

## 2. Create a rack (Sample Loader)

1. Open **Sample Loader**.
2. Soft key **Racks** (or page indicator) opens **Sample Loader - Racks 2/2**.
3. The list shows the **8** pool slots (`1.` … `8.`). Empty slots show only the number; active racks show their name.
4. Soft key **New Rack** activates the first free pool slot (or the selected empty slot) and opens the Drum Rack Kit editor.
5. Soft key **Open Editor** opens Pads for the selected active rack.
6. Soft key **Clear Rack** deactivates the selected rack and deletes its workspace sidecar.
7. Soft key **Rename** opens the on-screen keyboard to rename the selected active rack (saved to its `.rack` sidecar).

## 3. Edit pads and bus

1. Press **Params** (or soft key **Edit Pads**). You land on **Drum Rack Kit 1/2**.
2. The main area shows a **4×4 grid** of pads (Start Note at **bottom-left**, rising toward top-right). Each pad shows the MIDI note that triggers it and the sample slot below (`---` if empty); the selected pad is highlighted.
3. Use the **arrow keys** (or tap a hardware pad) to select which of the 16 pads you are editing.
4. Encoder / +/- assigns a loaded sample to the selected pad (and previews it). Soft key **Start Note** (then encoder) sets the MIDI note for pad 1; pads 2–16 follow chromatically.
5. Soft key **Pad** (or **Enter**) opens pad settings for the selected pad.
6. On pad settings, set **Sample**, **Choke**, **Volume**, **Panning**, and per-pad **Delay** / **Reverb** sends. Soft-key values edit with the encoder; arrows move soft-key focus.
7. Soft key **Env** opens the pad envelope page (**Attack** / **Decay** / **Sustain** / **Release** plus **Tune** / **Finetune**). Soft key **Back** returns to pad settings.
8. Soft key **Back** (or **Enter**) on pad settings returns to the kit overview.
9. Press **Params** for **Drum Rack Bus 2/2** (shared filter, drive, bit depth, and kit-level reverb/delay sends).
10. Press Params once more to return to the kit overview.

Normal sample Instrument Parameters / Automation screens are **not** used for Drum Racks. Sample Playback no longer offers a **Drum Rack** play mode — racks are not sample instruments.

## 4. Sequence a kit

1. Open the Instr popup. After synths (when shown), **active racks** appear above samples `01–48`.
2. Select the rack and enter notes for pads. Pad 1 triggers on the kit’s **Start Note** (default matches Beat Slice’s first pad); each next pad is one semitone higher.
3. Assign that instrument on multiple sample tracks for polyphony within the kit (one voice per sample track).

## 5. Shared kit bus

1. Select the rack instrument, open Instrument Editor → **Drum Rack Bus** (Params from the kit overview).
2. Filter / overdrive / bit depth apply to every track currently playing that kit (and update live while pads are sounding).
3. Kit delay/reverb sends add to each pad’s own sends: effective send is `min(100, padSend + busSend)`. Pads default to 0 so bus-only kits keep working.

## 6. Capacity / voices

| Pool | Count | Role |
| | | |
| Sample voices | **8** (one per sample track) | One-shots, loops, slices, **Drum Rack pads** |
| Drum rack instruments | **8** | First-class kit instruments in project memory |
| Synth parts | **3** (when enabled) | Separate from the kit bus |

## 7. Persistence

- Active racks persist as `instruments/rack_XX.rack` (workspace and project), not beside sample `.pti` files.
- Saving a project copies active rack sidecars; inactive slots drop stale `rack_XX.rack` files.
- Legacy Option A projects that used sample-slot `playModeDrumRack` + `instrument_XX.rack` are **not** migrated: on load those sample slots are cleared empty.

## 8. Drum Rack vs Beat Slice

| | **Beat Slice** (stock drum kits) | **Drum Rack** |
| | | |
| Instrument type | Sample slot play mode | First-class rack pool instrument |
| Audio | One sliced sample | Pads point at other sample slots |
| Shared FX bus | Per sample instrument | Dedicated kit bus (filter / drive / bit / sends) |
| Instr list | Among samples `01–48` | Active racks listed above samples |

Use Beat Slice when the kit is already one sliced `.pti`. Use Drum Rack when you want a kit built from separate project instruments and one reverb (or filter) over the whole kit.
