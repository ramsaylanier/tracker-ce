#ifndef INTERFACE_CORE_MODULELISTPAGESUPPORT_H_
#define INTERFACE_CORE_MODULELISTPAGESUPPORT_H_

#include "displayStructs.h"
#include "paramValueList.h"

#include <stdint.h>

// Shared param/value list chrome for ModulePage hosts on Master / Song / Instrument / Pattern.
// Each screen embeds one of these and still owns its own active page + show/hide chrome.
class ModuleListPageSupport
{
  public:
    static constexpr uint8_t kRows = 8;
    static constexpr uint8_t kNameLen = 24;
    static constexpr uint8_t kValueLen = 12;

    // Creates the full-bleed list control used by module pages (y=29, h=394).
    void createListControl();

    void showList();
    void hideList();

    void setListCount(uint8_t count);
    void setListRow(uint8_t index, const char* name, const char* value);
    void setListFocus(uint8_t index, bool editingValue);
    void refresh();

    hControl control() const { return listControl; }

  private:
    hControl listControl = -1;
    strParamValueList list{};
    char names[kRows][kNameLen]{};
    char values[kRows][kValueLen]{};
    char* namePtrs[kRows]{};
    char* valuePtrs[kRows]{};
};

#endif /* INTERFACE_CORE_MODULELISTPAGESUPPORT_H_ */
