#include "instrumentEditor/instrumentEditor.h"

#include "DrumRack.h"
#include "core/interface.h"
#include "core/interfacePopups.h"
#include "fileManager.h"
#include "mtAudioEngine.h"
#include "mtConfig.h"
#include "mtPadBoard.h"
#include "mtPadsBacklight.h"
#include "mtSequencer.h"
#include "util_math.h"

#include <stdio.h>

extern playerEngine instrumentPlayer[];

static cInstrumentEditor* IE = &instrumentEditor;

static strDrumRackInstrument* editRack()
{
    return DrumRack::globalInstrument(mtProject.values.lastUsedInstrument);
}

static void markRackChanged()
{
    if (!isDrumRackInstrument(mtProject.values.lastUsedInstrument))
        return;
    DrumRack::saveSidecar((uint8_t)drumRackInstrumentToIdx(mtProject.values.lastUsedInstrument),
                          newFileManager.getFileSystem());
}

static void refreshKitVoices(uint32_t mask)
{
    const uint8_t globalIdx = mtProject.values.lastUsedInstrument;
    for (uint8_t i = 0; i < audioEngine::kVoices; i++)
    {
        if (DrumRack::isVoiceOnGlobalKit(i, globalIdx))
            instrumentPlayer[i].setRefreshBit(mask);
    }
}

static void refreshKitFilterRouting()
{
    const uint8_t globalIdx = mtProject.values.lastUsedInstrument;
    for (uint8_t i = 0; i < audioEngine::kVoices; i++)
    {
        if (DrumRack::isVoiceOnGlobalKit(i, globalIdx))
            instrumentPlayer[i].applyDrumRackBusFilterState();
    }
}

static uint8_t selectedPadIdx()
{
    strDrumRackInstrument* rack = editRack();
    return rack != nullptr ? rack->rack.selectedPad : 0;
}

// Screen grid is top-left origin; pad 0 (Start Note) is bottom-left, then up/right.
static uint8_t drumRackPadToGridIdx(uint8_t pad)
{
    constexpr uint8_t kCols = 4;
    constexpr uint8_t kRows = 4;
    if (pad >= kDrumRackPadCount)
        pad = kDrumRackPadCount - 1;
    const uint8_t rowFromBottom = pad / kCols;
    const uint8_t col = pad % kCols;
    return (uint8_t)((kRows - 1 - rowFromBottom) * kCols + col);
}

static const char* const kDrumRackFilterLabels[filterModeCount] = {
    "Disabled",
    "Low-pass",
    "High-pass",
    "Band-pass",
};

static void functDrumRackPadsEncoder(int16_t value);
static void functDrumRackPadsUp();
static void functDrumRackPadsDown();
static void functDrumRackPadsLeft();
static void functDrumRackPadsRight();
static void functDrumRackPadsButtons(uint8_t button, uint8_t state);
static void functDrumRackPadsDelete();
static void functDrumRackPads(uint8_t pad, uint8_t state, int16_t velo);
static void functDrumRackPadsShift(uint8_t state);
static void functDrumRackPadsMiniMiddle(uint8_t state);
static void functDrumRackPadsMoreBack();
static void functDrumRackPadsEnvEnter();
static void unfocusDrumRackOverviewSoftKeys();

static void functDrumRackBusEncoder(int16_t value);
static void functDrumRackBusUp();
static void functDrumRackBusDown();
static void functDrumRackBusLeft();
static void functDrumRackBusRight();
static void functDrumRackBusButtons(uint8_t button, uint8_t state);
static void functDrumRackBusDelete();
static void functDrumRackBusPads(uint8_t pad, uint8_t state, int16_t velo);
static void functDrumRackBusShift(uint8_t state);
static void functDrumRackBusMiniMiddle(uint8_t state);

static void applyBarStyles()
{
    uint32_t shadow = (display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0);
    uint32_t gradient = (display.getTheme().barShadow > 0 ? controlStyleGradient : 0);

    for (uint8_t i = 0; i < 8; i++)
    {
        display.setControlStyle(
          IE->barControl[i],
          (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground) | shadow |
            gradient);
    }
}

static void clearSoftKeyChrome()
{
    for (uint8_t i = 0; i < 8; i++)
    {
        display.setControlColors(IE->label[i], display.getTheme().activeLabelsColors);
        display.setControlColors(IE->barControl[i], IE->localActiveBarColors);
        display.setRemoveControlStyle(IE->label[i], controlStyleLimitTextWidth);
        display.setRemoveControlStyle(IE->label[i], controlStyleSpecialFx2);
        display.setRemoveControlStyle(IE->label[i], controlStyleShowBitmap);
        display.setControlText2(IE->label[i], "");
        display.setControlValue(IE->label[i], 1);
        display.setControlStyle2(IE->label[i],
                                 controlStyleCenterX | controlStyleCenterY | controlStyleFont2);
        display.setControlShow(IE->label[i]);
        display.setControlHide(IE->barControl[i]);
    }

    display.setControlHide(IE->doubleLabelAutomation);
    display.setControlHide(IE->instrumentSynthCCListControl);
    display.setControlHide(IE->instrumentListControl);
    display.setControlHide(IE->filterModeListControl);
    display.setControlHide(IE->blankSpaceCover[0]);
    display.setControlHide(IE->list1Control);
    display.setControlHide(IE->list2Control);
    display.setControlHide(IE->list3Control);
    display.setControlHide(IE->list4Control);
    display.setControlHide(IE->paramsListControl);
    display.setControlHide(IE->modelListControl);
    display.setControlHide(IE->imgGridControl);
    display.setControlPosition(IE->imgGridControl, (800 / 16) * 1, 97);
    display.setControlSize(IE->imgGridControl, (800 / 8) * 3, 308);
    display.setControlHide(IE->noteLabel);

    for (uint8_t i = 0; i < 6; i++)
        display.setControlHide(IE->ccNameListControl[i]);
}

static void previewDrumRackPad(uint8_t pad, uint8_t state)
{
    // Hardware pad layout stays beat-slice ordered; map to rack pad index, then play the
    // rack's configured start-note + pad offset.
    const uint8_t padIdx = drumRackNoteToPad(mtPadBoard.getNoteFromPad(pad, true));
    const uint8_t playNote =
      drumRackPadToNote(padIdx, DrumRack::firstNoteOfGlobal(mtProject.values.lastUsedInstrument));

    if (state == buttonPress)
    {
        padsBacklight.setFrontLayer(1, mtConfig.values.padsLightFront, pad);
        DrumRack::selectPad(mtProject.values.lastUsedInstrument, padIdx);
        sequencer.handleNoteOn(Sequencer::GRID_OUTSIDE_PATTERN, playNote, -1, pad);
    }
    else if (state == buttonRelease)
    {
        padsBacklight.setFrontLayer(0, 0, pad);
        sequencer.handleNoteOff(Sequencer::GRID_OUTSIDE_PATTERN, playNote, 0, pad);
    }
}

