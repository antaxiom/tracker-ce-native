#include "spec_controls.h"

#include "lineIndicatorControl.h"
#include "pointsControl.h"
#include "progressCursorControl.h"
#include "scenario_util.h"
#include "sliceControl.h"
#include "spectrumControl.h"
#include "wtProgress.h"

#include <algorithm>
#include <deque>
#include <vector>

namespace fwref
{
namespace
{

std::deque<strTrackerPoints> gPoints;
std::deque<strTrackerSpectrum> gSpectra;
std::deque<strLineIndicatorSelection> gLineIndicators;
std::deque<strSlicePoints> gSlices;
std::deque<wt_position_data_t> gWts;

int resolveOr(const std::string& expr, const Scenario& scenario, bool& ok, int fallback)
{
    if (expr.empty())
        return fallback;
    return resolveInt(expr, scenario, ok);
}

strTrackerSpectrum::PointsLimit pointsLimitOf(int limit)
{
    if (limit <= 200)
        return strTrackerSpectrum::k200;
    if (limit >= 600)
        return strTrackerSpectrum::k600;
    return strTrackerSpectrum::k300;
}

} // namespace

void resetSampleSpecArenas()
{
    gPoints.clear();
    gSpectra.clear();
    gLineIndicators.clear();
    gSlices.clear();
    gWts.clear();
}

hControl createSampleControl(const SpecEntry& e, strControlProperties& prop,
                             const Scenario& scenario, bool& ok)
{
    hControl handle = static_cast<hControl>(-1);

    if (e.control == "cPoints")
    {
        gPoints.emplace_back();
        strTrackerPoints& points = gPoints.back();
        points.pointsType = static_cast<uint8_t>(resolveInt(e.points.type, scenario, ok));
        points.startPoint = static_cast<int16_t>(resolveInt(e.points.start, scenario, ok));
        points.endPoint = static_cast<int16_t>(resolveInt(e.points.end, scenario, ok));
        points.loopPoint1 = static_cast<int16_t>(resolveInt(e.points.loop1, scenario, ok));
        points.loopPoint2 = static_cast<int16_t>(resolveInt(e.points.loop2, scenario, ok));
        points.selected = static_cast<uint8_t>(resolveInt(e.points.selected, scenario, ok));
        points.zerocrossed =
          static_cast<uint8_t>(resolveInt(e.points.zerocrossed, scenario, ok));
        if (!ok)
            return handle;
        prop.data = &points;
        handle = display.createControl<cPoints>(&prop);
    }
    else if (e.control == "cSpectrum")
    {
        gSpectra.emplace_back();
        strTrackerSpectrum& spectrum = gSpectra.back();
        const int limit = resolveOr(e.spectrum.pointsLimit, scenario, ok,
                                    static_cast<int>(strTrackerSpectrum::k300));
        spectrum.init();
        spectrum.spectrumType = static_cast<uint8_t>(resolveInt(e.spectrum.type, scenario, ok));
        spectrum.pointsLimit = pointsLimitOf(limit);
        const int width = resolveOr(e.spectrum.width, scenario, ok,
                                    prop.w ? static_cast<int>(prop.w) : 600);
        spectrum.width = static_cast<uint32_t>(width);
        if (!ok)
            return handle;

        std::vector<int> waveform;
        if (!e.spectrum.waveformKey.empty() &&
            !resolveLoaderSampleIntList(e.spectrum.waveformKey, scenario, waveform))
        {
            ok = false;
            return handle;
        }
        const int step = (width > 0 && limit > 0) ? (width / limit) : 1;
        for (size_t i = 0; i < waveform.size(); ++i)
        {
            const int idx = static_cast<int>(i) * std::max(1, step);
            if (idx < 0 || idx >= 600)
                break;
            int amp = std::clamp(waveform[i], 0, 127);
            spectrum.upperData[idx] = static_cast<int8_t>(amp);
            spectrum.lowerData[idx] = static_cast<int8_t>(-amp);
        }

        prop.data = &spectrum;
        handle = display.createControl<cSpectrum>(&prop);
    }
    else if (e.control == "cProgressCursor")
    {
        handle = display.createControl<cProgressCursor>(&prop);
    }
    else if (e.control == "cLineIndicator")
    {
        gLineIndicators.emplace_back();
        strLineIndicatorSelection& selection = gLineIndicators.back();
        selection.start =
          static_cast<uint16_t>(resolveInt(e.lineIndicator.start, scenario, ok));
        selection.end = static_cast<uint16_t>(resolveInt(e.lineIndicator.end, scenario, ok));
        if (!ok)
            return handle;
        prop.data = &selection;
        handle = display.createControl<cLineIndicator>(&prop);
    }
    else if (e.control == "cSlicePoints")
    {
        std::vector<int> slices;
        if (!e.slice.from.empty() &&
            !resolveLoaderSampleIntList(e.slice.from, scenario, slices))
        {
            ok = false;
            return handle;
        }
        gSlices.emplace_back();
        strSlicePoints& points = gSlices.back();
        const int count = std::clamp(static_cast<int>(slices.size()), 0,
                                     static_cast<int>(MAX_SLICE_NUMBER));
        points.sliceCount = static_cast<uint8_t>(count);
        points.selected = static_cast<uint8_t>(resolveInt(e.slice.selected, scenario, ok));
        for (int i = 0; i < static_cast<int>(MAX_SLICE_NUMBER); ++i)
        {
            points.slice[i] = -1;
            points.isSliceZerocrossed[i] = false;
        }
        for (int i = 0; i < count; ++i)
            points.slice[i] = static_cast<int16_t>(slices[static_cast<size_t>(i)]);
        if (!ok)
            return handle;
        prop.data = &points;
        handle = display.createControl<cSlicePoints>(&prop);
    }
    else if (e.control == "cWtProgress")
    {
        gWts.emplace_back();
        wt_position_data_t& wt = gWts.back();
        wt.position = static_cast<uint32_t>(resolveInt(e.wt.position, scenario, ok));
        wt.window = static_cast<uint32_t>(resolveInt(e.wt.window, scenario, ok));
        wt.positionMax = static_cast<uint32_t>(resolveInt(e.wt.positionMax, scenario, ok));
        if (!ok)
            return handle;
        prop.data = &wt;
        handle = display.createControl<cWtProgress>(&prop);
    }

    return handle;
}

} // namespace fwref
