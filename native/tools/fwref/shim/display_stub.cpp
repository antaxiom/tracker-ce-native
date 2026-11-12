#include "display.h"
#include "commonControls.h"

#include <string.h>

uint32_t ImageViewerJpegAddress = 350000;
cDisplay display;

// Bitmap table is not used by the Instrument/Master scenarios. The array must
// exist because label.cpp references it. Indexing is guarded by
// displayBitmapsCount in the control code.
const strBitmap bitmaps[displayBitmapsCount] = {};

cDisplay::cDisplay()
{
    themeStorage = themeDefault;
}

cDisplayControl* cDisplay::control(int handle)
{
    if (handle < 0 || handle >= controlsCount)
        return nullptr;
    return controlsTable[handle];
}

void cDisplay::showControl(int handle)
{
    if (handle < 0 || handle >= controlsCount)
        return;
    if (controlsTable[handle] != nullptr)
        controlsTable[handle]->setStyle(controlsTable[handle]->style | controlStyleShow);
}

void cDisplay::hideControl(int handle)
{
    if (handle < 0 || handle >= controlsCount)
        return;
    if (controlsTable[handle] != nullptr)
        controlsTable[handle]->setStyle(controlsTable[handle]->style & ~controlStyleShow);
}

void cDisplay::destroyControl(int handle)
{
    if (handle < 0 || handle >= controlsCount)
        return;
    delete controlsTable[handle];
    controlsTable[handle] = nullptr;
}

void cDisplay::renderAll()
{
    // Mirrors cDisplay::step3(): high slot first, slot 0 last (slot 0 ends up
    // on top because APPEND executes the lists in the order they are queued).
    // Each control contributes its stages in order (display.cpp:518-538); the
    // multi-stage controls (cTracker) use update2..update5 for their columns.
    for (int i = controlsCount - 1; i >= 0; --i)
    {
        cDisplayControl* p = controlsTable[i];
        if (p == nullptr)
            continue;
        if (p->style & controlStyleShow)
        {
            // Only the stages the control declares are appended (display.cpp:572).
            for (uint8_t stage = 0; stage < p->stagesCount; ++stage)
            {
                switch (stage)
                {
                    case 0: p->update(); break;
                    case 1: p->update2(); break;
                    case 2: p->update3(); break;
                    case 3: p->update4(); break;
                    case 4: p->update5(); break;
                    default: break;
                }
            }
        }
    }
}

void cDisplay::destroyAll()
{
    for (int i = 0; i < controlsCount; ++i)
    {
        delete controlsTable[i];
        controlsTable[i] = nullptr;
    }
}

void cDisplay::setControlPosition(int handle, int16_t x, int16_t y)
{
    if (handle < 0 || handle >= controlsCount)
        return;
    cDisplayControl* p = controlsTable[handle];
    if (p == nullptr)
        return;
    if (x >= 0)
        p->posX = static_cast<uint16_t>(x);
    if (y >= 0)
        p->posY = static_cast<uint16_t>(y);
}

void cDisplay::setControlSize(int handle, int16_t w, int16_t h)
{
    if (handle < 0 || handle >= controlsCount)
        return;
    cDisplayControl* p = controlsTable[handle];
    if (p == nullptr)
        return;
    if (w >= 0)
        p->width = static_cast<uint16_t>(w);
    if (h >= 0)
        p->height = static_cast<uint16_t>(h);
}

void cDisplay::setControlStyle(int handle, uint32_t style)
{
    if (handle < 0 || handle >= controlsCount)
        return;
    if (controlsTable[handle] != nullptr)
        controlsTable[handle]->setStyle(style);
}

void cDisplay::setControlStyle2(int handle, uint32_t style)
{
    // Only cLabel implements setStyle2 in the controls used by these screens.
    if (handle < 0 || handle >= controlsCount)
        return;
    cDisplayControl* p = controlsTable[handle];
    if (p != nullptr && p->id == controlIdLabel)
        static_cast<cLabel*>(p)->setStyle2(style);
}

void cDisplay::setControlText(int handle, const char* text)
{
    if (handle < 0 || handle >= controlsCount)
        return;
    if (controlsTable[handle] != nullptr)
        controlsTable[handle]->setText(text);
}

void cDisplay::setControlText2(int handle, const char* text)
{
    if (handle < 0 || handle >= controlsCount)
        return;
    cDisplayControl* p = controlsTable[handle];
    if (p != nullptr && p->id == controlIdLabel)
        static_cast<cLabel*>(p)->setText2(text);
}

void cDisplay::setControlValue(int handle, int value)
{
    if (handle < 0 || handle >= controlsCount)
        return;
    if (controlsTable[handle] != nullptr)
        controlsTable[handle]->setValue(value);
}

void cDisplay::setControlColors(int handle, const uint32_t* colors)
{
    if (handle < 0 || handle >= controlsCount)
        return;
    if (controlsTable[handle] != nullptr)
        controlsTable[handle]->setColors(colors);
}

void cDisplay::setControlData(int handle, void* data)
{
    if (handle < 0 || handle >= controlsCount)
        return;
    if (controlsTable[handle] != nullptr)
        controlsTable[handle]->setData(data);
}

void cDisplay::setAddControlStyle(int handle, uint32_t style)
{
    if (handle < 0 || handle >= controlsCount)
        return;
    if (controlsTable[handle] != nullptr)
        controlsTable[handle]->setStyle(controlsTable[handle]->style | style);
}

void cDisplay::setRemoveControlStyle(int handle, uint32_t style)
{
    if (handle < 0 || handle >= controlsCount)
        return;
    if (controlsTable[handle] != nullptr)
        controlsTable[handle]->setStyle(controlsTable[handle]->style & ~style);
}