// Audition the selected pad after encoder sample assignment (source pad 12 = Enter/preview).
static void previewSelectedDrumRackPad()
{
    strDrumRackInstrument* rack = editRack();
    if (rack == nullptr)
        return;

    const uint8_t padIdx = rack->rack.selectedPad;
    const uint8_t playNote = drumRackPadToNote(padIdx, rack->rack.firstNote);

    sequencer.handleNoteOff(Sequencer::GRID_OUTSIDE_PATTERN, playNote, 0, 12);
    if (rack->rack.pads[padIdx].sampleSlot == kDrumRackEmptySlot)
        return;
    sequencer.handleNoteOn(Sequencer::GRID_OUTSIDE_PATTERN, playNote, -1, 12);
}

void cInstrumentEditor::setDrumRackPadsScreenFunct()
{
    setBaseScreenFunct();

    ip->clearButtonsRange(interfaceButton0, interfaceButton7);
    ip->clearAllPots();

    for (uint8_t i = interfaceButton0; i <= interfaceButton5; i++)
        ip->setButtonObj(i, functDrumRackPadsButtons);

    if (drumRackPadEnvPage)
    {
        ip->clearButton(interfaceButton6);
        ip->setButtonObj(interfaceButton7, buttonPress, functDrumRackPadsMoreBack);
    }
    else if (drumRackPadDetailPage)
    {
        ip->setButtonObj(interfaceButton6, buttonPress, functDrumRackPadsEnvEnter);
        ip->setButtonObj(interfaceButton7, buttonPress, functDrumRackPadsMoreBack);
    }
    else
    {
        // Overview: soft key 6 = Start Note focus, soft key 7 = Pad → detail.
        ip->setButtonObj(interfaceButton6, functDrumRackPadsButtons);
        ip->setButtonObj(interfaceButton7, buttonPress, functDrumRackPadsMoreBack);
    }

    ip->setPotObj(interfacePot0, functDrumRackPadsEncoder);
    ip->setButtonObj(interfaceButtonLeft, buttonPress, functDrumRackPadsLeft);
    ip->setButtonObj(interfaceButtonRight, buttonPress, functDrumRackPadsRight);
    ip->setButtonObj(interfaceButtonUp, buttonPress, functDrumRackPadsUp);
    ip->setButtonObj(interfaceButtonDown, buttonPress, functDrumRackPadsDown);
    ip->setButtonObj(interfaceButtonDelete, buttonPress, functDrumRackPadsDelete);
    ip->clearButton(interfaceButtonInsert);
    ip->setButtonObj(interfaceButtonShift, functDrumRackPadsShift);
    ip->setButtonObj(interfaceButtonMiniMiddle, functDrumRackPadsMiniMiddle);
    ip->setPadsGlobal(functDrumRackPads);
}

void cInstrumentEditor::setDrumRackBusScreenFunct()
{
    setBaseScreenFunct();

    ip->clearButtonsRange(interfaceButton0, interfaceButton7);
    ip->clearAllPots();

    for (uint8_t i = interfaceButton0; i <= interfaceButton6; i++)
        ip->setButtonObj(i, functDrumRackBusButtons);

    ip->clearButton(interfaceButton7);

    ip->setPotObj(interfacePot0, functDrumRackBusEncoder);
    ip->setButtonObj(interfaceButtonLeft, buttonPress, functDrumRackBusLeft);
    ip->setButtonObj(interfaceButtonRight, buttonPress, functDrumRackBusRight);
    ip->setButtonObj(interfaceButtonUp, buttonPress, functDrumRackBusUp);
    ip->setButtonObj(interfaceButtonDown, buttonPress, functDrumRackBusDown);
    ip->setButtonObj(interfaceButtonDelete, buttonPress, functDrumRackBusDelete);
    ip->clearButton(interfaceButtonInsert);
    ip->setButtonObj(interfaceButtonShift, functDrumRackBusShift);
    ip->setButtonObj(interfaceButtonMiniMiddle, functDrumRackBusMiniMiddle);
    ip->setPadsGlobal(functDrumRackBusPads);
}

