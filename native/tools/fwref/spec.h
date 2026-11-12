#ifndef FWREF_SPEC_H_
#define FWREF_SPEC_H_

#include <cstdint>
#include <string>
#include <vector>

namespace fwref
{

struct SpecSource
{
    std::string file;
    int firstLine = 0;
    int lastLine = 0;
    std::string sha256;
};

struct SpecLine
{
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    uint32_t color = 0;
};

struct SpecList
{
    std::vector<std::string> items;
    std::string scenarioKey; // @scenario.list (items provided by scenario.cpp)
    std::string start = "0"; // int or @key
    int length = 0;
    int linesCount = 0;
    int selectionActive = 0;
    int lineWidthMax = 0;
    int lineTextLenMax = 0;
};

struct SpecParamList
{
    std::string scenarioKey; // @scenario.rows: [name, value] pairs
    std::string start = "0";
    int length = 0;
    int linesCount = 14;
};

struct SpecMultiLabel
{
    int textLinesCount = 1;
    std::string selected = "0";
    int xOffset = 0;
    int yOffset = 0;
    int yForFxTextHole = 0;
    std::vector<std::string> text; // literal or @key
};

struct SpecGrid
{
    std::string selected = "0";
    std::string notesKey;     // @scenario list of note names
    std::string finetuneKey;  // @scenario list of finetune strings
    int count = 48;
};

struct SpecDots
{
    int count = 0;
    int current = 0;
};

// cSongPlayer list geometry (songPlayer.cpp). Content comes from Scenario.song.
struct SpecSong
{
    std::string start = "";  // int or @key; empty -> scenario.selectedSlot
    std::string length = ""; // optional override; empty -> playlist size
    int linesCount = 12;
};

// cPoints (points.cpp) marker geometry. Values are int expressions or @keys.
struct SpecPoints
{
    std::string type = "0"; // pointsType (play mode)
    std::string start = "-1";
    std::string end = "-1";
    std::string loop1 = "-1";
    std::string loop2 = "-1";
    std::string selected = "0";
    std::string zerocrossed = "0";
};

// cSpectrum (spectrum.cpp). `from` names a deterministic waveform in Scenario.
struct SpecSpectrum
{
    std::string type = "0";         // spectrumType
    std::string pointsLimit = "300"; // 200|300|600
    std::string width = "";          // empty -> control width
    std::string waveformKey = "";    // e.g. "sample.waveform"
};

// cSlicePoints (slice.cpp).
struct SpecSlice
{
    std::string from = ""; // waveform-like slice list key
    std::string selected = "0";
};

// cWtProgress (wtProgress.cpp).
struct SpecWtProgress
{
    std::string position = "0";
    std::string window = "0";
    std::string positionMax = "1";
};

// cLineIndicator (lineIndicator.cpp).
struct SpecLineIndicator
{
    std::string start = "0";
    std::string end = "0";
};

struct SpecPlace
{
    std::string x;
    std::string y;
    std::string w;
    std::string h;
};

struct SpecEntry
{
    int order = 0;
    std::string control;

    std::string x = "0";
    std::string y = "0";
    std::string w = "0";
    std::string h = "0";

    std::vector<std::string> style;
    std::vector<std::string> style2;
    std::vector<uint32_t> colors;
    bool hasColors = false;

    bool hasValue = false;
    std::string value = "0";      // integer literal or @key
    std::vector<SpecPlace> places; // cFrame only: all framesPlaces rows
    std::string text = "";        // literal or @key
    std::string text2 = "";       // literal or @key
    std::string textFormat;       // formatter id for `text` (see scenario.cpp)
    std::string text2Format;      // formatter id (see scenario.cpp)
    int text2Arg = 0;             // per-column arg for text2 formatters
    std::string barValue;         // bar value mapping id
    bool barDataWhite = false;

    bool hasLine = false;
    SpecLine line;

    bool hasDots = false;
    SpecDots dots;

    bool hasList = false;
    SpecList list;

    bool hasParamList = false;
    SpecParamList paramList;

    bool hasMulti = false;
    SpecMultiLabel multi;

    bool hasGrid = false;
    SpecGrid grid;

    bool hasSong = false;
    SpecSong song;

    bool hasPoints = false;
    SpecPoints points;

    bool hasSpectrum = false;
    SpecSpectrum spectrum;

    bool hasSlice = false;
    SpecSlice slice;

    bool hasWt = false;
    SpecWtProgress wt;

    bool hasLineIndicator = false;
    SpecLineIndicator lineIndicator;

    std::string barCompare; // cBar compare-track value (strBarData::value)

    bool hidden = false;   // created but not shown (firmware default-hidden controls)
    int barArg = 0;        // per-index bar mapping argument
    SpecSource source;
};

struct ScreenSpec
{
    std::string screen;
    std::string scenario;
    std::string title;
    std::string rightLabel;
    bool chromeOnTop = false; // title bar paints over the screen body
    std::vector<SpecEntry> entries;
    std::vector<SpecEntry> chrome;
};

bool loadScreenSpecJson(const std::string& path, ScreenSpec& out, std::string& error);
bool loadChromeSpecJson(const std::string& path, std::vector<SpecEntry>& out, std::string& error);

bool evaluateExpression(const std::string& expr, int& out);
uint32_t styleMaskFromNames(const std::vector<std::string>& names);

// Strict variant: returns false if any name is unknown.
bool styleMaskFromNamesChecked(const std::vector<std::string>& names, uint32_t& mask);

} // namespace fwref

#endif // FWREF_SPEC_H_
