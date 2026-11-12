#include "spec.h"
#include "spec_parse_extras.h"

#include "displayStructs.h"

#include <nlohmann/json.hpp>

#include <cctype>
#include <fstream>
#include <map>

using nlohmann::json;

namespace fwref
{
namespace
{

bool parseIntValue(const json& v, int& out)
{
    if (v.is_number_integer())
    {
        out = v.get<int>();
        return true;
    }
    if (v.is_string())
    {
        return evaluateExpression(v.get<std::string>(), out);
    }
    return false;
}

SpecSource parseSource(const json& j)
{
    SpecSource s;
    if (!j.contains("source"))
        return s;
    const auto& sj = j["source"];
    s.file = sj.value("file", "");
    if (sj.contains("lines") && sj["lines"].is_array() && sj["lines"].size() == 2)
    {
        s.firstLine = sj["lines"][0].get<int>();
        s.lastLine = sj["lines"][1].get<int>();
    }
    s.sha256 = sj.value("sha256", "");
    return s;
}

SpecEntry parseEntry(const json& j)
{
    SpecEntry e;
    e.order = j.value("order", 0);
    e.control = j.value("control", "");
    e.x = j.value("x", "0");
    e.y = j.value("y", "0");
    e.w = j.value("w", "0");
    e.h = j.value("h", "0");

    if (j.contains("style"))
        for (const auto& s : j["style"])
            e.style.push_back(s.get<std::string>());
    if (j.contains("style2"))
        for (const auto& s : j["style2"])
            e.style2.push_back(s.get<std::string>());

    if (j.contains("colors") && j["colors"].is_array())
    {
        e.hasColors = true;
        for (const auto& c : j["colors"])
            e.colors.push_back(c.get<uint32_t>() & 0xFFFFFFu);
    }

    if (j.contains("value"))
    {
        e.hasValue = true;
        if (j["value"].is_string())
            e.value = j["value"].get<std::string>();
        else
            e.value = std::to_string(j["value"].get<int>());
    }

    if (j.contains("places") && j["places"].is_array())
    {
        for (const auto& pl : j["places"])
        {
            SpecPlace sp;
            sp.x = pl.value("x", "0");
            sp.y = pl.value("y", "0");
            sp.w = pl.value("w", "0");
            sp.h = pl.value("h", "0");
            e.places.push_back(sp);
        }
    }
    e.text = j.value("text", "");
    e.text2 = j.value("text2", "");
    e.textFormat = j.value("text_format", "");
    e.text2Format = j.value("text2_format", "");
    e.barValue = j.value("bar_value", "");
    e.barDataWhite = j.value("bar_data_white", false);

    if (j.contains("line"))
    {
        e.hasLine = true;
        const auto& l = j["line"];
        e.line.x1 = l.value("x1", 0);
        e.line.y1 = l.value("y1", 0);
        e.line.x2 = l.value("x2", 0);
        e.line.y2 = l.value("y2", 0);
        e.line.color = l.value("color", 0u) & 0xFFFFFFu;
    }

    if (j.contains("dots"))
    {
        e.hasDots = true;
        e.dots.count = j["dots"].value("count", 0);
        e.dots.current = j["dots"].value("current", 0);
    }

    parseSpecExtras(j, e);

    e.hidden = j.value("hidden", false);
    e.barArg = j.value("bar_arg", 0);

    e.source = parseSource(j);
    return e;
}

// --- integer expression evaluator (only 0-9 + - * / ( ) and whitespace) ---

struct ExprParser
{
    const std::string& s;
    size_t i = 0;
    bool ok = true;

    explicit ExprParser(const std::string& str) : s(str) {}

    void skip()
    {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
            i++;
    }

    long parseExpr()
    {
        long v = parseTerm();
        for (;;)
        {
            skip();
            if (i < s.size() && (s[i] == '+' || s[i] == '-'))
            {
                char op = s[i++];
                long r = parseTerm();
                v = (op == '+') ? v + r : v - r;
            }
            else
            {
                return v;
            }
        }
    }

    long parseTerm()
    {
        long v = parseFactor();
        for (;;)
        {
            skip();
            if (i < s.size() && (s[i] == '*' || s[i] == '/'))
            {
                char op = s[i++];
                long r = parseFactor();
                if (op == '*')
                {
                    v *= r;
                }
                else
                {
                    if (r == 0)
                    {
                        ok = false;
                        return 0;
                    }
                    v /= r;
                }
            }
            else
            {
                return v;
            }
        }
    }

    long parseFactor()
    {
        skip();
        if (i >= s.size())
        {
            ok = false;
            return 0;
        }
        if (s[i] == '(')
        {
            i++;
            long v = parseExpr();
            skip();
            if (i >= s.size() || s[i] != ')')
            {
                ok = false;
                return 0;
            }
            i++;
            return v;
        }
        if (std::isdigit(static_cast<unsigned char>(s[i])))
        {
            long v = 0;
            while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i])))
            {
                v = v * 10 + (s[i] - '0');
                i++;
            }
            return v;
        }
        ok = false;
        return 0;
    }
};

} // namespace

