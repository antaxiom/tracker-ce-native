#ifndef FWREF_DISPLAY_H_
#define FWREF_DISPLAY_H_

// Mark the real core/libs/NewDisplay/display.h as already handled. The control
// sources live next to that header, so their quoted "#include \"display.h\""
// always resolves to the real file; this guard makes that include a no-op.
#ifndef LIBS_NEWDISPLAY_DISPLAY_H_
#define LIBS_NEWDISPLAY_DISPLAY_H_
#endif
//
// The firmware control sources (bar.cpp, label.cpp, list.cpp, frame.cpp,
// bgLabel.cpp) include "display.h" only for the global `cDisplay display`
// object and its createControl<T>()/getTheme() surface. Everything that
// actually draws lives in the unmodified control .cpp files.
//
// This shim deliberately does not replace any drawing, layout, colour or font
// logic. It only provides host allocation of controls, the theme pointer and
// the reverse-creation-order draw loop from cDisplay::step3().

#include <stdint.h>

#include "config.h"
#include "Themes.h"
#include "displayControls.h"
#include "displayStructs.h"
// The real display.h reaches mtStructs.h through sliceControl.h; cSongPlayer
// uses kAllTracks from it (songPlayer.cpp:17, 792).
#include "mtStructs.h"

class cDisplay
{
  public:
    cDisplay();

    template<typename controlClass>
    [[gnu::noinline]] hControl createControl(strControlProperties* properties)
    {
        int16_t slot = 0;
        while (slot < controlsCount && controlsTable[slot] != nullptr)
            slot++;

        if (slot >= controlsCount)
            return -1;

        if (((properties->style >> 8) & 15) > displayFontCount)
            properties->style &= ~(15u << 8);

        controlsTable[slot] = new controlClass(properties);
        return static_cast<hControl>(slot);
    }

    Theme& getTheme() { return *((Theme*)&themeStorage); }

    cDisplayControl* control(int handle);
    void showControl(int handle);
    void hideControl(int handle);
    void refreshControl(int handle) { showControl(handle); }
    void synchronizeRefresh() {}

    // Firmware editor aliases (gMasterParams/gConfigEdit/gPerformance).
    void setControlShow(int handle) { showControl(handle); }
    void setControlHide(int handle) { hideControl(handle); }
    void destroyControl(int handle);
    void resetControlQueue() {}
    void setControlDefaultColors(int handle, const uint32_t* colors)
    {
        setControlColors(handle, colors);
    }

    // Mirrors cDisplay::step3(): iterate the control table from the last slot
    // down to slot 0, so earlier-created controls are appended (drawn) last.
    void renderAll();

    void destroyAll();

    void setControlPosition(int handle, int16_t x, int16_t y);
    void setControlSize(int handle, int16_t w, int16_t h);
    void setControlStyle(int handle, uint32_t style);
    void setControlStyle2(int handle, uint32_t style);
    void setControlText(int handle, const char* text);
    void setControlText2(int handle, const char* text);
    void setControlValue(int handle, int value);
    void setControlColors(int handle, const uint32_t* colors);
    void setControlData(int handle, void* data);
    void setAddControlStyle(int handle, uint32_t style);
    void setRemoveControlStyle(int handle, uint32_t style);

    DisplayDriver* getDisplayDriver() { return nullptr; }

  private:
    cDisplayControl* controlsTable[controlsCount] = { nullptr };
    Theme themeStorage;
};

extern uint32_t ImageViewerJpegAddress;
extern cDisplay display;

#endif // FWREF_DISPLAY_H_
