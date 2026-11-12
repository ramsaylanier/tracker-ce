#ifndef MODULES_SDK_SOFTKEY_H_
#define MODULES_SDK_SOFTKEY_H_

#include "Input.h"
#include "UiKit.h"

#include <stdint.h>

class ModulePage;

// Soft-key handler for a builtin page slot (0..7).
// May open a module page by returning it from pageToOpen() after onPress.
class SoftKeyHandler
{
  public:
    virtual ~SoftKeyHandler() {}

    virtual const char* label() const = 0;
    virtual const char* label2() const { return nullptr; }
    virtual LabelState state() const { return LabelState::Filled; }

    // Return true if the press was handled (and the builtin handler should skip).
    virtual bool onPress(PageHost& host, SoftKeyButtonState state, bool shift) = 0;

    // Optional module page to open after a handled Press. Default: none.
    virtual ModulePage* pageToOpen() const { return nullptr; }
};

#endif /* MODULES_SDK_SOFTKEY_H_ */
