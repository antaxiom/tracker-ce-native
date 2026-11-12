#include "spec_parse_extras.h"

#include "spec.h"

#include <string>

namespace fwref
{
namespace
{

// A block value may be an integer literal or an integer expression / @key.
std::string scalar(const nlohmann::json& v)
{
    if (v.is_string())
        return v.get<std::string>();
    if (v.is_number_integer())
        return std::to_string(v.get<int>());
    if (v.is_boolean())
        return v.get<bool>() ? "1" : "0";
    return "0";
}

} // namespace

void parseSpecExtras(const nlohmann::json& j, SpecEntry& e)
{
if (j.contains("list"))
{
    e.hasList = true;
    const auto& l = j["list"];
    if (l.contains("items"))
        for (const auto& item : l["items"])
            e.list.items.push_back(item.get<std::string>());
    e.list.scenarioKey = l.value("from", "");
    if (l.contains("start"))
    {
        if (l["start"].is_string())
            e.list.start = l["start"].get<std::string>();
        else
            e.list.start = std::to_string(l["start"].get<int>());
    }
    e.list.length = l.value("length", static_cast<int>(e.list.items.size()));
    e.list.linesCount = l.value("linesCount", static_cast<int>(e.list.items.size()));
    e.list.selectionActive = l.value("selectionActive", 0) ? 1 : 0;
    e.list.lineWidthMax = l.value("lineWidthMax", 0);
    e.list.lineTextLenMax = l.value("lineTextLenMax", 0);
}

if (j.contains("param_list"))
{
    e.hasParamList = true;
    const auto& l = j["param_list"];
    e.paramList.scenarioKey = l.value("from", "");
    if (l.contains("start"))
    {
        if (l["start"].is_string())
            e.paramList.start = l["start"].get<std::string>();
        else
            e.paramList.start = std::to_string(l["start"].get<int>());
    }
    e.paramList.length = l.value("length", 0);
    e.paramList.linesCount = l.value("linesCount", 14);
}

if (j.contains("multi"))
{
    e.hasMulti = true;
    const auto& m = j["multi"];
    e.multi.textLinesCount = m.value("textLinesCount", 1);
    if (m.contains("selected"))
    {
        if (m["selected"].is_string())
            e.multi.selected = m["selected"].get<std::string>();
        else
            e.multi.selected = std::to_string(m["selected"].get<int>());
    }
    e.multi.xOffset = m.value("xOffset", 0);
    e.multi.yOffset = m.value("yOffset", 0);
    e.multi.yForFxTextHole = m.value("yForFxTextHole", 0);
    if (m.contains("text"))
        for (const auto& t : m["text"])
            e.multi.text.push_back(t.get<std::string>());
}

if (j.contains("grid"))
{
    e.hasGrid = true;
    const auto& g = j["grid"];
    if (g.contains("selected"))
    {
        if (g["selected"].is_string())
            e.grid.selected = g["selected"].get<std::string>();
        else
            e.grid.selected = std::to_string(g["selected"].get<int>());
    }
    e.grid.notesKey = g.value("notes_from", "");
    e.grid.finetuneKey = g.value("finetunes_from", "");
    e.grid.count = g.value("count", 48);
}

if (j.contains("song"))
{
    e.hasSong = true;
    const auto& s = j["song"];
    if (s.contains("start"))
    {
        if (s["start"].is_string())
            e.song.start = s["start"].get<std::string>();
        else
            e.song.start = std::to_string(s["start"].get<int>());
    }
    if (s.contains("length"))
    {
        if (s["length"].is_string())
            e.song.length = s["length"].get<std::string>();
        else
            e.song.length = std::to_string(s["length"].get<int>());
    }
    e.song.linesCount = s.value("linesCount", 12);
}

if (j.contains("points"))
{
    e.hasPoints = true;
    const auto& p = j["points"];
    if (p.contains("type"))
        e.points.type = scalar(p["type"]);
    if (p.contains("start"))
        e.points.start = scalar(p["start"]);
    if (p.contains("end"))
        e.points.end = scalar(p["end"]);
    if (p.contains("loop1"))
        e.points.loop1 = scalar(p["loop1"]);
    if (p.contains("loop2"))
        e.points.loop2 = scalar(p["loop2"]);
    if (p.contains("selected"))
        e.points.selected = scalar(p["selected"]);
    if (p.contains("zerocrossed"))
        e.points.zerocrossed = scalar(p["zerocrossed"]);
}

if (j.contains("spectrum"))
{
    e.hasSpectrum = true;
    const auto& s = j["spectrum"];
    if (s.contains("type"))
        e.spectrum.type = scalar(s["type"]);
    if (s.contains("pointsLimit"))
        e.spectrum.pointsLimit = scalar(s["pointsLimit"]);
    if (s.contains("width"))
        e.spectrum.width = scalar(s["width"]);
    e.spectrum.waveformKey = s.value("from", "");
}

if (j.contains("slice_points"))
{
    e.hasSlice = true;
    const auto& s = j["slice_points"];
    e.slice.from = s.value("from", "");
    if (s.contains("selected"))
        e.slice.selected = scalar(s["selected"]);
}

if (j.contains("wt_progress"))
{
    e.hasWt = true;
    const auto& w = j["wt_progress"];
    if (w.contains("position"))
        e.wt.position = scalar(w["position"]);
    if (w.contains("window"))
        e.wt.window = scalar(w["window"]);
    if (w.contains("positionMax"))
        e.wt.positionMax = scalar(w["positionMax"]);
}

if (j.contains("line_indicator"))
{
    e.hasLineIndicator = true;
    const auto& l = j["line_indicator"];
    if (l.contains("start"))
        e.lineIndicator.start = scalar(l["start"]);
    if (l.contains("end"))
        e.lineIndicator.end = scalar(l["end"]);
}

if (j.contains("bar_compare"))
    e.barCompare = scalar(j["bar_compare"]);
}

} // namespace fwref