void cInstrumentEditor::showDrumRackPadsScreen()
{
    showTitleBar();
    refreshFramePosition();
    clearSoftKeyChrome();
    applyBarStyles();

    if (drumRackPadEnvPage)
    {
        display.setAddControlStyle(label[7], controlStyleSpecialFx2);
        display.setControlColors(label[7], display.getTheme().moreButtonColors);
        display.setControlValue(label[7], 1);

        labelDataMore.dotIndicator.count = 2;
        labelDataMore.dotIndicator.current = 1;
        display.setControlData(label[7], &labelDataMore);

        display.setControlText(label[0], "Attack");
        display.setControlText(label[1], "Decay");
        display.setControlText(label[2], "Sustain");
        display.setControlText(label[3], "Release");
        display.setControlText(label[4], "Tune");
        display.setControlText(label[5], "Finetune");
        display.setControlText(label[6], "");
        display.setControlText(label[7], "Back");

        display.setControlStyle(
          barControl[0],
          (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground));
        display.setControlStyle(
          barControl[1],
          (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground));
        display.setControlStyle(
          barControl[2],
          (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground));
        display.setControlStyle(
          barControl[3],
          (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground));
        display.setControlStyle(
          barControl[4],
          (controlStyleShow | controlStyleValueLeftRight_100_100 | controlStyleBackground));
        display.setControlStyle(
          barControl[5],
          (controlStyleShow | controlStyleValueLeftRight_100_100 | controlStyleBackground));

        display.setControlShow(barControl[0]);
        display.setControlShow(barControl[1]);
        display.setControlShow(barControl[2]);
        display.setControlShow(barControl[3]);
        display.setControlShow(barControl[4]);
        display.setControlShow(barControl[5]);

        showDrumRackPadAttack();
        showDrumRackPadDecay();
        showDrumRackPadSustain();
        showDrumRackPadRelease();
        showDrumRackPadTune();
        showDrumRackPadFineTune();

        for (uint8_t i = 0; i < 8; i++)
            display.refreshControl(label[i]);

        activateLabelsBorder();
    }
    else if (drumRackPadDetailPage)
    {
        display.setAddControlStyle(label[6], controlStyleSpecialFx2);
        display.setControlColors(label[6], display.getTheme().moreButtonColors);
        display.setControlValue(label[6], 1);
        display.setAddControlStyle(label[7], controlStyleSpecialFx2);
        display.setControlColors(label[7], display.getTheme().moreButtonColors);
        display.setControlValue(label[7], 1);

        labelDataMore.dotIndicator.count = 2;
        labelDataMore.dotIndicator.current = 0;
        display.setControlData(label[7], &labelDataMore);

        display.setControlText(label[0], "Sample");
        display.setControlText(label[1], "Choke");
        display.setControlText(label[2], "Volume");
        display.setControlText(label[3], "Panning");
        display.setControlText(label[4], "Delay");
        display.setControlText(label[5], "Reverb");
        display.setControlText(label[6], "Env");
        display.setControlText(label[7], "Back");

        display.setControlHide(barControl[0]);
        display.setControlHide(barControl[1]);
        display.setControlHide(barControl[6]);
        display.setControlHide(barControl[7]);

        display.setControlStyle(
          barControl[2],
          (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground));
        display.setControlStyle(
          barControl[3],
          (controlStyleShow | controlStyleValueLeftRight_100_100 | controlStyleBackground));
        display.setControlStyle(
          barControl[4],
          (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground));
        display.setControlStyle(
          barControl[5],
          (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground));

        display.setControlShow(barControl[2]);
        display.setControlShow(barControl[3]);
        display.setControlShow(barControl[4]);
        display.setControlShow(barControl[5]);

        showDrumRackPadSample();
        showDrumRackPadChoke();
        showDrumRackPadVolume();
        showDrumRackPadPanning();
        showDrumRackPadDelaySend();
        showDrumRackPadReverbSend();

        for (uint8_t i = 0; i < 8; i++)
            display.refreshControl(label[i]);

        activateLabelsBorder();
    }
    else
    {
        // Kit overview: pad grid + Start Note; encoder edits sample unless Start Note
        // is focused. Soft key 7 = Pad → pad detail.
        for (uint8_t i = 0; i < 6; i++)
        {
            display.setControlHide(label[i]);
            display.setControlHide(barControl[i]);
        }
        display.setControlHide(barControl[6]);
        display.setControlHide(barControl[7]);

        display.setControlText(label[6], "Start Note");
        display.setAddControlStyle(label[7], controlStyleSpecialFx2);
        display.setControlColors(label[7], display.getTheme().moreButtonColors);
        display.setControlValue(label[7], 1);
        display.setControlText(label[7], "Pad");
        display.setControlShow(label[6]);
        display.setControlShow(label[7]);

        // No soft-key focus by default so encoder edits sample.
        selectedPlace[mtInstEditModeDrumRackPads] = 1;

        showDrumRackPadsGrid();
        showDrumRackStartNote();

        display.refreshControl(label[6]);
        display.refreshControl(label[7]);
        activateLabelsBorder();
    }
}

void cInstrumentEditor::showDrumRackPadsGrid()
{
    strDrumRackInstrument* rack = editRack();
    const uint8_t selectedPad = rack != nullptr ? rack->rack.selectedPad : 0;
    const uint8_t firstNote = rack != nullptr ? rack->rack.firstNote : kDrumRackFirstNote;
    const uint8_t selectedGrid = drumRackPadToGridIdx(selectedPad);

    for (uint8_t pad = 0; pad < kDrumRackPadCount; pad++)
    {
        const uint8_t gridIdx = drumRackPadToGridIdx(pad);
        const uint8_t note = drumRackPadToNote(pad, firstNote);
        snprintf(drumRackPadGridText[gridIdx], kDrumRackPadGridStringLen, "%s", mtNotes[note]);
        drumRackPadGridTextPtrs[gridIdx] = drumRackPadGridText[gridIdx];

        const uint8_t sampleSlot =
          rack != nullptr ? rack->rack.pads[pad].sampleSlot : kDrumRackEmptySlot;
        if (sampleSlot == kDrumRackEmptySlot || sampleSlot >= kSampleInstrumentsCount)
            snprintf(drumRackPadGridSubText[gridIdx], kDrumRackPadGridStringLen, "---");
        else
            snprintf(
              drumRackPadGridSubText[gridIdx], kDrumRackPadGridStringLen, "%02u", sampleSlot + 1);
        drumRackPadGridSubTextPtrs[gridIdx] = drumRackPadGridSubText[gridIdx];
    }

    // Square cells + square pads so horizontal and vertical gaps match.
    // Soft-key columns 0–5 for the grid; Start Note / Pad occupy 6–7.
    // Pad size leaves room under each square for the sample slot label.
    constexpr int kGridSide = 360;
    constexpr int kPadSize = 58; // (360 - 5) / 4 = 88 cell → 30px gap each way
    constexpr int kGridAreaW = (800 / 8) * 6 - 2;
    constexpr int kGridAreaH = 394;
    constexpr int kGridX = 1 + (kGridAreaW - kGridSide) / 2;
    constexpr int kGridY = 29 + (kGridAreaH - kGridSide) / 2;

    imgGridData.columnsCount = 4;
    imgGridData.rowsCount = 4;
    imgGridData.imgW = kPadSize;
    imgGridData.imgH = kPadSize;
    imgGridData.showSelection = true;
    imgGridData.textInPad = true;
    imgGridData.selection = selectedGrid;
    imgGridData.textList = drumRackPadGridTextPtrs;
    imgGridData.subTextList = drumRackPadGridSubTextPtrs;
    imgGridData.imgList = nullptr;

    display.setControlPosition(imgGridControl, kGridX, kGridY);
    display.setControlSize(imgGridControl, kGridSide, kGridSide);
    display.setControlData(imgGridControl, &imgGridData);
    display.setControlValue(imgGridControl, selectedGrid);
    display.setControlShow(imgGridControl);
    display.refreshControl(imgGridControl);
}

void cInstrumentEditor::showDrumRackBusScreen()
{
    showTitleBar();
    refreshFramePosition();
    clearSoftKeyChrome();
    applyBarStyles();

    display.setControlText(label[0], "Filter Type");
    display.setControlText(label[1], "Cutoff");
    display.setControlText(label[2], "Resonance");
    display.setControlText(label[3], "Overdrive");
    display.setControlText(label[4], "Bit depth");
    display.setControlText(label[5], "Reverb send");
    display.setControlText(label[6], "Delay send");
    display.setControlText(label[7], "");
    display.setControlValue(label[7], 0);
    display.setControlHide(label[7]);

    display.setControlStyle(
      barControl[1], (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground));
    display.setControlStyle(
      barControl[2], (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground));
    display.setControlStyle(
      barControl[3], (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground));
    display.setControlStyle(
      barControl[4], (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground));
    display.setControlStyle(
      barControl[5], (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground));
    display.setControlStyle(
      barControl[6], (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground));

    display.setControlShow(barControl[1]);
    display.setControlShow(barControl[2]);
    display.setControlShow(barControl[3]);
    display.setControlShow(barControl[4]);
    display.setControlShow(barControl[5]);
    display.setControlShow(barControl[6]);

    showDrumRackBusFilterType();
    showDrumRackBusCutoff();
    showDrumRackBusResonance();
    showDrumRackBusOverdrive();
    showDrumRackBusBitDepth();
    showDrumRackBusReverbSend();
    showDrumRackBusDelaySend();

    for (uint8_t i = 0; i < 7; i++)
        display.refreshControl(label[i]);

    activateLabelsBorder();
}

