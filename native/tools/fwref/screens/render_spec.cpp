#include "render_spec.h"

#include "audit.h"

#include "commonControls.h"
#include "display.h"
#include "fwref_internal.h"
#include "scenario_util.h"
#include "spec_controls.h"

#include <algorithm>
#include <deque>
#include <vector>

namespace fwref
{
namespace
{

std::deque<std::vector<uint32_t>> gColorArenas;
std::deque<std::string> gLabelStrings;

void resetArenas()
{
    // Only safe after display.destroyAll().
    gColorArenas.clear();
    gLabelStrings.clear();
    resetSpecArenas();
}

const uint32_t* colorTable(const SpecEntry& e)
{
    if (!e.hasColors)
        return nullptr;
    gColorArenas.emplace_back(e.colors);
    return gColorArenas.back().data();
}

std::string substituteChromeText(const std::string& text, const ScreenSpec& spec,
                                 const Scenario& scenario)
{
    if (text == "@title")
        return spec.title;
    if (text == "@rightLabel")
        return resolveText(spec.rightLabel, scenario);
    return resolveText(text, scenario);
}

} // namespace

bool renderSpec(trakr::emulator::FT812DisplayEmulator& emu,
                const ScreenSpec& spec,
                const std::vector<SpecEntry>& chrome,
                const Scenario& scenario)
{
    resetAudit();
    initFonts();
    setTarget(&emu);

    emu.clear(trakr::emulator::ColorRGB::fromHex(0x000000));
    emu.resetClip();
    emu.setBlendMode(trakr::emulator::BlendMode::SrcAlphaOneMinusSrcAlpha);

    display.destroyAll();
    resetArenas();

    std::vector<SpecEntry> body = spec.entries;
    std::vector<SpecEntry> chromeEntries;
    chromeEntries.reserve(chrome.size());
    for (const auto& c : chrome)
    {
        // TitleBar::show() defaults showRightLabel=false (titleBar.h:18).
        if (c.text == "@rightLabel" && spec.rightLabel.empty())
            continue;
        chromeEntries.push_back(c);
    }

    auto byOrder = [](const SpecEntry& a, const SpecEntry& b) { return a.order < b.order; };
    std::vector<SpecEntry> merged;
    merged.reserve(body.size() + chromeEntries.size());
    if (spec.chromeOnTop)
    {
        // Chrome controls are created before the screen body in the firmware,
        // so they occupy the lowest slots and cDisplay::renderAll() paints them
        // last (on top). Song's vertical track names rely on this clipping.
        std::stable_sort(chromeEntries.begin(), chromeEntries.end(), byOrder);
        std::stable_sort(body.begin(), body.end(), byOrder);
        merged.insert(merged.end(), chromeEntries.begin(), chromeEntries.end());
        merged.insert(merged.end(), body.begin(), body.end());
    }
    else
    {
        merged.insert(merged.end(), body.begin(), body.end());
        merged.insert(merged.end(), chromeEntries.begin(), chromeEntries.end());
        std::stable_sort(merged.begin(), merged.end(), byOrder);
    }

    for (const SpecEntry& e : merged)
    {
        bool ok = true;
        int x = resolveInt(e.x, scenario, ok);
        int y = resolveInt(e.y, scenario, ok);
        int w = resolveInt(e.w, scenario, ok);
        int h = resolveInt(e.h, scenario, ok);
        if (!ok)
            return false;

        uint32_t style = 0;
        if (!styleMaskFromNamesChecked(e.style, style))
            return false;
        uint32_t style2 = 0;
        if (!e.style2.empty() && !styleMaskFromNamesChecked(e.style2, style2))
            return false;

        strControlProperties prop;
        prop.x = static_cast<uint16_t>(std::max(0, x));
        prop.y = static_cast<uint16_t>(std::max(0, y));
        prop.w = static_cast<uint16_t>(std::max(0, w));
        prop.h = static_cast<uint16_t>(std::max(0, h));
        if (e.hidden)
            style &= ~static_cast<uint32_t>(controlStyleShow);
        prop.style = style;
        prop.colors = colorTable(e);

        hControl handle = createSpecControl(e, prop, scenario, ok);
        if (!ok)
            return false;
        cDisplayControl* ctl = display.control(handle);
        if (ctl == nullptr)
            continue;

        if (!e.style2.empty() && e.control == "cLabel")
            static_cast<cLabel*>(ctl)->setStyle2(style2);

        if (e.control == "cLabel")
        {
            auto* label = static_cast<cLabel*>(ctl);
            std::string text;
            if (!e.textFormat.empty())
                text = formatLabelValue(e.textFormat, scenario);
            else
                text = substituteChromeText(e.text, spec, scenario);
            gLabelStrings.push_back(text);
            label->setText(gLabelStrings.back().c_str());

            std::string text2;
            if (!e.text2Format.empty())
                text2 = formatLabelValue(e.text2Format, scenario, e.text2Arg);
            else
                text2 = substituteChromeText(e.text2, spec, scenario);
            gLabelStrings.push_back(text2);
            label->setText2(gLabelStrings.back().c_str());
        }

        if (!e.barValue.empty())
        {
            display.setControlValue(handle, barValueFor(e.barValue, e.barArg, scenario));
        }
        else if (e.hasValue)
        {
            int value = resolveInt(e.value, scenario, ok);
            if (!ok)
                return false;
            display.setControlValue(handle, value);
        }

        if (!e.hidden)
            display.showControl(handle);
    }

    display.renderAll();

    if (strict() && auditFailed())
    {
        printAuditReport(spec.title.empty() ? "renderSpec" : spec.title.c_str());
        return false;
    }
    return true;
}

} // namespace fwref
