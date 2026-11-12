#ifndef MODULES_SDK_CONFIGITEM_H_
#define MODULES_SDK_CONFIGITEM_H_

#include <stdint.h>

enum class ConfigItemType : uint8_t
{
    ListText = 0,   // uint8_t value + string table
    ValueU8 = 1,    // uint8_t min..max
    Action = 2,     // trigger callback
};

class ConfigItem
{
  public:
    virtual ~ConfigItem() {}

    virtual const char* name() const = 0;
    virtual const char* description() const { return ""; }
    virtual ConfigItemType type() const = 0;

    // ListText / ValueU8
    virtual uint8_t* valuePtr() { return nullptr; }
    virtual uint8_t minValue() const { return 0; }
    virtual uint8_t maxValue() const { return 0; }
    virtual uint8_t listCount() const { return 0; }
    virtual const char* const* listTexts() const { return nullptr; }

    virtual void onChange() {}
    virtual void onAction() {}
};

#endif /* MODULES_SDK_CONFIGITEM_H_ */