void cInstrumentEditor::showDrumRackPadSample()
{
    strDrumRackPad* pad = DrumRack::editPad(mtProject.values.lastUsedInstrument);
    if (pad == nullptr || pad->sampleSlot == kDrumRackEmptySlot)
        sprintf(labelText[0], "---");
    else
        sprintf(labelText[0], "%02u", pad->sampleSlot + 1);
    display.setControlText2(label[0], labelText[0]);
    display.refreshControl(label[0]);
}

void cInstrumentEditor::showDrumRackStartNote()
{
    strDrumRackInstrument* rack = editRack();
    const uint8_t firstNote = rack != nullptr ? rack->rack.firstNote : kDrumRackFirstNote;
    sprintf(labelText[6], "%s", mtNotes[firstNote]);
    display.setControlText2(label[6], labelText[6]);
    display.refreshControl(label[6]);
}

void cInstrumentEditor::showDrumRackPadChoke()
{
    strDrumRackPad* pad = DrumRack::editPad(mtProject.values.lastUsedInstrument);
    const uint8_t choke = pad != nullptr ? pad->chokeGroup : 0;
    if (choke == 0)
        sprintf(labelText[1], "Off");
    else
        sprintf(labelText[1], "%u", choke);
    display.setControlText2(label[1], labelText[1]);
    display.refreshControl(label[1]);
}

void cInstrumentEditor::showDrumRackPadVolume()
{
    strDrumRackPad* pad = DrumRack::editPad(mtProject.values.lastUsedInstrument);
    const uint8_t volume = pad != nullptr ? pad->volume : DEFAULT_INSTRUMENT_VOLUME;
    float volumeInDecybels = calcInstrumentVolumeDecybelValue(volume);
    sprintf(labelText[2], "%0.02f dB", volumeInDecybels);
    display.setControlText2(label[2], labelText[2]);
    display.refreshControl(label[2]);
    display.setControlValue(barControl[2], volume);
    display.refreshControl(barControl[2]);
}

void cInstrumentEditor::showDrumRackPadPanning()
{
    strDrumRackPad* pad = DrumRack::editPad(mtProject.values.lastUsedInstrument);
    const int16_t panning = pad != nullptr ? pad->panning : 50;
    sprintf(labelText[3], "%d", panning - 50);
    display.setControlText2(label[3], labelText[3]);
    display.refreshControl(label[3]);
    display.setControlValue(barControl[3], panning * 2 - 100);
    display.refreshControl(barControl[3]);
}

void cInstrumentEditor::showDrumRackPadTune()
{
    strDrumRackPad* pad = DrumRack::editPad(mtProject.values.lastUsedInstrument);
    const int8_t tune = pad != nullptr ? pad->tune : 0;
    sprintf(labelText[4], "%d", tune);
    display.setControlText2(label[4], labelText[4]);
    display.refreshControl(label[4]);
    display.setControlValue(barControl[4], (tune * 100) / 24);
    display.refreshControl(barControl[4]);
}

void cInstrumentEditor::showDrumRackPadFineTune()
{
    strDrumRackPad* pad = DrumRack::editPad(mtProject.values.lastUsedInstrument);
    const int8_t fineTune = pad != nullptr ? pad->fineTune : 0;
    sprintf(labelText[5], "%d", fineTune);
    display.setControlText2(label[5], labelText[5]);
    display.refreshControl(label[5]);
    display.setControlValue(barControl[5], fineTune);
    display.refreshControl(barControl[5]);
}

void cInstrumentEditor::showDrumRackPadDelaySend()
{
    strDrumRackInstrument* rack = editRack();
    const uint8_t padIdx = selectedPadIdx();
    const uint8_t delaySend = rack != nullptr ? rack->rack.delaySend[padIdx] : 0;
    float sendInDecybels = calcInstrumentSendsDecybelValue(delaySend);
    sprintf(labelText[4], "%0.02f dB", sendInDecybels);
    display.setControlText2(label[4], labelText[4]);
    display.refreshControl(label[4]);
    display.setControlValue(barControl[4], delaySend);
    display.refreshControl(barControl[4]);
}

void cInstrumentEditor::showDrumRackPadReverbSend()
{
    strDrumRackInstrument* rack = editRack();
    const uint8_t padIdx = selectedPadIdx();
    const uint8_t reverbSend = rack != nullptr ? rack->rack.reverbSend[padIdx] : 0;
    float sendInDecybels = calcInstrumentSendsDecybelValue(reverbSend);
    sprintf(labelText[5], "%0.02f dB", sendInDecybels);
    display.setControlText2(label[5], labelText[5]);
    display.refreshControl(label[5]);
    display.setControlValue(barControl[5], reverbSend);
    display.refreshControl(barControl[5]);
}

void cInstrumentEditor::showDrumRackPadAttack()
{
    strDrumRackPad* pad = DrumRack::editPad(mtProject.values.lastUsedInstrument);
    const uint16_t attack = pad != nullptr ? pad->attack : 0;
    sprintf(labelText[0], "%.3fs", attack / 1000.0f);
    display.setControlText2(label[0], labelText[0]);
    display.refreshControl(label[0]);
    display.setControlValue(barControl[0],
                            attack > ENVELOPE_MICRO_RANGE
                              ? map<int>(attack, ENVELOPE_MICRO_RANGE, ATTACK_MAX, 50, 100)
                              : map<int>(attack, 0, ENVELOPE_MICRO_RANGE, 0, 50));
    display.refreshControl(barControl[0]);
}

void cInstrumentEditor::showDrumRackPadDecay()
{
    strDrumRackPad* pad = DrumRack::editPad(mtProject.values.lastUsedInstrument);
    const uint16_t decay = pad != nullptr ? pad->decay : 0;
    sprintf(labelText[1], "%.3fs", decay / 1000.0f);
    display.setControlText2(label[1], labelText[1]);
    display.refreshControl(label[1]);
    display.setControlValue(barControl[1],
                            decay > ENVELOPE_MICRO_RANGE
                              ? map<int>(decay, ENVELOPE_MICRO_RANGE, DECAY_MAX, 50, 100)
                              : map<int>(decay, 0, ENVELOPE_MICRO_RANGE, 0, 50));
    display.refreshControl(barControl[1]);
}

