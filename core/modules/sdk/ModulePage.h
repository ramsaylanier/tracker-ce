#ifndef MODULES_SDK_MODULEPAGE_H_
#define MODULES_SDK_MODULEPAGE_H_

#include "Input.h"
#include "UiKit.h"

// Guest page hosted inside an existing ModuleBase screen.
class ModulePage
{
  public:
    virtual ~ModulePage() {}

    virtual const char* title() const = 0;
    virtual bool canOpen(PageHost& host) const
    {
        (void)host;
        return true;
    }

    virtual void onShow(PageHost& host) = 0;
    virtual void onHide(PageHost& host) = 0;
    virtual void onInput(PageHost& host, const InputEvent& event) = 0;
};

#endif /* MODULES_SDK_MODULEPAGE_H_ */
