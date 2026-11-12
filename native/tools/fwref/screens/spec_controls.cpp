#include "spec_controls.h"

#include "barControl.h"
#include "commonControls.h"
#include "gridEditorControl.h"
#include "horizontalBarControl.h"
#include "list.h"
#include "multiLabelControl.h"
#include "paramValueList.h"
#include "scenario_util.h"
#include "textBoxControl.h"
#include "trackerControl.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>

namespace fwref
{
namespace
{

std::deque<strList> gLists;
std::deque<std::vector<char*>> gListRows;
std::deque<strLabelData> gLabelData;
std::deque<strFrameData> gFrameData;
std::deque<std::array<uint16_t, 4>> gPlaceRows;
std::deque<strBarData> gBarData;
std::deque<strTrackerPattern> gTrackerData;
std::deque<strMultiLabelData> gMultiData;
std::deque<strParamValueList> gParamLists;
std::deque<std::vector<char*>> gParamParams;
std::deque<std::vector<char*>> gParamValues;
std::deque<strPadNamesGridEditor> gGridNames;
std::deque<std::vector<char*>> gGridNotePtrs;
std::deque<std::vector<std::array<char, 5>>> gGridFinetunes;
std::deque<std::string> gStrings;

// Firmware row content for an empty pattern (fPatternEdit.cpp:718-790):
// note "---", instrument "--", fx "----". Only the 16 visible rows are stored
// by cTracker; row numbers/lengths are derived by the control itself.
void fillEmptyTrackerPattern(strTrackerPattern& tp, const PatternScenario& ps)
{
    for (auto& track : tp.track)
    {
        for (auto& row : track.row)
        {
            row.note[0] = '-';
            row.note[1] = '-';
            row.note[2] = '-';
            row.note[3] = 0;
            row.note[4] = 0;
            row.instr[0] = '-';
            row.instr[1] = '-';
            row.instr[2] = 0;
            row.instr[3] = 0;
            for (int f = 0; f < 2; ++f)
            {
                row.fx[f][0] = '-';
                row.fx[f][1] = '-';
                row.fx[f][2] = '-';
                row.fx[f][3] = '-';
                row.fx[f][4] = 0;
            }
        }
    }
    for (uint8_t t = 0; t < strTrackerPattern::kTrackCount; ++t)
    {
        tp.tracksNames[t] = ps.trackNames[t].c_str();
        tp.inactive[t] = 0;
    }

    tp.actualTrack = static_cast<int8_t>(ps.actualTrack);
    tp.actualStep = static_cast<int16_t>(ps.actualStep);
    tp.middleVisibleStep = static_cast<int16_t>(ps.actualStep);
    tp.patternLength = static_cast<int16_t>(ps.patternLength);
    tp.firstVisibleTrack = static_cast<int8_t>(ps.firstVisibleTrack);
    tp.stepDevider = static_cast<uint8_t>(ps.stepDevider);
    tp.showPlayhead = ps.showPlayhead;
    tp.playheadPosition = static_cast<int16_t>(ps.playheadPosition);
    tp.playheadStartTrack = 0;
    tp.playheadEndTrack = 7;
    tp.selectState = static_cast<int8_t>(ps.selectState);
    tp.selectedParam = static_cast<int8_t>(ps.selectedParam);
    tp.selectColor = static_cast<uint8_t>(ps.selectColor);
    tp.selectStartTrack = 0;
    tp.selectStartStep = 0;
    tp.selectEndTrack = 0;
    tp.selectEndStep = 0;
    tp.selectColumn = 0;
    tp.popupMode = 0;
    tp.showTrackNames = true;
}

const char* keepString(const std::string& text)
{
    gStrings.push_back(text);
    return gStrings.back().c_str();
}

} // namespace

void resetSpecArenas()
{
    gLists.clear();
    gListRows.clear();
    gLabelData.clear();
    gFrameData.clear();
    gPlaceRows.clear();
    gBarData.clear();
    gTrackerData.clear();
    gMultiData.clear();
    gParamLists.clear();
    gParamParams.clear();
    gParamValues.clear();
    gGridNames.clear();
    gGridNotePtrs.clear();
    gGridFinetunes.clear();
    gStrings.clear();
    resetSongSpecArenas();
    resetSampleSpecArenas();
}

const std::string& storeSpecString(const std::string& text)
{
    gStrings.push_back(text);
    return gStrings.back();
}

hControl createSpecControl(const SpecEntry& e, strControlProperties& prop,
                           const Scenario& scenario, bool& ok)
{
    hControl handle = static_cast<hControl>(-1);

    if (e.control == "cFrame")
    {
        std::vector<SpecPlace> places = e.places;
        if (places.empty())
            places.push_back({ e.x, e.y, e.w, e.h });
        if (places.size() > 64)
        {
            ok = false;
            return handle;
        }
        gFrameData.emplace_back();
        strFrameData& fd = gFrameData.back();
        fd.placesCount = static_cast<uint8_t>(places.size());
        fd.startPlace = 0;
        fd.multiSelActiveNum = 0;
        for (size_t pi = 0; pi < places.size(); ++pi)
        {
            int px = resolveInt(places[pi].x, scenario, ok);
            int py = resolveInt(places[pi].y, scenario, ok);
            int pw = resolveInt(places[pi].w, scenario, ok);
            int ph = resolveInt(places[pi].h, scenario, ok);
            if (!ok)
                return handle;
            gPlaceRows.emplace_back(std::array<uint16_t, 4>{
              static_cast<uint16_t>(std::max(0, px)), static_cast<uint16_t>(std::max(0, py)),
              static_cast<uint16_t>(std::max(0, pw)), static_cast<uint16_t>(std::max(0, ph)) });
            fd.places[pi] = gPlaceRows.back().data();
        }
        prop.data = &fd;
        handle = display.createControl<cFrame>(&prop);
    }
    else if (e.control == "cLabel")
    {
        if (e.hasDots)
        {
            gLabelData.emplace_back();
            strLabelData& ld = gLabelData.back();
            ld.dotIndicator.count = static_cast<uint8_t>(e.dots.count);
            ld.dotIndicator.current = static_cast<uint8_t>(e.dots.current);
            ld.styleValue = 0;
            ld.line = {};
            prop.data = &ld;
        }
        if (e.hasLine)
        {
            gLabelData.emplace_back();
            strLabelData& ld = gLabelData.back();
            ld.line.x1 = static_cast<uint16_t>(e.line.x1);
            ld.line.y1 = static_cast<uint16_t>(e.line.y1);
            ld.line.x2 = static_cast<uint16_t>(e.line.x2);
            ld.line.y2 = static_cast<uint16_t>(e.line.y2);
            ld.line.color = e.line.color;
            ld.styleValue = 0;
            prop.data = &ld;
        }
        handle = display.createControl<cLabel>(&prop);
    }
    else if (e.control == "cBar")
    {
        if (e.barDataWhite)
        {
            gBarData.emplace_back();
            gBarData.back().changingColor = 1;
            gBarData.back().value = 0xFFFFFF;
            prop.data = &gBarData.back();
        }
        if (!e.barCompare.empty())
        {
            int compare = resolveInt(e.barCompare, scenario, ok);
            if (!ok)
                return handle;
            gBarData.emplace_back();
            gBarData.back().changingColor = 0;
            gBarData.back().value = compare;
            prop.data = &gBarData.back();
        }
        handle = display.createControl<cBar>(&prop);
    }
    else if (e.control == "cList")
    {
        std::vector<std::string> items = e.list.items;
        if (!e.list.scenarioKey.empty())
        {
            if (!resolveStringList(e.list.scenarioKey, scenario, items))
            {
                ok = false;
                return handle;
            }
        }
        int start = 0;
        if (!e.list.start.empty())
            start = resolveInt(e.list.start, scenario, ok);
        if (!ok)
            return handle;

        gLists.emplace_back();
        strList& list = gLists.back();
        list.start = static_cast<int16_t>(start);
        list.length = static_cast<uint16_t>(items.size());
        list.linesCount = static_cast<uint16_t>(e.list.linesCount);
        list.selectTab = nullptr;
        list.selectionActive = static_cast<uint8_t>(e.list.selectionActive);
        list.icon = {};
        list.lineWidthMax = static_cast<uint16_t>(e.list.lineWidthMax);
        list.lineTextLenMax = static_cast<uint16_t>(e.list.lineTextLenMax);

        gListRows.emplace_back();
        std::vector<char*>& rows = gListRows.back();
        for (const auto& item : items)
            rows.push_back(const_cast<char*>(keepString(item)));
        list.data = rows.data();

        prop.data = &list;
        handle = display.createControl<cList>(&prop);
    }
    else if (e.control == "cParamValueList")
    {
        std::vector<std::pair<std::string, std::string>> rows;
        if (!e.paramList.scenarioKey.empty() &&
            !resolveRows(e.paramList.scenarioKey, scenario, rows))
        {
            ok = false;
            return handle;
        }
        int start = 0;
        if (!e.paramList.start.empty())
            start = resolveInt(e.paramList.start, scenario, ok);
        if (!ok)
            return handle;

        gParamLists.emplace_back();
        strParamValueList& list = gParamLists.back();
        list.valueEditActive = 0;
        list.start = static_cast<int16_t>(start);
        list.length = static_cast<uint16_t>(e.paramList.length > 0 ? e.paramList.length
                                                                   : rows.size());
        list.linesCount = static_cast<uint16_t>(e.paramList.linesCount);

        gParamParams.emplace_back();
        gParamValues.emplace_back();
        std::vector<char*>& params = gParamParams.back();
        std::vector<char*>& values = gParamValues.back();
        for (const auto& row : rows)
        {
            params.push_back(const_cast<char*>(keepString(row.first)));
            values.push_back(const_cast<char*>(keepString(row.second)));
        }
        list.params = params.data();
        list.values = values.data();

        prop.data = &list;
        handle = display.createControl<cParamValueList>(&prop);
    }
    else if (e.control == "cBgLabel")
    {
        handle = display.createControl<cBgLabel>(&prop);
    }
    else if (e.control == "cTracker")
    {
        gTrackerData.emplace_back();
        strTrackerPattern& tp = gTrackerData.back();
        tp.init();
        fillEmptyTrackerPattern(tp, scenario.pattern);
        prop.data = &tp;
        handle = display.createControl<cTracker>(&prop);
    }
    else if (e.control == "cMultiLabel")
    {
        int selected = 0;
        if (!e.multi.selected.empty())
            selected = resolveInt(e.multi.selected, scenario, ok);
        if (!ok)
            return handle;

        gMultiData.emplace_back();
        strMultiLabelData& md = gMultiData.back();
        md.textLinesCount = static_cast<uint8_t>(e.multi.textLinesCount);
        md.selected = static_cast<uint8_t>(selected);
        md.xOffset = static_cast<uint16_t>(e.multi.xOffset);
        md.yOffset = static_cast<uint16_t>(e.multi.yOffset);
        md.yForFxTextHole = static_cast<uint16_t>(e.multi.yForFxTextHole);
        for (size_t i = 0; i < e.multi.text.size() && i < 5; ++i)
            md.text[i] = const_cast<char*>(keepString(resolveText(e.multi.text[i], scenario)));

        prop.data = &md;
        handle = display.createControl<cMultiLabel>(&prop);
    }
    else if (e.control == "cTextBox")
    {
        prop.data = nullptr;
        handle = display.createControl<cTextBox>(&prop);
        if (handle >= 0)
        {
            std::string text = e.textFormat.empty() ? resolveText(e.text, scenario) : "";
            display.setControlText(handle, keepString(text));
        }
    }
    else if (e.control == "cGridEditorPopup")
    {
        int selected = 0;
        if (!e.grid.selected.empty())
            selected = resolveInt(e.grid.selected, scenario, ok);
        if (!ok)
            return handle;

        gGridNotePtrs.emplace_back();
        gGridFinetunes.emplace_back();
        gGridNames.emplace_back();
        std::vector<char*>& notes = gGridNotePtrs.back();
        std::vector<std::array<char, 5>>& finetunes = gGridFinetunes.back();
        strPadNamesGridEditor& names = gGridNames.back();

        const int count = std::max(1, e.grid.count);
        finetunes.resize(static_cast<size_t>(count));
        for (int i = 0; i < count; ++i)
        {
            char note[8];
            std::snprintf(note, sizeof(note), "%s%d", "C-", (i % 8) + 1);
            notes.push_back(const_cast<char*>(keepString(note)));
            std::snprintf(finetunes[static_cast<size_t>(i)].data(), 5, "%d", 0);
        }
        names.length = static_cast<uint8_t>(std::min(count, 48));
        names.noteName = notes.data();
        names.finetuneName = reinterpret_cast<char (*)[5]>(finetunes.data());

        prop.value = selected;
        prop.data = &names;
        handle = display.createControl<cGridEditorPopup>(&prop);
    }
    else if (e.control == "cSongPlayer")
    {
        handle = createSongPlayerControl(e, prop, scenario, ok);
    }
    else if (e.control == "cHorizontalBar")
    {
        prop.data = nullptr;
        handle = display.createControl<cHorizontalBar>(&prop);
        if (handle >= 0 && !e.text.empty())
            display.setControlText(handle, keepString(resolveText(e.text, scenario)));
    }
    else if (e.control == "cPoints" || e.control == "cSpectrum" ||
             e.control == "cProgressCursor" || e.control == "cLineIndicator" ||
             e.control == "cSlicePoints" || e.control == "cWtProgress")
    {
        handle = createSampleControl(e, prop, scenario, ok);
    }

    return handle;
}

} // namespace fwref