void cInstrumentEditor::showDrumRackPadSustain()
{
    strDrumRackPad* pad = DrumRack::editPad(mtProject.values.lastUsedInstrument);
    const float sustain = pad != nullptr ? pad->sustain : 1.0f;
    sprintf(labelText[2], "%.0f", sustain * 100.0f);
    display.setControlText2(label[2], labelText[2]);
    display.refreshControl(label[2]);
    display.setControlValue(barControl[2], (int)(sustain * 100.0f));
    display.refreshControl(barControl[2]);
}

void cInstrumentEditor::showDrumRackPadRelease()
{
    strDrumRackPad* pad = DrumRack::editPad(mtProject.values.lastUsedInstrument);
    const uint16_t release = pad != nullptr ? pad->release : 0;
    sprintf(labelText[3], "%.3fs", release / 1000.0f);
    display.setControlText2(label[3], labelText[3]);
    display.refreshControl(label[3]);
    display.setControlValue(barControl[3],
                            release > ENVELOPE_MICRO_RANGE
                              ? map<int>(release, ENVELOPE_MICRO_RANGE, RELEASE_MAX, 50, 100)
                              : map<int>(release, 0, ENVELOPE_MICRO_RANGE, 0, 50));
    display.refreshControl(barControl[3]);
}

void cInstrumentEditor::showDrumRackBusFilterType()
{
    strDrumRackInstrument* rack = editRack();
    uint8_t pos = 0;
    if (rack != nullptr && rack->filterEnable)
    {
        if (rack->filterType == lowPass)
            pos = 1;
        else if (rack->filterType == highPass)
            pos = 2;
        else
            pos = 3;
    }
    filterModeListPos = pos;
    display.setControlText2(label[0], kDrumRackFilterLabels[pos]);
    display.refreshControl(label[0]);
}

void cInstrumentEditor::showDrumRackBusCutoff()
{
    strDrumRackInstrument* rack = editRack();
    const float cutOff = rack != nullptr ? rack->cutOff : 1.0f;
    const bool filterEnable = rack != nullptr && rack->filterEnable;
    uint8_t temp_cutoff = (uint8_t)(cutOff * 100);
    sprintf(labelText[1], "%d", temp_cutoff);
    if (filterEnable)
    {
        display.setControlColors(label[1], display.getTheme().activeLabelsColors);
        display.setControlColors(barControl[1], localActiveBarColors);
    }
    else
    {
        display.setControlColors(label[1], display.getTheme().inactiveLabelsColors);
        display.setControlColors(barControl[1], localInactiveBarColors);
    }
    display.setControlText2(label[1], labelText[1]);
    display.refreshControl(label[1]);
    display.setControlValue(barControl[1], temp_cutoff);
    display.refreshControl(barControl[1]);
}

void cInstrumentEditor::showDrumRackBusResonance()
{
    strDrumRackInstrument* rack = editRack();
    const float resonance = rack != nullptr ? rack->resonance : 0.0f;
    const bool filterEnable = rack != nullptr && rack->filterEnable;
    uint8_t temp_resonance =
      (uint8_t)(((resonance - RESONANCE_MIN) / (RESONANCE_MAX - RESONANCE_MIN)) * 100);
    sprintf(labelText[2], "%d", temp_resonance);
    if (filterEnable)
    {
        display.setControlColors(label[2], display.getTheme().activeLabelsColors);
        display.setControlColors(barControl[2], localActiveBarColors);
    }
    else
    {
        display.setControlColors(label[2], display.getTheme().inactiveLabelsColors);
        display.setControlColors(barControl[2], localInactiveBarColors);
    }
    display.setControlText2(label[2], labelText[2]);
    display.refreshControl(label[2]);
    display.setControlValue(barControl[2], temp_resonance);
    display.refreshControl(barControl[2]);
}

void cInstrumentEditor::showDrumRackBusOverdrive()
{
    strDrumRackInstrument* rack = editRack();
    const uint8_t overdrive = rack != nullptr ? rack->overdrive : 0;
    sprintf(labelText[3], "%d", overdrive);
    display.setControlText2(label[3], labelText[3]);
    display.refreshControl(label[3]);
    display.setControlValue(barControl[3], overdrive);
    display.refreshControl(barControl[3]);
}

void cInstrumentEditor::showDrumRackBusBitDepth()
{
    strDrumRackInstrument* rack = editRack();
    const uint8_t bitDepth = rack != nullptr ? rack->bitDepth : 16;
    sprintf(labelText[4], "%d", bitDepth);
    display.setControlText2(label[4], labelText[4]);
    display.refreshControl(label[4]);
    display.setControlValue(barControl[4],
                            (uint8_t)map<int>(bitDepth, BIT_DEPTH_MIN, BIT_DEPTH_MAX, 0, 100));
    display.refreshControl(barControl[4]);
}

void cInstrumentEditor::showDrumRackBusReverbSend()
{
    strDrumRackInstrument* rack = editRack();
    const uint8_t reverbSend = rack != nullptr ? rack->reverbSend : 0;
    float sendInDecybels = calcInstrumentSendsDecybelValue(reverbSend);
    sprintf(labelText[5], "%0.02f dB", sendInDecybels);
    display.setControlText2(label[5], labelText[5]);
    display.refreshControl(label[5]);
    display.setControlValue(barControl[5], reverbSend);
    display.refreshControl(barControl[5]);
}

void cInstrumentEditor::showDrumRackBusDelaySend()
{
    strDrumRackInstrument* rack = editRack();
    const uint8_t delaySend = rack != nullptr ? rack->delaySend : 0;
    float sendInDecybels = calcInstrumentSendsDecybelValue(delaySend);
    sprintf(labelText[6], "%0.02f dB", sendInDecybels);
    display.setControlText2(label[6], labelText[6]);
    display.refreshControl(label[6]);
    display.setControlValue(barControl[6], delaySend);
    display.refreshControl(barControl[6]);
}

