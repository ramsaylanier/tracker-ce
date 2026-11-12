#ifndef MODULES_SDK_COLUMNS_H_
#define MODULES_SDK_COLUMNS_H_

#include "UiKit.h"

#include <stdint.h>

// Optional focusable column on the Song playlist list.
class SongColumn
{
  public:
    static constexpr uint8_t kSongSlotCount = 255;

    virtual ~SongColumn() {}

    virtual const char* header() const = 0;
    virtual const char* focusEncoderLabel() const = 0;
    virtual void formatCell(uint8_t slot, char* out, uint8_t outSize) const = 0;

    virtual void onEncoder(PageHost& host, uint8_t slot, int8_t delta) = 0;
    virtual void onClear(PageHost& host, uint8_t slot) = 0;

    virtual void onInserted(uint8_t index, bool duplicate) = 0;
    virtual void onRemoved(uint8_t index) = 0;
    virtual void clearAll() = 0;

    // Main thread only. Called when song playback enters this playlist slot.
    virtual void onSlotStart(uint8_t slot) = 0;
};

// Optional Pattern step column. The editor and sequencer currently use only the first
// registered column (patternColumnAt(0)).
class PatternColumn
{
  public:
    static constexpr uint8_t kMaxTracks = 16;
    static constexpr uint16_t kMaxSteps = 128;

    virtual ~PatternColumn() {}

    virtual const char* header() const = 0;
    virtual char letter() const = 0;

    virtual void formatCell(uint8_t track, uint16_t step, char* out, uint8_t outSize) const = 0;
    virtual void onEdit(uint8_t track, uint16_t step, int8_t delta) = 0;
    virtual void onClear(uint8_t track, uint16_t step) = 0;

    // Called from the sequencer tick inside the audio buffer callback (same path as
    // pattern Step FX handlers), not from the main loop. Keep it short: no file I/O,
    // no PageHost calls.
    virtual void onStep(uint8_t track, uint16_t step) = 0;

    virtual void onInserted(uint8_t track, uint16_t step, bool duplicate) = 0;
    virtual void onRemoved(uint8_t track, uint16_t step) = 0;

    // Pattern lifecycle (per-pattern sidecar).
    virtual void onPatternLoad(uint16_t patternIdx) = 0;
    virtual void onPatternSave(uint16_t patternIdx) = 0;
};

#endif /* MODULES_SDK_COLUMNS_H_ */
