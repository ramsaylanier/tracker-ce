# Feature Module SDK v2 — Developer Guide

The **Feature Module SDK** (`core/modules/sdk/`) lets you add compile-time firmware features that live in **this repo, another directory, or a separate repository**. Modules are ordinary C++ compiled from `FEATURE_MODULE_DIRS` and linked into the `.ptf` image.

There is **no SD-card code loader** for feature modules (use `.ptfx` for Sample Editor DSP plugins).

| Extension path | Delivery | Typical use |
| :--- | :--- | :--- |
| **`.ptfx` Sample Editor plugins** | SD card binary | Offline sample DSP / generators |
| **PolySynth Custom Synth SDK** | Compile into `.ptf` | Real-time synth engines (Tracker+ / Mini) |
| **Feature Module SDK v2** | Compile into `.ptf` from external dirs | Pages, softkeys, columns, Step FX, Config, sidecars |

---

## 1. What a module can do

| Capability | Registrar / API |
| :--- | :--- |
| **Add a page** to Pattern / Song / Instrument / Master / Performance | `addPage(ScreenId, ModulePage*, order)` |
| **Replace a builtin Master page** | `replacePage(ScreenId::Master, builtinPageId, ModulePage*)` |
| **Add / override softkeys** on a builtin page | `addSoftKey(ScreenId, builtinPageId, slot, SoftKeyHandler*)` |
| **Open a module page from a softkey** | `SoftKeyHandler::pageToOpen()` (Instrument Effects uses this) |
| **Song list column** | `addSongColumn(SongColumn*)` — first registration wins |
| **Pattern step column** | `addPatternColumn(PatternColumn*)` — first registration wins |
| **Pattern Step FX** | `addPatternStepFx(info, handler)` |
| **Config → Modules** | `addConfigItem(ConfigItem*)` |
| **Lifecycle events** | `addEventListener(ModuleEventListener*)` |
| **Project sidecar** | `ModuleSidecar` + open/save/clear hooks |
| **Sample FX query** | `setSampleInstrumentFxProvider` from `init` |
| **Song row mute query** | `setSongRowMuteProvider` from `init` (arrange greying + Rec mute) |

**Boundary rule:** change core only to grow the **host/dispatcher** surface. Product behavior stays in the module directory.

Do **not** include from a module: `performanceMode.h`, `songEditor.h`, `display.h`, File Manager headers, `mtStructs.h`, `mtAudioEngine.h`, or sequencer internals. Depend on SDK headers + `Platform.h` / `cr_section_macros.h`. Enforce with:

```bash
python3 scripts/check_module_includes.py
```

---

## 2. Architecture

```text
FEATURE_MODULE_DIRS ──► module.mk + sources
        │
        ▼
 generate_registry.py ──► feature_modules_registry.cpp
        │
        ▼
 FeatureModuleRegistry::registerAllExtensions()
        │
        ▼
 ExtensionCatalog (static arrays)
        ├── ScreenExtensionPoint per ModuleBase screen
        │     ├── Module pages (Mini dots / cycle / InputEvent)
        │     └── SoftKey overrides on builtin pages
        ├── SongColumn / PatternColumn (capacity 1 each; first-wins)
        ├── Pattern Step FX catalog
        └── Config "Modules" group
```

Modules register through `FeatureModule::registerExtensions(ExtensionRegistrar&)`. Core screens own a `ScreenExtensionPoint` + shared `ModulePageHost` (widget kit). List-style module pages share `ModuleListPageSupport` for title/list/softkey chrome. Domain work goes through services (`getPerformanceService()`, `getSongService()`, `getEngineService()`, …).

Changing `FEATURE_MODULE_DIRS` regenerates the registry via a stamp file (empty dirs yield an empty registry — no stale includes).

---

## 3. External module layout

```text
my-module/
├── module.mk
├── MyModule.h
└── MyModule.cpp
```

```makefile
MODULE_NAME        := my-module
MODULE_TITLE       := My Module
MODULE_DESCRIPTION := Short blurb shown in make select
MODULE_ORDER       := 40
MODULE_INSTANCE    := myModule
MODULE_HEADER      := MyModule.h
MODULE_SRCS        := MyModule.cpp
```

```bash
make select
make all FEATURE_MODULE_DIRS="/path/to/my-module"
make all FEATURE_MODULE_DIRS=   # firmware only
```

