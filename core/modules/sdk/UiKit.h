#ifndef MODULES_SDK_UIKIT_H_
#define MODULES_SDK_UIKIT_H_

#include <stdint.h>

enum class LabelState : uint8_t
{
    Empty = 0,  // inactive colors
    Filled = 1, // slot has data
    Active = 2, // currently recalled / armed
    Muted = 3,  // mute emphasis
};

enum class DeviceKind : uint8_t
{
    Original = 0,
    Plus = 1,
    Mini = 2,
};

// Host-provided widget kit. Modules must not call display internals.
//
// The host copies every string it is given, so passing a stack buffer is fine.
// Copies are truncated: titles to 39 characters, soft-key texts to 13.
class PageHost
{
  public:
    virtual ~PageHost() {}

    // Page name only. The screen may append its own page counter (Master shows "Name 4/4").
    virtual void setTitle(const char* title) = 0;

    // Soft keys under the display (0..7).
    virtual void setSoftKey(uint8_t index,
                            const char* text,
                            LabelState state,
                            const char* text2 = nullptr) = 0;
    virtual void finishSoftKeys() = 0;

    // Param name + value list.
    virtual void setListCount(uint8_t count) = 0;
    virtual void setListRow(uint8_t index, const char* name, const char* value) = 0;
    virtual void setListFocus(uint8_t index, bool editingValue) = 0;

    virtual void refresh() = 0;
    // Leave the module page and restore the host screen chrome.
    virtual void close() = 0;

    virtual bool isShiftPressed() const = 0;
    virtual DeviceKind device() const = 0;
};

#endif /* MODULES_SDK_UIKIT_H_ */
