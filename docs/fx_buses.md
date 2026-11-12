# FX Buses (experiment)

Shared filter / drive / bit-depth / send buses for **sample instruments** (`01–48`). Sequencing and sound design stay on each sample slot; a bus is an optional group overlay.

## Membership

- Each sample instrument may join **one** FX bus: `Off`, `Bus 1` … `Bus 4`.
- Assign from **Instrument Effects** → soft key **Bus** (encoder / arrows).
- Membership and bus settings are stored in a Feature Module sidecar (`fxbuses.bin`), not in the `.pti` instrument blob (instrument size stays 372 bytes).

## What the bus owns

| Parameter | Behavior when instrument is on a bus |
|-----------|--------------------------------------|
| Filter enable / type / cutoff / resonance | **Replace** the instrument filter block for that voice |
| Overdrive | **Replace** instrument overdrive |
| Bit depth | **Replace** instrument bit depth |
| Delay send | **Additive**: `min(100, instrumentSend + busSend)` |
| Reverb send | **Additive**: `min(100, instrumentSend + busSend)` |

With **Bus = Off**, the voice uses the instrument’s own filter / drive / bit / sends as usual. Instrument delay/reverb sends remain editable while on a bus (the bus acts as a group offset).

## Editing a bus

**Instrument Effects → Edit Bus**

- Opens the Instrument FX Buses editor for the bus the current instrument belongs to (or Bus 1 if Off). Softkey `pageToOpen()` targets that page by identity.
- Edit filter, cutoff, resonance, overdrive, bit depth, reverb send, delay send.

**Master → FX Buses 4/4**

- Cycle Master pages: Master 1/4 → Track Mixer 2/4 → Global Mixer 3/4 → **FX Buses 4/4**. On the Mini, pick the page from the Module Menu (Middle cycles the Master pages).
- Soft keys 1–4 select **Bus 1–4** (the bus being edited is highlighted). **Prev** / **Next** or Up / Down move between the bus params, the encoder changes the focused value, and **Back** returns to Master 1/4.
- Master edits **bus definitions only**; membership stays on Instrument Effects.

Master and Instrument each use a separate `ModulePage` adapter with its own focus/bus UI state; both share one bank/storage owner. Changes from either editor refresh currently sounding **member** voices (linked per-voice FX — each member voice applies the same bus settings; this is not a true summed-bus filter).

## Audio model (v1)

```
Track → Sample instrument (full Instr Params)
              ├─ Bus Off  → instrument filter / drive / bit / sends
              └─ Bus 1..4 → bus filter / drive / bit
                            + effective sends = clamp(instr + bus)
```

Core audio resolves these values through `SampleInstrumentFxQuery` (default = instrument fields). The FX Buses module installs a provider at `init` so bus membership overlays apply without core including `FxBuses.h`.

The Instrument Effects soft keys and the Master page are registered by the module through the Module SDK (`registerExtensions`), with no core `#ifdef`. Builds with FX Buses unselected have no Bus soft keys and three Master pages (1/3 to 3/3).

True audio-summed buses (one filter over a mixed kick+snare) are out of scope for this experiment.

## Persistence

- Workspace / project load-save via the `fxbuses` Feature Module sidecar.
- Older projects without a sidecar default to **no bus** membership and default bus params.
