
#ifndef INTERFACE_CONFIGEDITOR_CONFIGMENU_H_
#define INTERFACE_CONFIGEDITOR_CONFIGMENU_H_

#include "configEditor/configMenuDefs.h"
#include "util_math.h"

#include <cstdio>

class cMenuGroup;
class cMenuItem;

class cMenuBase
{
  protected:
    cMenuBase(menu_t menu_type)
      : type(menu_type)
    {
    }
    menu_t type;

    friend cMenuGroup;
    friend cMenuItem;
};

typedef cMenuBase* hMenuItem;

class cMenuGroup : public cMenuBase
{
  public:
    cMenuGroup(cMenuGroup& parent, const char* name, uint8_t childs_count)
      : cMenuBase(menuTypeGroup)
      , parentMenu(&parent)
      , childsCapacity(childs_count)
      , childsCount(childs_count)
      , childs(new hMenuItem[childs_count]())
      , childsNames(new char*[childs_count]())
      , childsValues(new char*[childs_count]())
      , groupName(name)
    {
        slot = parent.getNextSlotNumber();
        if (slot < parent.childsCount)
        {
            parent.childs[slot] = this;
            parent.childsNames[slot] = (char*)name;
        }
        selectedItem = 0;
        slotsUsed = 0;
    }

    ~cMenuGroup() { delete[] childs; }

    cMenuGroup* getSelChild() { return (cMenuGroup*)childs[selectedItem]; }

    uint8_t getCount() { return childsCount; }

    // Visible child count; must not exceed capacity allocated at construction.
    void setChildCount(uint8_t count)
    {
        if (count > childsCapacity)
        {
            count = childsCapacity;
        }
        childsCount = count;
        if (childsCount == 0)
        {
            selectedItem = 0;
        }
        else if (selectedItem >= childsCount)
        {
            selectedItem = childsCount - 1;
        }
    }

    char** getNames() { return childsNames; }

    char** getValues()
    {
        reloadValues();
        return childsValues;
    }

    uint8_t getSelectedItem() { return selectedItem; }

    uint8_t getNextSlotNumber()
    {
        uint8_t nextSlot = slotsUsed;
        slotsUsed++;
        return nextSlot;
    }

    void reloadValues();

    void executeItem();

    int getSlot() { return slot; }

  private:
    cMenuGroup* parentMenu;
    const uint8_t childsCapacity;
    uint8_t childsCount;
    hMenuItem* childs;
    char** childsNames;
    char** childsValues;
    const char* groupName;

    uint8_t selectedItem;
    uint8_t slotsUsed;
    uint8_t slot;

    friend class cMenuItem;
    friend class cConfigEditor;
};

class cMenuItem : public cMenuBase
{
  public:
    cMenuItem(cMenuGroup& parent,
              const char* name,
              const char* desctription,
              menu_item_t type,
              const void* setupPtr)
      : cMenuBase(menuTypeItem)
      , parentMenu(&parent)
      , itemName(name)
      , itemDesc(desctription)
      , itemType(type)
      , itemSetup(setupPtr)
      , ptrValueText(itemType == menuItemTypeValueU8 || itemType == menuTypeItemListValues
                       ? new char[kTempValueTextLen + 1]
                       : nullptr)
      , slot(parent.getNextSlotNumber())
    {
        if (slot < parent.childsCapacity)
        {
            parent.childs[slot] = this;
            parent.childsNames[slot] = (char*)name;
        }
        valueEditState = 0;
        applySetupSafety();
    }

    ~cMenuItem() {}

    // Rebind name/type/setup after construction (used by Modules config pool).
    void reconfigure(const char* name, const char* desctription, menu_item_t type, const void* setupPtr)
    {
        itemName = name;
        itemDesc = desctription;
        itemType = type;
        itemSetup = setupPtr;
        if (parentMenu != nullptr && slot < parentMenu->childsCapacity)
        {
            parentMenu->childsNames[slot] = (char*)name;
        }
        if ((itemType == menuItemTypeValueU8 || itemType == menuTypeItemListValues) &&
            ptrValueText == nullptr)
        {
            ptrValueText = new char[kTempValueTextLen + 1];
        }
        valueEditState = 0;
        applySetupSafety();
    }

