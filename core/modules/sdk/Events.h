#ifndef MODULES_SDK_EVENTS_H_
#define MODULES_SDK_EVENTS_H_

#include <stdint.h>

// Firmware event listeners for feature modules.
// onSongSlotStart may run inside the audio buffer callback — keep it short
// (no file I/O, no PageHost). Other hooks are main-thread.
class ModuleEventListener
{
  public:
    virtual ~ModuleEventListener() {}

    virtual void onSongSlotStart(uint8_t slot) { (void)slot; }
    virtual void onSongPlaylistReplaced() {}
    // Playlist row inserted after `index` (duplicate copies row `index`).
    virtual void onSongInserted(uint8_t index, bool duplicate)
    {
        (void)index;
        (void)duplicate;
    }
    virtual void onSongRemoved(uint8_t index) { (void)index; }
    virtual void onPatternLoad(uint16_t patternIdx) { (void)patternIdx; }
    virtual void onPatternSave(uint16_t patternIdx) { (void)patternIdx; }
};

#endif /* MODULES_SDK_EVENTS_H_ */