void cInstrumentEditor::changeDrumRackPadParamValue(int value, ValueChangeSource source)
{
    (void)source;
    strDrumRackPad* pad = DrumRack::editPad(mtProject.values.lastUsedInstrument);
    if (pad == nullptr)
        return;

    const uint8_t place = selectedPlace[mtInstEditModeDrumRackPads];

    if (drumRackPadEnvPage)
    {
        switch (place)
        {
            case 0:
            {
                int next = (int)pad->attack + value * 10;
                if (next < 0)
                    next = 0;
                if (next > ATTACK_MAX)
                    next = ATTACK_MAX;
                pad->attack = (uint16_t)next;
                showDrumRackPadAttack();
                break;
            }
            case 1:
            {
                int next = (int)pad->decay + value * 10;
                if (next < 0)
                    next = 0;
                if (next > DECAY_MAX)
                    next = DECAY_MAX;
                pad->decay = (uint16_t)next;
                showDrumRackPadDecay();
                break;
            }
            case 2:
            {
                float fVal = value * 0.01f;
                if (pad->sustain + fVal < 0)
                    pad->sustain = 0;
                else if (pad->sustain + fVal > 1.0f)
                    pad->sustain = 1.0f;
                else
                    pad->sustain += fVal;
                showDrumRackPadSustain();
                break;
            }
            case 3:
            {
                int next = (int)pad->release + value * 10;
                if (next < 0)
                    next = 0;
                if (next > RELEASE_MAX)
                    next = RELEASE_MAX;
                pad->release = (uint16_t)next;
                showDrumRackPadRelease();
                break;
            }
            case 4:
            {
                int step = value == 10 ? 12 : (value == -10 ? -12 : value);
                if (pad->tune + step < MIN_INSTRUMENT_TUNE)
                    pad->tune = MIN_INSTRUMENT_TUNE;
                else if (pad->tune + step > MAX_INSTRUMENT_TUNE)
                    pad->tune = MAX_INSTRUMENT_TUNE;
                else
                    pad->tune += step;
                showDrumRackPadTune();
                break;
            }
            case 5:
            {
                if (pad->fineTune + value < MIN_INSTRUMENT_FINETUNE)
                    pad->fineTune = MIN_INSTRUMENT_FINETUNE;
                else if (pad->fineTune + value > MAX_INSTRUMENT_FINETUNE)
                    pad->fineTune = MAX_INSTRUMENT_FINETUNE;
                else
                    pad->fineTune += value;
                showDrumRackPadFineTune();
                break;
            }
            default: break;
        }
    }
    else if (drumRackPadDetailPage)
    {
        // Detail soft keys: 0=Sample … 5=Reverb
        switch (place)
        {
            case 0:
                DrumRack::cyclePadSampleSlot(mtProject.values.lastUsedInstrument, value);
                showDrumRackPadSample();
                previewSelectedDrumRackPad();
                break;
            case 1:
            {
                int next = (int)pad->chokeGroup + value;
                if (next < 0)
                    next = 0;
                if (next > 8)
                    next = 8;
                pad->chokeGroup = (uint8_t)next;
                showDrumRackPadChoke();
                break;
            }
            case 2:
            {
                if (pad->volume + value < 0)
                    pad->volume = 0;
                else if (pad->volume + value > MAX_INSTRUMENT_VOLUME)
                    pad->volume = MAX_INSTRUMENT_VOLUME;
                else
                    pad->volume += value;
                showDrumRackPadVolume();
                break;
            }
            case 3:
            {
                if (pad->panning + value < PANNING_MIN)
                    pad->panning = PANNING_MIN;
                else if (pad->panning + value > PANNING_MAX)
                    pad->panning = PANNING_MAX;
                else
                    pad->panning += value;
                showDrumRackPadPanning();
                break;
            }
            case 4:
            {
                strDrumRackInstrument* rack = editRack();
                if (rack == nullptr)
                    break;
                const uint8_t padIdx = rack->rack.selectedPad;
                int next = (int)rack->rack.delaySend[padIdx] + value;
                next = clamp(next, 0, 100);
                rack->rack.delaySend[padIdx] = (uint8_t)next;
                showDrumRackPadDelaySend();
                refreshKitVoices(DELAY_SEND_MASK);
                break;
            }
            case 5:
            {
                strDrumRackInstrument* rack = editRack();
                if (rack == nullptr)
                    break;
                const uint8_t padIdx = rack->rack.selectedPad;
                int next = (int)rack->rack.reverbSend[padIdx] + value;
                next = clamp(next, 0, 100);
                rack->rack.reverbSend[padIdx] = (uint8_t)next;
                showDrumRackPadReverbSend();
                refreshKitVoices(REVERB_SEND_MASK);
                break;
            }
            default: break;
        }
    }
    else
    {
        // Overview: encoder edits Start Note only while that soft key is focused;
        // otherwise it always changes the selected pad's sample.
        if (place == 0)
        {
            strDrumRackInstrument* rack = editRack();
            if (rack == nullptr)
                return;
            const int maxFirst = 127 - (int)(kDrumRackPadCount - 1);
            int next = (int)rack->rack.firstNote + value;
            if (next < 0)
                next = 0;
            if (next > maxFirst)
                next = maxFirst;
            rack->rack.firstNote = (uint8_t)next;
            showDrumRackStartNote();
            showDrumRackPadsGrid();
        }
        else
        {
            DrumRack::cyclePadSampleSlot(mtProject.values.lastUsedInstrument, value);
            showDrumRackPadsGrid();
            previewSelectedDrumRackPad();
        }
    }

    markRackChanged();
}

void cInstrumentEditor::changeDrumRackBusParamValue(int value, ValueChangeSource source)
{
    strDrumRackInstrument* rack = editRack();
    if (rack == nullptr)
        return;

    bool invert = source != kEncoder || interfaceGlobals.invertValForList;
    int delta = invert ? -value : value;

    switch (selectedPlace[mtInstEditModeDrumRackBus])
    {
        case 0:
        {
            int pos = 0;
            if (rack->filterEnable)
            {
                if (rack->filterType == lowPass)
                    pos = 1;
                else if (rack->filterType == highPass)
                    pos = 2;
                else
                    pos = 3;
            }
            pos = clamp(pos + delta, 0, 3);
            if (pos == 0)
            {
                rack->filterEnable = 0;
            }
            else
            {
                rack->filterEnable = 1;
                rack->filterType = (pos == 1) ? lowPass : (pos == 2) ? highPass : bandPass;
            }
            showDrumRackBusFilterType();
            showDrumRackBusCutoff();
            showDrumRackBusResonance();
            refreshKitFilterRouting();
            break;
        }
        case 1:
        {
            int next = (int)(rack->cutOff * 100.0f) + value;
            next = clamp(next, 0, 100);
            rack->cutOff = next / 100.0f;
            showDrumRackBusCutoff();
            refreshKitVoices(CUTOFF_MASK);
            break;
        }
        case 2:
        {
            float step = (RESONANCE_MAX - RESONANCE_MIN) / 100.0f;
            rack->resonance = clamp(rack->resonance + value * step, RESONANCE_MIN, RESONANCE_MAX);
            showDrumRackBusResonance();
            refreshKitVoices(RESONANCE_MASK);
            break;
        }
        case 3:
            rack->overdrive = (uint8_t)clamp((int)rack->overdrive + value, 0, 100);
            showDrumRackBusOverdrive();
            refreshKitVoices(OVERDRIVE_MASK);
            break;
        case 4:
            rack->bitDepth =
              (uint8_t)clamp((int)rack->bitDepth + value, (int)BIT_DEPTH_MIN, (int)BIT_DEPTH_MAX);
            showDrumRackBusBitDepth();
            refreshKitVoices(BIT_DEPTH_MASK);
            break;
        case 5:
            rack->reverbSend = (uint8_t)clamp((int)rack->reverbSend + value, 0, 100);
            showDrumRackBusReverbSend();
            refreshKitVoices(REVERB_SEND_MASK);
            break;
        case 6:
            rack->delaySend = (uint8_t)clamp((int)rack->delaySend + value, 0, 100);
            showDrumRackBusDelaySend();
            refreshKitVoices(DELAY_SEND_MASK);
            break;
        default: break;
    }
    markRackChanged();
}

