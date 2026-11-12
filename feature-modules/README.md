# Feature Modules

External compile-time modules for Tracker CE. Each directory is a self-contained feature that uses the **Module SDK v2** (`core/modules/sdk/`) and is linked into the firmware via `FEATURE_MODULE_DIRS`.

See [`docs/feature_module_sdk.md`](../docs/feature_module_sdk.md).

## Bundled modules

| Directory | Hooks | Order |
| :--- | :--- | ---: |
| [`performance-fx-scenes/`](performance-fx-scenes/) | Performance page **Scenes** | 10 |
| [`song-fx-scenes/`](song-fx-scenes/) | Song FX column + page + Step FX `N` | 15 |
| [`pattern-launch-scenes/`](pattern-launch-scenes/) | Performance page **Patterns** | 20 |
| [`mute-banks/`](mute-banks/) | Performance page **Mutes** | 30 |
| [`fx-buses/`](fx-buses/) | Master + Instrument FX Buses pages + softkeys | 40 |
| [`_example/`](_example/) | SDK hook demo (Performance + Pattern pages; not built by default) | 90 |

## Build

```bash
make select
# or
python3 scripts/build_with_modules.py

make all
make all FEATURE_MODULE_DIRS=
make all FEATURE_MODULE_DIRS="feature-modules/_example"
```

Changing `FEATURE_MODULE_DIRS` regenerates `feature_modules_registry.cpp` automatically (stamp file); an empty list yields an empty registry.

## Softkeys that open pages

`SoftKeyHandler::pageToOpen()` returns the `ModulePage*` the host should open after a handled press. Instrument Effects uses this so **Edit Bus** opens the FxBuses Instrument editor regardless of page registration order.

## List-page hosts

Master / Song / Instrument / Pattern share `ModuleListPageSupport` for title, list rows, focus, and softkey chrome. Performance stays softkey-only for its module pages.

## Include rule

```bash
python3 scripts/check_module_includes.py
```

Modules may only include SDK headers (`FeatureModule.h`, `sdk/*`), `ModuleSidecar.h`, `SampleInstrumentFxQuery.h`, `Platform.h`, `cr_section_macros.h`, and standard C/C++ headers.