Directories under `feature-modules/` whose names start with `_` (e.g. `_example`) are **not** selected by default.

---

## 4. `FeatureModule` contract

```cpp
class FeatureModule
{
  public:
    virtual const char* id() const = 0;
    virtual void registerExtensions(ExtensionRegistrar& r) {}
    virtual void init(FileSystem* fs) {}
    virtual void onWorkspaceClear() {}
    virtual void onWorkspaceLoad() {}
    virtual void onProjectOpen(const char* path, const char* name) {}
    virtual void onProjectSave(const char* path, const char* name) {}
    virtual void update() {} // main thread only
};
```

---

## 5. Pages & softkeys

```cpp
class ModulePage
{
    virtual const char* title() const = 0;
    virtual bool canOpen(PageHost& host) const;
    virtual void onShow(PageHost& host) = 0;
    virtual void onHide(PageHost& host) = 0;
    virtual void onInput(PageHost& host, const InputEvent& event) = 0;
};
```

`PageHost` is the widget kit: `setTitle`, `setSoftKey` / `finishSoftKeys`, `setListCount` / `setListRow` / `setListFocus`, `refresh`, `close`, `isShiftPressed`, `device()`.

The host copies every string it is given, so a module can format into a stack buffer. Pass only the page name to `setTitle`: the screen adds its own page counter where it has one (Master shows `FX Buses 4/4`).

`InputEvent` carries `kind` (`SoftKey`, `Encoder`, `Navigate`, `HNavigate`, `Clear`, `Back`), `index`, `state`, `delta`, `shift`.

Softkeys on builtin pages:

```cpp
registrar.addSoftKey(ScreenId::Instrument, BuiltinPage::Instrument::Effects, 5, &myHandler);
```

To open a specific module page from that softkey (instead of relying on page index), override `SoftKeyHandler::pageToOpen()` and return the `ModulePage*`. Instrument Effects calls this after a handled press.

Pattern module pages via `addPage(ScreenId::Pattern, …)` are supported (see `_example`). No production Pattern page ships yet.

---

## 6. Columns, Step FX, Config, Services

- **SongColumn / PatternColumn** — capacity is **1** each (`kMaxSongColumns` / `kMaxPatternColumns`). A second registration fails; only the first is shown (**first-wins**). Song cells ≤ 4 chars; Pattern optional field after FX2 (`selectedParam == 4`).
- **Pattern Step FX** — type ids `128..143`; handler is a function pointer. It is called from the sequencer tick inside the audio buffer callback, not the main loop, so keep it short and do not do file I/O or draw from it.
- **ConfigItem** / **ModuleEventListener** — song slot start, playlist replace/insert/remove, pattern load/save. Used by production modules and `_example`.
- **Services** — `getPerformanceService()` (scenes / FX rack / mute queries), `getSongService()` (`selectedSlot`), `getEngineService()` (instrument FX fields + `notifyInstrumentFxDirty`).
- **Song row mutes** — `SongRowMuteQuery.h` provider for per-playlist-slot mute greying and Rec Shift+Mute on the arrange grid.
- **`replacePage`** — **Master-only**. Song / Pattern accept `addPage` and soft keys / columns, but do not replace their builtin views.

---

## 7. Bundled modules

| Directory | Hooks |
| :--- | :--- |
| `performance-fx-scenes/` | Performance page **Scenes** |
| `pattern-launch-scenes/` | Performance page **Patterns** |
| `song-row-mutes/` | Song arrange per-row track mutes (`songrowmutes.bin`) |
| `song-fx-scenes/` | Song column + Song page + Step FX `N` |
| `fx-buses/` | Master + Instrument editor pages (separate UI state) + Instrument Effects softkeys + sample FX provider |
| `_example/` | Hook demo: Performance page, Pattern page, softkey, Config, events (not built by default) |

---

## 8. Out of scope

- SD-card executable feature modules
- Audio-callback / ISR module hooks beyond `SampleInstrumentFxQuery` / song-row mute apply (main thread)
- Multi-column cycling (hosts keep first-wins only)
- Song / Pattern `replacePage` hosts
- New top-level `ModuleBase` screens (slot 11 remains reserved)
- Synth engine authoring (PolySynth SDK)