static void unfocusDrumRackOverviewSoftKeys()
{
    // selectedPlace >= placesCount hides the soft-key frame; encoder then edits sample.
    if (!IE->drumRackPadEnvPage && !IE->drumRackPadDetailPage)
        IE->selectedPlace[mtInstEditModeDrumRackPads] = 1;
}

static void refreshDrumRackPadEditors()
{
    if (IE->drumRackPadEnvPage)
    {
        IE->showDrumRackPadAttack();
        IE->showDrumRackPadDecay();
        IE->showDrumRackPadSustain();
        IE->showDrumRackPadRelease();
        IE->showDrumRackPadTune();
        IE->showDrumRackPadFineTune();
    }
    else if (IE->drumRackPadDetailPage)
    {
        IE->showDrumRackPadSample();
        IE->showDrumRackPadChoke();
        IE->showDrumRackPadVolume();
        IE->showDrumRackPadPanning();
        IE->showDrumRackPadDelaySend();
        IE->showDrumRackPadReverbSend();
    }
    else
    {
        IE->showDrumRackPadsGrid();
        IE->showDrumRackStartNote();
    }
}

// direction: 0=up, 1=right, 2=down, 3=left
// Pad row 0 is the bottom row (Start Note); up moves to higher pad indices.
static void navigateDrumRackPadsGrid(int direction)
{
    strDrumRackInstrument* rack = editRack();
    if (rack == nullptr)
        return;

    constexpr uint8_t kCols = 4;
    constexpr uint8_t kRows = 4;
    const uint8_t pad = rack->rack.selectedPad;
    uint8_t row = pad / kCols;
    uint8_t col = pad % kCols;

    switch (direction)
    {
        case 0: row = (uint8_t)((row + 1) % kRows); break;
        case 1: col = (uint8_t)((col + 1) % kCols); break;
        case 2: row = (uint8_t)((row + kRows - 1) % kRows); break;
        case 3: col = (uint8_t)((col + kCols - 1) % kCols); break;
        default: return;
    }

    DrumRack::selectPad(mtProject.values.lastUsedInstrument, (uint8_t)(row * kCols + col));
    unfocusDrumRackOverviewSoftKeys();
    refreshDrumRackPadEditors();
    IE->activateLabelsBorder();
    markRackChanged();
}

static void functDrumRackPadsEncoder(int16_t value)
{
    IE->changeDrumRackPadParamValue(value, IE->kEncoder);
}

static void functDrumRackPadsUp()
{
    if (IE->drumRackPadEnvPage || IE->drumRackPadDetailPage)
        IE->changeDrumRackPadParamValue(1);
    else
        navigateDrumRackPadsGrid(0);
}

static void functDrumRackPadsDown()
{
    if (IE->drumRackPadEnvPage || IE->drumRackPadDetailPage)
        IE->changeDrumRackPadParamValue(-1);
    else
        navigateDrumRackPadsGrid(2);
}

static void functDrumRackPadsLeft()
{
    if (IE->drumRackPadEnvPage || IE->drumRackPadDetailPage)
    {
        if (IE->selectedPlace[mtInstEditModeDrumRackPads] > 0)
            IE->selectedPlace[mtInstEditModeDrumRackPads]--;
        IE->activateLabelsBorder();
        return;
    }
    navigateDrumRackPadsGrid(3);
}

static void functDrumRackPadsRight()
{
    if (IE->drumRackPadEnvPage || IE->drumRackPadDetailPage)
    {
        if (IE->frameData.placesCount > 0 &&
            IE->selectedPlace[mtInstEditModeDrumRackPads] < IE->frameData.placesCount - 1)
            IE->selectedPlace[mtInstEditModeDrumRackPads]++;
        IE->activateLabelsBorder();
        return;
    }
    navigateDrumRackPadsGrid(1);
}

static void functDrumRackPadsButtons(uint8_t button, uint8_t state)
{
    if (state != buttonPress || IE->mode != mtInstEditModeDrumRackPads)
        return;

    if (IE->drumRackPadEnvPage)
    {
        if (button > 5)
            return;
        IE->selectedPlace[mtInstEditModeDrumRackPads] = button;
    }
    else if (IE->drumRackPadDetailPage)
    {
        if (button > 5)
            return;
        IE->selectedPlace[mtInstEditModeDrumRackPads] = button;
    }
    else
    {
        // Overview: soft key 6 focuses Start Note for encoder edits.
        if (button != 6)
            return;
        IE->selectedPlace[mtInstEditModeDrumRackPads] = 0;
    }

    IE->activateLabelsBorder();
}

static void functDrumRackPadsMoreBack()
{
    if (IE->mode != mtInstEditModeDrumRackPads)
        return;

    if (IE->drumRackPadEnvPage)
    {
        IE->drumRackPadEnvPage = false;
        IE->drumRackPadDetailPage = true;
    }
    else if (IE->drumRackPadDetailPage)
    {
        IE->drumRackPadDetailPage = false;
        unfocusDrumRackOverviewSoftKeys();
    }
    else
    {
        // Overview Pad soft key → pad detail.
        IE->drumRackPadDetailPage = true;
        IE->selectedPlace[mtInstEditModeDrumRackPads] = 0;
    }

    IE->showDrumRackPadsScreen();
    IE->setDrumRackPadsScreenFunct();
}

static void functDrumRackPadsEnvEnter()
{
    if (IE->mode != mtInstEditModeDrumRackPads || !IE->drumRackPadDetailPage)
        return;

    IE->drumRackPadEnvPage = true;
    IE->selectedPlace[mtInstEditModeDrumRackPads] = 0;
    IE->showDrumRackPadsScreen();
    IE->setDrumRackPadsScreenFunct();
}

