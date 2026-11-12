#include "moduleListPageSupport.h"

#include "display.h"
#include "paramValueList.h"

#include <cstdio>

void ModuleListPageSupport::createListControl()
{
    for (uint8_t i = 0; i < kRows; i++)
    {
        namePtrs[i] = names[i];
        valuePtrs[i] = values[i];
        names[i][0] = '\0';
        values[i][0] = '\0';
    }

    list.linesCount = kRows;
    list.start = 0;
    list.length = kRows;
    list.params = namePtrs;
    list.values = valuePtrs;
    list.valueEditActive = 0;

    strControlProperties listProp;
    listProp.style = controlStyleFont1 | controlStyleBackground |
                     (display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0);
    listProp.x = 0 + 1;
    listProp.y = 29;
    listProp.w = 800 - 2;
    listProp.h = 394;
    listProp.value = 0;
    listProp.colors = display.getTheme().activeListColors;
    listProp.data = &list;
    listControl = display.createControl<cParamValueList>(&listProp);
}

void ModuleListPageSupport::showList()
{
    display.setControlData(listControl, &list);
    display.setControlValue(listControl, 0);
    display.setControlShow(listControl);
    display.refreshControl(listControl);
}

void ModuleListPageSupport::hideList()
{
    display.setControlHide(listControl);
    display.refreshControl(listControl);
}

void ModuleListPageSupport::setListCount(uint8_t count)
{
    if (count > kRows)
    {
        count = kRows;
    }
    list.length = count;
    list.linesCount = count > 0 ? count : 1;
    list.params = namePtrs;
    list.values = valuePtrs;
}

void ModuleListPageSupport::setListRow(uint8_t index, const char* name, const char* value)
{
    if (index >= kRows)
    {
        return;
    }

    snprintf(names[index], kNameLen, "%s", name != nullptr ? name : "");
    snprintf(values[index], kValueLen, "%s", value != nullptr ? value : "");
}

void ModuleListPageSupport::setListFocus(uint8_t index, bool editingValue)
{
    list.start = index;
    list.valueEditActive = editingValue ? 1 : 0;
    display.setControlData(listControl, &list);
    display.setControlValue(listControl, index);
    display.setControlShow(listControl);
    display.refreshControl(listControl);
}

void ModuleListPageSupport::refresh()
{
    display.setControlData(listControl, &list);
    display.refreshControl(listControl);
    display.synchronizeRefresh();
}