    char* getValue()
    {
        switch (itemType)
        {
            case menuItemTypeValueU8:
            {
                snprintf(ptrValueText,
                         kTempValueTextLen,
                         "%d",
                         *((strItemTypeValue8*)itemSetup)->value +
                           ((strItemTypeValue8*)itemSetup)->dev);
                return ptrValueText;
            }
            case menuTypeItemListValues:
            {
                snprintf(ptrValueText,
                         kTempValueTextLen,
                         "%d",
                         *((strItemTypeValue8*)itemSetup)->value +
                           ((strItemTypeValue8*)itemSetup)->dev);
                return ptrValueText;
            }
            case menuTypeItemListText:
                return (char*)((strItemTypeListText*)itemSetup)
                  ->ptrText[*((strItemTypeListText*)itemSetup)->value];
            case menuTypeItemListTextWithAction:
                return (char*)((strItemTypeListTextWithAction*)itemSetup)
                  ->ptrText[*((strItemTypeListTextWithAction*)itemSetup)->value];
            case menuTypeItemLabel: return (char*)((strItemTypeLabel*)itemSetup)->text;
            case menuTypeItemLabelPtr: return (char*)*(((strItemTypeLabelPtr*)itemSetup)->text);

            case menuItemTypeEmpty:
            case menuTypeItemActionButton: return (char*)ptrEmptyName;
        }
        return (char*)ptrEmptyName;
    }

    menu_item_t getItemType() { return itemType; }

    const void* getItemSetup() { return itemSetup; }

    uint8_t getValueEditState() { return valueEditState; }

    void resetValueEditState() { valueEditState = 0; }

  private:
    void applySetupSafety()
    {
        switch (itemType)
        {
            case menuTypeItemListText:
            {
                if (itemSetup == nullptr || ((strItemTypeListText*)itemSetup)->value == nullptr ||
                    ((strItemTypeListText*)itemSetup)->count == 0)
                {
                    break;
                }
                *((strItemTypeListText*)itemSetup)->value =
                  clamp<uint8_t>(*((strItemTypeListText*)itemSetup)->value,
                                 0,
                                 (((strItemTypeListText*)itemSetup)->count) - 1);
                break;
            }
            case menuTypeItemListTextWithAction:
            {
                if (itemSetup == nullptr ||
                    ((strItemTypeListTextWithAction*)itemSetup)->value == nullptr ||
                    ((strItemTypeListTextWithAction*)itemSetup)->count == 0)
                {
                    break;
                }
                *((strItemTypeListTextWithAction*)itemSetup)->value =
                  clamp<uint8_t>(*((strItemTypeListTextWithAction*)itemSetup)->value,
                                 0,
                                 (((strItemTypeListTextWithAction*)itemSetup)->count) - 1);
                break;
            }
            case menuTypeItemLabel:
            {
                if (itemSetup != nullptr && ((strItemTypeLabel*)itemSetup)->text == nullptr)
                {
                    ((strItemTypeLabel*)itemSetup)->text = ptrEmptyName;
                }
                break;
            }
            case menuTypeItemLabelPtr:
            {
                if (itemSetup != nullptr && ((strItemTypeLabelPtr*)itemSetup)->text == nullptr)
                {
                    ((strItemTypeLabelPtr*)itemSetup)->text = (const char**)&ptrEmptyName;
                }
                break;
            }
            default: break;
        }
    }

    cMenuGroup* parentMenu;
    const char* itemName;
    const char* itemDesc;
    menu_item_t itemType;
    const void* itemSetup;

    static constexpr uint8_t kTempValueTextLen = 5;
    char* ptrValueText;

    uint8_t valueEditState;
    uint8_t slot;

    friend class cMenuGroup;
};

#endif /* INTERFACE_CONFIGEDITOR_CONFIGMENU_H_ */