static void functDrumRackPadsDelete()
{
    strDrumRackPad* pad = DrumRack::editPad(mtProject.values.lastUsedInstrument);
    if (pad == nullptr)
        return;

    if (IE->drumRackPadEnvPage)
    {
        switch (IE->selectedPlace[mtInstEditModeDrumRackPads])
        {
            case 0:
                pad->attack = 10;
                IE->showDrumRackPadAttack();
                break;
            case 1:
                pad->decay = 3000;
                IE->showDrumRackPadDecay();
                break;
            case 2: pad->sustain = 1.0f; IE->showDrumRackPadSustain(); break;
            case 3: pad->release = 1000; IE->showDrumRackPadRelease(); break;
            case 4:
                pad->tune = 0;
                IE->showDrumRackPadTune();
                break;
            case 5:
                pad->fineTune = 0;
                IE->showDrumRackPadFineTune();
                break;
            default: break;
        }
    }
    else if (IE->drumRackPadDetailPage)
    {
        switch (IE->selectedPlace[mtInstEditModeDrumRackPads])
        {
            case 0:
                pad->sampleSlot = kDrumRackEmptySlot;
                IE->showDrumRackPadSample();
                break;
            case 1:
                pad->chokeGroup = 0;
                IE->showDrumRackPadChoke();
                break;
            case 2:
                pad->volume = DEFAULT_INSTRUMENT_VOLUME;
                IE->showDrumRackPadVolume();
                break;
            case 3:
                pad->panning = 50;
                IE->showDrumRackPadPanning();
                break;
            case 4:
            {
                strDrumRackInstrument* rack = editRack();
                if (rack != nullptr)
                {
                    rack->rack.delaySend[rack->rack.selectedPad] = 0;
                    IE->showDrumRackPadDelaySend();
                    refreshKitVoices(DELAY_SEND_MASK);
                }
                break;
            }
            case 5:
            {
                strDrumRackInstrument* rack = editRack();
                if (rack != nullptr)
                {
                    rack->rack.reverbSend[rack->rack.selectedPad] = 0;
                    IE->showDrumRackPadReverbSend();
                    refreshKitVoices(REVERB_SEND_MASK);
                }
                break;
            }
            default: break;
        }
    }
    else
    {
        if (IE->selectedPlace[mtInstEditModeDrumRackPads] == 0)
        {
            strDrumRackInstrument* rack = editRack();
            if (rack != nullptr)
            {
                rack->rack.firstNote = kDrumRackFirstNote;
                IE->showDrumRackStartNote();
                IE->showDrumRackPadsGrid();
            }
        }
        else
        {
            pad->sampleSlot = kDrumRackEmptySlot;
            IE->showDrumRackPadsGrid();
        }
    }
    markRackChanged();
}

static void functDrumRackPads(uint8_t pad, uint8_t state, int16_t)
{
    previewDrumRackPad(pad, state);
    if (state == buttonPress && IE->mode == mtInstEditModeDrumRackPads)
    {
        unfocusDrumRackOverviewSoftKeys();
        refreshDrumRackPadEditors();
        IE->activateLabelsBorder();
        markRackChanged();
    }
}

static void functDrumRackPadsShift(uint8_t state)
{
    if (state == buttonPress)
    {
        if (mtPopups.isNoteKeyboardPopupActive())
            mtPopups.toggleNoteKeyboardPopup(InterfacePopups::noteKeyboardOption::kShowEditButtons);
    }
    else if (state == buttonRelease)
    {
        if (mtPopups.isNoteKeyboardPopupActive())
            mtPopups.toggleNoteKeyboardPopup(InterfacePopups::noteKeyboardOption::kForceShow);
    }
}

static void functDrumRackPadsMiniMiddle(uint8_t state)
{
    if (state != buttonPress || IE->mode != mtInstEditModeDrumRackPads || IE->drumRackPadEnvPage)
        return;

    // Enter mirrors Pad / Back between overview and pad detail.
    functDrumRackPadsMoreBack();
}

static void functDrumRackBusEncoder(int16_t value)
{
    IE->changeDrumRackBusParamValue(value, IE->kEncoder);
}

static void functDrumRackBusUp()
{
    IE->changeDrumRackBusParamValue(1);
}

static void functDrumRackBusDown()
{
    IE->changeDrumRackBusParamValue(-1);
}

static void functDrumRackBusLeft()
{
    if (IE->selectedPlace[mtInstEditModeDrumRackBus] > 0)
        IE->selectedPlace[mtInstEditModeDrumRackBus]--;
    IE->activateLabelsBorder();
}

static void functDrumRackBusRight()
{
    if (IE->selectedPlace[mtInstEditModeDrumRackBus] < IE->frameData.placesCount - 1)
        IE->selectedPlace[mtInstEditModeDrumRackBus]++;
    IE->activateLabelsBorder();
}

static void functDrumRackBusButtons(uint8_t button, uint8_t state)
{
    if (state != buttonPress || IE->mode != mtInstEditModeDrumRackBus)
        return;
    if (button > 6)
        return;

    if (button == 0 && IE->selectedPlace[mtInstEditModeDrumRackBus] == 0)
    {
        IE->rollListOver = true;
        IE->changeDrumRackBusParamValue(1);
        IE->rollListOver = false;
    }

    IE->selectedPlace[mtInstEditModeDrumRackBus] = button;
    IE->activateLabelsBorder();
}

static void functDrumRackBusDelete()
{
    strDrumRackInstrument* rack = editRack();
    if (rack == nullptr)
        return;

    switch (IE->selectedPlace[mtInstEditModeDrumRackBus])
    {
        case 0:
            rack->filterEnable = 0;
            rack->filterType = lowPass;
            IE->showDrumRackBusFilterType();
            IE->showDrumRackBusCutoff();
            IE->showDrumRackBusResonance();
            refreshKitFilterRouting();
            break;
        case 1:
            rack->cutOff = 1.0f;
            IE->showDrumRackBusCutoff();
            refreshKitVoices(CUTOFF_MASK);
            break;
        case 2:
            rack->resonance = 0.0f;
            IE->showDrumRackBusResonance();
            refreshKitVoices(RESONANCE_MASK);
            break;
        case 3:
            rack->overdrive = 0;
            IE->showDrumRackBusOverdrive();
            refreshKitVoices(OVERDRIVE_MASK);
            break;
        case 4:
            rack->bitDepth = 16;
            IE->showDrumRackBusBitDepth();
            refreshKitVoices(BIT_DEPTH_MASK);
            break;
        case 5:
            rack->reverbSend = 0;
            IE->showDrumRackBusReverbSend();
            refreshKitVoices(REVERB_SEND_MASK);
            break;
        case 6:
            rack->delaySend = 0;
            IE->showDrumRackBusDelaySend();
            refreshKitVoices(DELAY_SEND_MASK);
            break;
        default: break;
    }
    markRackChanged();
}

static void functDrumRackBusPads(uint8_t pad, uint8_t state, int16_t)
{
    previewDrumRackPad(pad, state);
}

static void functDrumRackBusShift(uint8_t state)
{
    functDrumRackPadsShift(state);
}

static void functDrumRackBusMiniMiddle(uint8_t state)
{
    previewDrumRackPad(12, state);
}