bool evaluateExpression(const std::string& expr, int& out)
{
    // Sample waveform geometry is written verbatim from the firmware as
    // `75 + ((300 - (MAX_16BIT / 300)) / 2)` (mtStructs.h:53).
    std::string normalized = expr;
    for (size_t pos = normalized.find("MAX_16BIT"); pos != std::string::npos;
         pos = normalized.find("MAX_16BIT", pos))
        normalized.replace(pos, 9, "65535");

    ExprParser p(normalized);
    long v = p.parseExpr();
    p.skip();
    if (!p.ok || p.i != normalized.size())
        return false;
    out = static_cast<int>(v);
    return true;
}

namespace
{
const std::map<std::string, uint32_t>& styleTable()
{
    static const std::map<std::string, uint32_t> kMap = {
        { "controlStyleShow", controlStyleShow },
        { "controlStyleCenterX", controlStyleCenterX },
        { "controlStyleCenterY", controlStyleCenterY },
        { "controlStyleRightX", controlStyleRightX },
        { "controlStyleShowBitmap", controlStyleShowBitmap },
        { "controlStyleUnderline", controlStyleUnderline },
        { "controlStyleVerticalText", controlStyleVerticalText },
        { "controlStyleFont1", controlStyleFont1 },
        { "controlStyleFont2", controlStyleFont2 },
        { "controlStyleFont3", controlStyleFont3 },
        { "controlStyleFont4", controlStyleFont4 },
        { "controlStyleFont5", controlStyleFont5 },
        { "controlStyleBorder", controlStyleBorder },
        { "controlStyleBackground", controlStyleBackground },
        { "controlStyleRoundedBorder", controlStyleRoundedBorder },
        { "controlStyleNoTransparency", controlStyleNoTransparency },
        { "controlStyleValueLeftRight_100_100", controlStyleValueLeftRight_100_100 },
        { "controlStyleValueLeftRight_24_24", controlStyleValueLeftRight_24_24 },
        { "controlStyleValue_0_100", controlStyleValue_0_100 },
        { "controlStyleValueNumberOnly", controlStyleValueNumberOnly },
        { "controlStyleCompareTwoValues", controlStyleCompareTwoValues },
        { "controlStyleBottomShadow", controlStyleBottomShadow },
        { "controlStyleTransformImg", controlStyleTransformImg },
        { "controlStyleHexNumbers", controlStyleHexNumbers },
        { "controlStyleStartNumbersFrom1", controlStyleStartNumbersFrom1 },
        { "controlStyleReverse_0_100", controlStyleReverse_0_100 },
        { "controlStyleGradient", controlStyleGradient },
        { "controlStyleSpecialFx", controlStyleSpecialFx },
        { "controlStyleSpecialFx2", controlStyleSpecialFx2 },
        { "controlStyleLimitTextWidth", controlStyleLimitTextWidth },
    };
    return kMap;
}
} // namespace

bool styleMaskFromNamesChecked(const std::vector<std::string>& names, uint32_t& mask)
{
    mask = 0;
    for (const auto& n : names)
    {
        auto it = styleTable().find(n);
        if (it == styleTable().end())
            return false;
        mask |= it->second;
    }
    return true;
}

uint32_t styleMaskFromNames(const std::vector<std::string>& names)
{
    uint32_t mask = 0;
    for (const auto& n : names)
    {
        auto it = styleTable().find(n);
        if (it != styleTable().end())
            mask |= it->second;
    }
    return mask;
}

bool loadScreenSpecJson(const std::string& path, ScreenSpec& out, std::string& error)
{
    std::ifstream f(path);
    if (!f)
    {
        error = "cannot open spec: " + path;
        return false;
    }
    json j;
    try
    {
        f >> j;
    }
    catch (const std::exception& e)
    {
        error = std::string("invalid spec JSON: ") + e.what();
        return false;
    }

    out.screen = j.value("screen", "");
    out.scenario = j.value("scenario", "");
    out.chromeOnTop = j.value("chrome_on_top", false);
    if (j.contains("chrome"))
    {
        out.title = j["chrome"].value("title", "");
        out.rightLabel = j["chrome"].value("rightLabel", "");
    }
    if (!j.contains("entries") || !j["entries"].is_array())
    {
        error = "spec has no entries array";
        return false;
    }
    for (const auto& e : j["entries"])
        out.entries.push_back(parseEntry(e));
    return true;
}

bool loadChromeSpecJson(const std::string& path, std::vector<SpecEntry>& out, std::string& error)
{
    std::ifstream f(path);
    if (!f)
    {
        error = "cannot open chrome spec: " + path;
        return false;
    }
    json j;
    try
    {
        f >> j;
    }
    catch (const std::exception& e)
    {
        error = std::string("invalid chrome JSON: ") + e.what();
        return false;
    }
    if (!j.contains("entries") || !j["entries"].is_array())
    {
        error = "chrome spec has no entries array";
        return false;
    }
    for (const auto& e : j["entries"])
        out.push_back(parseEntry(e));
    return true;
}

} // namespace fwref
