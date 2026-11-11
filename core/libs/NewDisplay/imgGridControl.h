#ifndef LIBS_NEWDISPLAY_IMGGRIDCONTROL_H_
#define LIBS_NEWDISPLAY_IMGGRIDCONTROL_H_

#include "displayControls.h"

struct strImgGridData
{
    uint8_t columnsCount;
    uint8_t rowsCount;

    uint8_t imgW = 0;
    uint8_t imgH = 0;

    bool showSelection = false;
    // When true, textList is centered inside each pad square instead of below it.
    bool textInPad = false;
    int selection;

    char** textList;
    // Optional second line drawn below each pad (e.g. sample slot under note).
    char** subTextList = nullptr;
    int8_t* imgList;
};

//--------------------------------------------------------------------
//--------------------------------------------------------------------
//--------------------------------------------------------------------
class ImgGrid : public cDisplayControl
{
  public:
    ImgGrid(strControlProperties* properties = nullptr);
    virtual ~ImgGrid();

    virtual void update();

    virtual void setStyle(uint32_t style);
    virtual void setText(const char* text);
    virtual void setValue(int value);
    virtual void setColors(const uint32_t* colors);
    virtual void setData(void* data);

  protected:
    strImgGridData* data;

    uint16_t textStyle;
};

#endif
