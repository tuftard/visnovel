#define NOMINMAX
#include "soraeditor.h"
#include <string>
#include <random>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <vector>
#include <windows.h>

using namespace std;

// =============================================================
// SETUP
// =============================================================
namespace Retro
{
    const sf::Color Desktop(30, 130, 225);
    const sf::Color Frame(190, 225, 250);
    const sf::Color TitleTop(240, 250, 255);
    const sf::Color TitleBot(150, 200, 240);
    const sf::Color Panel(215, 235, 250); // code background
    const sf::Color Navy(20, 30, 90);     // text + status bar
    const sf::Color Royal(45, 65, 185);   // checker dark
    const sf::Color Mint(165, 240, 190);  // checker light
    const sf::Color Cream(245, 232, 140); // output panel
    const sf::Color Close(235, 90, 120);
    const sf::Color Orange(240, 150, 50); // popup dashed border
    const sf::Color Hi(255, 255, 255);    // bevel light
    const sf::Color Lo(90, 120, 170);     // bevel dark
}

namespace Light
{
    const sf::Color text = Retro::Navy;
    const sf::Color keyword(45, 65, 185);
    const sf::Color builtin(0, 120, 150);
    const sf::Color str(190, 40, 110);
    const sf::Color number(220, 110, 20);
    const sf::Color comment(110, 140, 170);
    const sf::Color intro = sf::Color::Black; // original typed-out words
}

static void bevelBox(sf::RenderTarget &t, sf::FloatRect r, sf::Color fill, bool raised = true)
{
    sf::RectangleShape box(r.size);
    box.setPosition(r.position);
    box.setFillColor(fill);
    t.draw(box);

    sf::Color tl = raised ? Retro::Hi : Retro::Lo;
    sf::Color br = raised ? Retro::Lo : Retro::Hi;

    sf::RectangleShape e;
    e.setFillColor(tl);
    e.setSize({r.size.x, 2});
    e.setPosition(r.position);
    t.draw(e);
    e.setSize({2, r.size.y});
    e.setPosition(r.position);
    t.draw(e);
    e.setFillColor(br);
    e.setSize({r.size.x, 2});
    e.setPosition({r.position.x, r.position.y + r.size.y - 2});
    t.draw(e);
    e.setSize({2, r.size.y});
    e.setPosition({r.position.x + r.size.x - 2, r.position.y});
    t.draw(e);
}

static void gradientBar(sf::RenderTarget &t, sf::FloatRect r, sf::Color top, sf::Color bot)
{
    sf::VertexArray v(sf::PrimitiveType::TriangleStrip, 4);
    v[0] = sf::Vertex(r.position, top);
    v[1] = sf::Vertex({r.position.x + r.size.x, r.position.y}, top);
    v[2] = sf::Vertex({r.position.x, r.position.y + r.size.y}, bot);
    v[3] = sf::Vertex({r.position.x + r.size.x, r.position.y + r.size.y}, bot);
    t.draw(v);
}

SoraEditor::SoraEditor(const sf::Font &monoFont) : font_(&monoFont)
{
    cw_ = font_->getGlyph(U'M', 14, false).advance;

    background_.setSize({1200.f, 620.f});
    background_.setPosition({30.f, 20.f});
    background_.setFillColor(Retro::Frame);

    titleBar_.setSize({1200.f, 28.f});
    titleBar_.setPosition({30.f, 20.f});
    titleBar_.setFillColor(sf::Color::Cyan);

    closeBtn_.setSize({16.f, 16.f});
    closeBtn_.setPosition({1198.f, 26.f});
    closeBtn_.setFillColor(sf::Color(220, 50, 50));

    buildBtn_.setSize({150.f, 40.f});
    buildBtn_.setPosition({545.f, 490.f});
    buildBtn_.setFillColor(sf::Color(0, 110, 180));
    buildBtn_.setOutlineColor(sf::Color(0, 0, 255));
    buildBtn_.setOutlineThickness(1.f);

    cursor_.setSize({2.f, 16.f});
    cursor_.setFillColor(Retro::Navy);

    buildMenus();
}

// keeps the black_ flags the same length as code_
void SoraEditor::insertText(size_t pos, const string &s)
{
    code_.insert(pos, s);
    black_.insert(black_.begin() + static_cast<long long>(pos), s.size(), static_cast<char>(0));
}

void SoraEditor::eraseText(size_t pos, size_t n)
{
    code_.erase(pos, n);
    black_.erase(black_.begin() + static_cast<long long>(pos),
                 black_.begin() + static_cast<long long>(pos + n));
}

void SoraEditor::loadCommentsFrom(const string &fileName)
{
    ifstream file(fileName);

    if (!file.is_open())
        return;

    string line;
    int commentCount = 0;

    while (getline(file, line))
    {
        size_t first = line.find_first_not_of(" \t");

        if (first != string::npos && line[first] == '/' &&
            first + 1 < line.size() && line[first + 1] == '/')
        {
            code_ += line.substr(first) + "\n";
            commentCount++;

            if (commentCount >= 3)
                code_ += "\n";
        }
    }

    // everything loaded so far is the original intro text: draw it black
    black_.assign(code_.size(), 1);
    cursorPos_ = 0;
}
// =============================================================
// Constant Header: a new copy is stamped every time the editor opens or closes
// =============================================================
const string HEADER = "//Hey dummy just ignore all this code yknow you wouldn't understand it anyway, but you could learn something if you tried.....";

struct Stamp
{
    float x, y; // normalized 0-1
};

static vector<Stamp> stamps;
static mt19937 rng{random_device{}()};
static bool stampsLoaded = false;
static bool pendingStamp = false; // set by open()/close(), handled in drawStamps
static bool stampsLocked = false; // A+ grade: no new stamps until the game is closed (never saved)
static const float STAMP_ROW_H = 18.f; // one row per stamp, so copies never overlap

static void loadStamps(const string &path = "clutter.txt")
{
    ifstream in(path);
    Stamp s;
    while (in >> s.x >> s.y)
        stamps.push_back(s);
}

static void saveStamps(const string &path = "clutter.txt")
{
    ofstream out(path);
    for (const auto &s : stamps)
        out << s.x << " " << s.y << "\n";
}

// Picks a random row nobody is using yet; if all rows are taken,
// picks among the least-used rows.
static void addStamp(float textW, float winW, float winH)
{
    static int frames = 0;
    if (++frames % 120 == 0)
    {
        printf("drawStamps alive: pending=%d stamps=%zu\n", pendingStamp ? 1 : 0, stamps.size());
        fflush(stdout);
    }
    const int slots = max(1, static_cast<int>((winH - STAMP_ROW_H) / STAMP_ROW_H) + 1);
    vector<int> used(slots, 0);

    for (const auto &e : stamps)
    {
        int slot = clamp(static_cast<int>(lround(e.y * winH / STAMP_ROW_H)), 0, slots - 1);
        used[slot]++;
    }

    int fewest = *min_element(used.begin(), used.end());
    vector<int> candidates;
    for (int i = 0; i < slots; i++)
        if (used[i] == fewest)
            candidates.push_back(i);

    int slot = candidates[uniform_int_distribution<size_t>(0, candidates.size() - 1)(rng)];

    float rangeX = max(0.f, 1.f - textW / winW);
    Stamp s;
    s.x = uniform_real_distribution<float>(0.f, 1.f)(rng) * rangeX;
    s.y = (slot * STAMP_ROW_H) / winH;
    stamps.push_back(s);
}

void drawStamps(sf::RenderWindow &window, const sf::Font &font)
{
    if (!stampsLoaded)
    {
        loadStamps();
        stampsLoaded = true;
    }

    window.setView(window.getDefaultView()); // screen space, whatever the scene did

    sf::Text text(font, HEADER, 14);
    text.setFillColor(sf::Color(255, 255, 255, 170)); // light fill + dark outline
    text.setOutlineColor(sf::Color(0, 0, 0, 170));    // = readable on any background
    text.setOutlineThickness(1.f);

    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);

    if (pendingStamp)
    {
        if (!stampsLocked)
        {
            addStamp(text.getLocalBounds().size.x, winW, winH);
            saveStamps();
        }
        pendingStamp = false;
    }

    for (const auto &s : stamps)
    {
        text.setPosition({s.x * winW, s.y * winH});
        window.draw(text);
    }
}
// admin tool: wipe every stamp and the saved file
void clearStamps()
{
    stamps.clear();
    stampsLoaded = true; // don't reload the old file afterwards
    pendingStamp = false;
    saveStamps(); // writes an empty clutter.txt
}
// called after each graded build
void applyGradeToStamps(char rank)
{
    if (rank == 'F')
        return;

    if (!stampsLoaded)
    {
        loadStamps();
        stampsLoaded = true;
    }

    shuffle(stamps.begin(), stamps.end(), rng); // so random ones are removed from the back

    switch (rank)
    {
    case 'D':
        if (!stamps.empty())
            stamps.pop_back();
        break;

    case 'C':
        stamps.erase(stamps.end() - static_cast<long long>(stamps.size() / 2), stamps.end());
        break;

    case 'B':
        if (stamps.size() > 1)
            stamps.erase(stamps.begin() + 1, stamps.end());
        break;

    default: // A, S, anything higher
        stamps.clear();
        stampsLocked = true;
        break;
    }

    saveStamps();
}
void SoraEditor::open()
{
    printf("open() called\n");
    fflush(stdout);
    pendingStamp = true;
    open_ = true;
    scrollOffset_ = 0.f;
    cursorPos_ = 0;
    lastCursor_ = static_cast<size_t>(-1);
    menuOpen_ = -1;
    cursorClock_.restart();
}

void SoraEditor::close()
{
    pendingStamp = true;
    open_ = false;
    menuOpen_ = -1;
}

// =============================================================
// SAVE / BUILD / RUN
// =============================================================

bool SoraEditor::save() const
{
    ofstream out("test.cpp");

    if (!out.is_open())
        return false;

    out << code_;
    return true;
}

void SoraEditor::runExe()
{
    ifstream exe("test.exe", ios::binary);

    if (!exe.is_open())
    {
        buildOk_ = false;
        buildOutput_ = "No test.exe yet - build first.";
        return;
    }

    exe.close();

    // test code may use cin, so give it its own console window
    system("start \"\" cmd /k test.exe");
}

bool SoraEditor::build(bool launch)
{
    builtFlag_ = true;
    buildOk_ = false;

    if (!save())
    {
        buildOutput_ = "Could not write test.cpp";
        return false;
    }

    ::remove("test.exe"); // an old exe must not pass for a new build

    FILE *pipe = _popen("g++ test.cpp -o test.exe 2>&1", "r");

    if (!pipe)
    {
        buildOutput_ = "Could not start g++ (is it on PATH?)";
        return false;
    }

    string output;
    char buf[256];

    while (fgets(buf, sizeof buf, pipe))
        output += buf;

    int rc = _pclose(pipe);

    if (rc != 0)
    {
        buildOutput_ = output.empty() ? "Compile failed." : output;
        return false;
    }

    buildOk_ = true;
    buildOutput_ = output.empty() ? "Build succeeded - 0 errors." : output;

    if (launch)
        runExe();

    return true;
}

// =============================================================
// MENU BAR
// =============================================================

void SoraEditor::buildMenus()
{
    menuNames_ = {"File", "Edit", "Build", "View"};

    menus_.clear();
    menus_.resize(4);

    // File
    menus_[0] = {
        {"New", "", Cmd::New, false},
        {"Save", "Ctrl+S", Cmd::Save, false},
        {"", "", Cmd::None, true},
        {"Close Editor", "", Cmd::CloseEditor, false},
    };

    // Edit
    menus_[1] = {
        {"Undo", "Ctrl+Z", Cmd::Undo, false},
        {"Redo", "Ctrl+Y", Cmd::Redo, false},
        {"", "", Cmd::None, true},
        {"Paste", "Ctrl+V", Cmd::Paste, false},
        {"", "", Cmd::None, true},
        {"Copy Line", "", Cmd::CopyLine, false},
        {"Cut Line", "", Cmd::CutLine, false},
        {"Delete Line", "", Cmd::DeleteLine, false},
        {"Duplicate Line", "", Cmd::DuplicateLine, false},
    };

    // Build
    menus_[2] = {
        {"Build & Run", "F5", Cmd::BuildRun, false},
        {"Build Only", "", Cmd::BuildOnly, false},
        {"Run", "F6", Cmd::RunOnly, false},
        {"", "", Cmd::None, true},
        {"Clear Output", "", Cmd::ClearOutput, false},
    };

    // View
    menus_[3] = {
        {"Scanlines", "", Cmd::ToggleScanlines, false},
        {"Intro Text Black", "", Cmd::ToggleIntroBlack, false},
        {"", "", Cmd::None, true},
        {"Go to Top", "", Cmd::GoTop, false},
        {"Go to Bottom", "", Cmd::GoBottom, false},
    };
}

sf::FloatRect SoraEditor::headerRect(int m) const
{
    return {{menuX + static_cast<float>(m) * (menuHeaderW + menuGap), menuY},
            {menuHeaderW, menuHeaderH}};
}

sf::FloatRect SoraEditor::dropRect(int m) const
{
    sf::FloatRect h = headerRect(m);
    float total = 0.f;

    for (const MenuItem &it : menus_[m])
        total += it.sep ? 8.f : 22.f;

    return {{h.position.x, h.position.y + h.size.y}, {menuW, total + 6.f}};
}

sf::FloatRect SoraEditor::itemRect(int m, int j) const
{
    sf::FloatRect h = headerRect(m);
    float y = h.position.y + h.size.y + 3.f;

    for (int k = 0; k < j; k++)
        y += menus_[m][k].sep ? 8.f : 22.f;

    float ht = menus_[m][j].sep ? 8.f : 22.f;
    return {{h.position.x + 2.f, y}, {menuW - 4.f, ht}};
}

void SoraEditor::drawMenus(sf::RenderWindow &window)
{
    // header buttons (always visible)
    for (int i = 0; i < static_cast<int>(menuNames_.size()); i++)
    {
        sf::FloatRect r = headerRect(i);
        bool hot = r.contains(mouse_);

        if (menuOpen_ == i)
            bevelBox(window, r, Retro::Panel, false);
        else if (hot)
            bevelBox(window, r, Retro::TitleTop, true);

        sf::Text t(*font_, menuNames_[i], 13);
        t.setPosition({r.position.x + 9.f, r.position.y + 3.f});
        t.setFillColor(Retro::Navy);
        window.draw(t);
    }

    if (menuOpen_ < 0)
        return;

    // dropdown
    bevelBox(window, dropRect(menuOpen_), Retro::TitleTop, true);

    for (int j = 0; j < static_cast<int>(menus_[menuOpen_].size()); j++)
    {
        const MenuItem &it = menus_[menuOpen_][j];
        sf::FloatRect r = itemRect(menuOpen_, j);

        if (it.sep)
        {
            sf::RectangleShape line({r.size.x - 12.f, 1.f});
            line.setPosition({r.position.x + 6.f, r.position.y + 4.f});
            line.setFillColor(Retro::Lo);
            window.draw(line);
            continue;
        }

        bool hot = r.contains(mouse_);

        if (hot)
        {
            sf::RectangleShape bar(r.size);
            bar.setPosition(r.position);
            bar.setFillColor(Retro::Royal);
            window.draw(bar);
        }

        sf::Color fg = hot ? sf::Color::White : Retro::Navy;

        // check mark for toggles
        bool checked = (it.cmd == Cmd::ToggleScanlines && showScanlines_) ||
                       (it.cmd == Cmd::ToggleIntroBlack && showIntroBlack_);

        if (checked)
        {
            sf::Text ck(*font_, "x", 13);
            ck.setPosition({r.position.x + 6.f, r.position.y + 3.f});
            ck.setFillColor(fg);
            window.draw(ck);
        }

        sf::Text label(*font_, it.label, 13);
        label.setPosition({r.position.x + 22.f, r.position.y + 3.f});
        label.setFillColor(fg);
        window.draw(label);

        if (!it.hint.empty())
        {
            float hw = static_cast<float>(it.hint.size()) * cw_ * 12.f / 14.f;
            sf::Text hint(*font_, it.hint, 12);
            hint.setPosition({r.position.x + r.size.x - hw - 8.f, r.position.y + 4.f});
            hint.setFillColor(hot ? sf::Color::White : Retro::Lo);
            window.draw(hint);
        }
    }
}

// =============================================================
// MENU COMMANDS
// =============================================================

size_t SoraEditor::lineEndOf(size_t pos) const
{
    size_t e = code_.find('\n', pos);
    return e == string::npos ? code_.size() : e;
}

void SoraEditor::undo()
{
    if (undo_.empty())
        return;

    redo_.push_back({code_, black_, cursorPos_});

    Snapshot s = undo_.back();
    undo_.pop_back();

    code_ = s.code;
    black_ = s.black;
    cursorPos_ = min(s.cursor, code_.size());
    lastCursor_ = static_cast<size_t>(-1);
    cursorClock_.restart();
}

void SoraEditor::redo()
{
    if (redo_.empty())
        return;

    undo_.push_back({code_, black_, cursorPos_});

    Snapshot s = redo_.back();
    redo_.pop_back();

    code_ = s.code;
    black_ = s.black;
    cursorPos_ = min(s.cursor, code_.size());
    lastCursor_ = static_cast<size_t>(-1);
    cursorClock_.restart();
}

void SoraEditor::execute(Cmd c)
{
    // commands that never change the text
    switch (c)
    {
    case Cmd::Undo:
        undo();
        return;

    case Cmd::Redo:
        redo();
        return;

    case Cmd::Save:
        buildOk_ = save();
        buildOutput_ = buildOk_ ? "Saved test.cpp" : "Could not write test.cpp";
        return;

    case Cmd::CloseEditor:
        close();
        return;

    case Cmd::CopyLine:
    {
        size_t ls = lineStart(cursorPos_);
        sf::Clipboard::setString(code_.substr(ls, lineEndOf(ls) - ls));
        return;
    }

    case Cmd::BuildRun:
        build(true);
        return;

    case Cmd::BuildOnly:
        build(false);
        return;

    case Cmd::RunOnly:
        runExe();
        return;

    case Cmd::ClearOutput:
        buildOutput_.clear();
        buildOk_ = true;
        return;

    case Cmd::ToggleScanlines:
        showScanlines_ = !showScanlines_;
        return;

    case Cmd::ToggleIntroBlack:
        showIntroBlack_ = !showIntroBlack_;
        return;

    case Cmd::GoTop:
        cursorPos_ = 0;
        scrollOffset_ = 0.f;
        lastCursor_ = static_cast<size_t>(-1);
        return;

    case Cmd::GoBottom:
        cursorPos_ = code_.size();
        lastCursor_ = static_cast<size_t>(-1);
        return;

    default:
        break;
    }

    // commands that edit the text: remember the old state for undo
    Snapshot before{code_, black_, cursorPos_};

    switch (c)
    {
    case Cmd::New:
        printf("New clicked\n");
        fflush(stdout);
        pendingStamp = true;
        eraseText(0, code_.size());
        cursorPos_ = 0;
        scrollOffset_ = 0.f;
        buildOutput_ = "New file.";
        buildOk_ = true;
        break;

    case Cmd::Paste:
        paste();
        break;

    case Cmd::CutLine:
    {
        size_t ls = lineStart(cursorPos_);
        size_t le = lineEndOf(ls);
        sf::Clipboard::setString(code_.substr(ls, le - ls));

        size_t from = ls;
        size_t to = le;

        if (le < code_.size())
            to = le + 1; // take the newline with the line
        else if (ls > 0)
            from = ls - 1; // last line: take the newline before it

        eraseText(from, to - from);
        cursorPos_ = min(from, code_.size());
        break;
    }

    case Cmd::DeleteLine:
    {
        size_t ls = lineStart(cursorPos_);
        size_t le = lineEndOf(ls);

        size_t from = ls;
        size_t to = le;

        if (le < code_.size())
            to = le + 1;
        else if (ls > 0)
            from = ls - 1;

        eraseText(from, to - from);
        cursorPos_ = min(from, code_.size());
        break;
    }

    case Cmd::DuplicateLine:
    {
        size_t ls = lineStart(cursorPos_);
        size_t le = lineEndOf(ls);
        size_t column = cursorPos_ - ls;
        string line = code_.substr(ls, le - ls);

        insertText(le, "\n" + line);
        cursorPos_ = le + 1 + min(column, line.size());
        break;
    }

    default:
        break;
    }

    if (code_ != before.code)
    {
        undo_.push_back(before);

        if (undo_.size() > 200)
            undo_.erase(undo_.begin());

        redo_.clear();
    }

    lastCursor_ = static_cast<size_t>(-1);
    cursorClock_.restart();
}

// =============================================================
// CURSOR HELPERS
// =============================================================

size_t SoraEditor::lineStart(size_t pos) const
{
    if (pos == 0)
        return 0;

    size_t p = code_.rfind('\n', pos - 1);
    return p == string::npos ? 0 : p + 1;
}

void SoraEditor::clampScroll()
{
    size_t lineCount = count(code_.begin(), code_.end(), '\n') + 1;
    float maxScroll = max(0.f, static_cast<float>(lineCount) * lineHeight - (codeBottom - codeTop));

    scrollOffset_ = clamp(scrollOffset_, 0.f, maxScroll);
}

void SoraEditor::placeCursor(sf::Vector2f mouse)
{
    int targetLine = max(0, static_cast<int>((mouse.y - codeTop + scrollOffset_) / lineHeight));

    size_t start = 0;
    int currentLine = 0;
    size_t target = code_.size();

    while (start < code_.size())
    {
        size_t end = code_.find('\n', start);

        if (end == string::npos)
            end = code_.size();

        if (currentLine == targetLine)
        {
            float drawX = codeLeft;
            target = start;
            float minDistance = abs(mouse.x - drawX);

            for (size_t i = start; i < end; i++)
            {
                sf::Text charText(*font_, string(1, code_[i]), 14);
                float charWidth = charText.getLocalBounds().size.x;
                float charCenter = drawX + charWidth / 2.f;
                float dist = abs(mouse.x - charCenter);

                if (dist < minDistance)
                {
                    minDistance = dist;
                    target = (mouse.x >= charCenter) ? i + 1 : i;
                }

                drawX += charWidth;
            }

            if (mouse.x >= drawX)
                target = end;

            break;
        }

        currentLine++;

        if (end == code_.size())
        {
            target = code_.size();
            break;
        }

        start = end + 1;
        target = start;
    }

    cursorPos_ = target;
    cursorClock_.restart();
}

void SoraEditor::moveUp()
{
    size_t start = lineStart(cursorPos_);

    if (start == 0)
        return;

    size_t prevEnd = start - 1;
    size_t prevStart = lineStart(prevEnd);

    size_t column = min(cursorPos_ - start, prevEnd - prevStart);
    cursorPos_ = prevStart + column;
}

void SoraEditor::moveDown()
{
    size_t start = lineStart(cursorPos_);
    size_t column = cursorPos_ - start;

    size_t nextStart = code_.find('\n', cursorPos_);

    if (nextStart == string::npos)
        return;

    nextStart++;

    size_t nextEnd = code_.find('\n', nextStart);

    if (nextEnd == string::npos)
        nextEnd = code_.size();

    cursorPos_ = nextStart + min(column, nextEnd - nextStart);
}

void SoraEditor::paste()
{
    string pasted = getClipboardText();

    if (pasted.empty())
        return;

    pasted.erase(remove(pasted.begin(), pasted.end(), '\r'), pasted.end());

    size_t tabPos = 0;

    while ((tabPos = pasted.find('\t', tabPos)) != string::npos)
    {
        pasted.replace(tabPos, 1, "    ");
        tabPos += 4;
    }

    // indentation of the current line
    size_t start = lineStart(cursorPos_);
    size_t indentEnd = start;

    while (indentEnd < code_.size() && (code_[indentEnd] == ' ' || code_[indentEnd] == '\t'))
        indentEnd++;

    string indent = code_.substr(start, indentEnd - start);

    string formatted;

    for (size_t i = 0; i < pasted.size(); i++)
    {
        formatted += pasted[i];

        if (pasted[i] == '\n' && i + 1 < pasted.size())
            formatted += indent;
    }

    insertText(cursorPos_, formatted);
    cursorPos_ += formatted.size();
}

// =============================================================
// INPUT
// =============================================================

void SoraEditor::onText(char32_t unicode)
{
    if (unicode < 32 || unicode >= 127)
        return;

    size_t column = cursorPos_ - lineStart(cursorPos_);

    if (column >= static_cast<size_t>(maxColumns))
    {
        insertText(cursorPos_, "\n");
        cursorPos_++;
    }

    insertText(cursorPos_, string(1, static_cast<char>(unicode)));
    cursorPos_++;
}

bool SoraEditor::onKey(const sf::Event::KeyPressed &k)
{
    using Key = sf::Keyboard::Key;

    if (k.control && k.code == Key::V)
    {
        paste();
        return true;
    }

    if (k.control && k.code == Key::S)
    {
        execute(Cmd::Save);
        return true;
    }

    switch (k.code)
    {
    case Key::F5:
        build(true);
        break;

    case Key::F6:
        runExe();
        break;

    case Key::Home:
        cursorPos_ = lineStart(cursorPos_);
        break;

    case Key::End:
        cursorPos_ = lineEndOf(cursorPos_);
        break;

    case Key::Tab:
        insertText(cursorPos_, "    ");
        cursorPos_ += 4;
        break;

    case Key::Backspace:
        if (cursorPos_ > 0)
        {
            eraseText(cursorPos_ - 1, 1);
            cursorPos_--;
        }
        break;

    case Key::Enter:
    {
        size_t start = lineStart(cursorPos_);
        size_t indentEnd = start;

        while (indentEnd < code_.size() && code_[indentEnd] == ' ')
            indentEnd++;

        string nl = "\n" + code_.substr(start, indentEnd - start);
        insertText(cursorPos_, nl);
        cursorPos_ += nl.size();
        break;
    }

    case Key::Delete:
        if (cursorPos_ < code_.size())
            eraseText(cursorPos_, 1);
        break;

    case Key::Left:
        if (cursorPos_ > 0)
            cursorPos_--;
        break;

    case Key::Right:
        if (cursorPos_ < code_.size())
            cursorPos_++;
        break;

    case Key::Up:
        moveUp();
        break;

    case Key::Down:
        moveDown();
        break;

    default:
        return false;
    }

    cursorClock_.restart();
    return true;
}

bool SoraEditor::handleEvent(const sf::Event &e)
{
    if (!open_)
        return false;

    // hover tracking (also lets you slide between menus while one is open)
    if (const auto *mv = e.getIf<sf::Event::MouseMoved>())
    {
        mouse_ = sf::Vector2f(mv->position);

        if (menuOpen_ >= 0)
        {
            for (int i = 0; i < static_cast<int>(menuNames_.size()); i++)
                if (headerRect(i).contains(mouse_))
                    menuOpen_ = i;
        }

        return false;
    }

    if (const auto *m = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (m->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f p(m->position);
            mouse_ = p;

            // menu headers
            for (int i = 0; i < static_cast<int>(menuNames_.size()); i++)
            {
                if (headerRect(i).contains(p))
                {
                    menuOpen_ = (menuOpen_ == i) ? -1 : i;
                    return true;
                }
            }

            // open dropdown: pick an item, or click away to close
            if (menuOpen_ >= 0)
            {
                int menu = menuOpen_;
                int hit = -1;

                for (int j = 0; j < static_cast<int>(menus_[menu].size()); j++)
                    if (!menus_[menu][j].sep && itemRect(menu, j).contains(p))
                        hit = j;

                menuOpen_ = -1;

                if (hit >= 0)
                    execute(menus_[menu][hit].cmd);

                return true;
            }

            if (closeBtn_.getGlobalBounds().contains(p))
            {
                close();
                return true;
            }

            if (buildBtn_.getGlobalBounds().contains(p))
            {
                build(true);
                return true;
            }

            if (p.x >= codeLeft && p.x <= codeRight &&
                p.y >= codeTop && p.y <= codeBottom)
                placeCursor(p);
        }

        return true; // modal: clicks never reach the game underneath
    }

    if (const auto *t = e.getIf<sf::Event::TextEntered>())
    {
        Snapshot before{code_, black_, cursorPos_};

        onText(t->unicode);

        if (code_ != before.code)
        {
            undo_.push_back(before);

            if (undo_.size() > 200)
                undo_.erase(undo_.begin());

            redo_.clear();
        }

        return true;
    }

    if (const auto *k = e.getIf<sf::Event::KeyPressed>())
    {
        using Key = sf::Keyboard::Key;

        if (menuOpen_ >= 0)
        {
            menuOpen_ = -1;

            if (k->code == Key::Escape)
                return true;
        }

        // undo / redo manage the stacks themselves
        if (k->control && k->code == Key::Z)
        {
            undo();
            return true;
        }

        if (k->control && k->code == Key::Y)
        {
            redo();
            return true;
        }

        Snapshot before{code_, black_, cursorPos_};

        onKey(*k);

        if (code_ != before.code)
        {
            undo_.push_back(before);

            if (undo_.size() > 200)
                undo_.erase(undo_.begin());

            redo_.clear();
        }

        return true;
    }

    if (const auto *wheel = e.getIf<sf::Event::MouseWheelScrolled>())
    {
        scrollOffset_ -= wheel->delta * 55.f;
        clampScroll();
        return true;
    }

    return false;
}

// =============================================================
// SoraEditor Scanlines
// =============================================================
void SoraEditor::rebuildScanlines()
{
    scanlines_.clear();
    const sf::Color c(0, 20, 60, 28);

    for (float y = 20.f; y <= 640.f; y += 2.f)
    {
        scanlines_.append(sf::Vertex({30.f, y}, c));
        scanlines_.append(sf::Vertex({1230.f, y}, c));
    }

    cachedTop_ = codeTop;
    cachedBottom_ = codeBottom;
    cachedLeft_ = codeLeft;
    cachedRight_ = codeRight;
}

void SoraEditor::drawScanlines(sf::RenderWindow &window)
{
    if (!open_ || !showScanlines_)
        return;

    if (scanlines_.getVertexCount() == 0 ||
        codeTop != cachedTop_ || codeBottom != cachedBottom_ ||
        codeLeft != cachedLeft_ || codeRight != cachedRight_)
        rebuildScanlines();

    window.draw(scanlines_);
}

// =============================================================
// DRAW
// =============================================================

void SoraEditor::draw(sf::RenderWindow &window)
{
    if (!open_)
        return;

    // keep the black flags valid even if code_ was changed from outside
    if (black_.size() != code_.size())
        black_.resize(code_.size(), 0);

    window.draw(background_);
    bevelBox(window, {{30.f, 20.f}, {1200.f, 620.f}}, Retro::Frame, true);
    gradientBar(window, {{32.f, 22.f}, {1196.f, 26.f}}, Retro::TitleTop, Retro::TitleBot);

    sf::Text title(*font_);
    title.setString("Disboard IDE - [anime.cpp]");
    title.setCharacterSize(13);
    title.setPosition({40.f, 27.f});
    title.setFillColor(Retro::Navy);
    window.draw(title);

    bevelBox(window, closeBtn_.getGlobalBounds(), Retro::Close, true);
    sf::Text x(*font_, "x", 13);
    x.setPosition({closeBtn_.getPosition().x + 4.f, closeBtn_.getPosition().y});
    x.setFillColor(sf::Color::White);
    window.draw(x);

    // sunken code panel behind the text
    bevelBox(window, {{40.f, 52.f}, {1175.f, 433.f}}, Retro::Panel, false);

    cursorPos_ = min(cursorPos_, code_.size());

    if (cursorPos_ != lastCursor_)
    {
        int cl = static_cast<int>(count(code_.begin(), code_.begin() + cursorPos_, '\n'));
        float top = cl * lineHeight;
        float viewH = codeBottom - codeTop;

        if (top < scrollOffset_)
            scrollOffset_ = top;
        else if (top + lineHeight > scrollOffset_ + viewH)
            scrollOffset_ = top + lineHeight - viewH;

        cursorClock_.restart();
        lastCursor_ = cursorPos_;
    }

    float drawX = codeLeft;
    float drawY = codeTop - scrollOffset_;

    auto drawRun = [&](const string &s, sf::Color color)
    {
        if (s.empty())
            return;

        if (drawY >= codeTop && drawY + lineHeight <= codeBottom && drawX < codeRight)
        {
            sf::Text text(*font_, s, 14);
            text.setFillColor(color);
            text.setPosition({drawX, drawY});
            window.draw(text);
        }

        drawX += static_cast<float>(s.size()) * cw_;
    };

    // draws code_[a, b): original intro characters in black, everything else in syntaxColor
    auto drawSpan = [&](size_t a, size_t b, sf::Color syntaxColor)
    {
        size_t j = a;

        while (j < b)
        {
            bool isBlack = showIntroBlack_ && black_[j] != 0;
            size_t k = j;

            while (k < b && (showIntroBlack_ && black_[k] != 0) == isBlack)
                k++;

            drawRun(code_.substr(j, k - j), isBlack ? Light::intro : syntaxColor);
            j = k;
        }
    };

    size_t i = 0;

    while (i < code_.size())
    {
        if (drawY >= codeBottom)
            break;

        char c = code_[i];

        if (c == '\n')
        {
            drawX = codeLeft;
            drawY += lineHeight;
            i++;
            continue;
        }

        // comment
        if (c == '/' && i + 1 < code_.size() && code_[i + 1] == '/')
        {
            size_t end = code_.find('\n', i);

            if (end == string::npos)
                end = code_.size();

            drawSpan(i, end, Light::comment);
            i = end;
            continue;
        }

        // string
        if (c == '"')
        {
            size_t eol = code_.find('\n', i);

            if (eol == string::npos)
                eol = code_.size();

            size_t end = code_.find('"', i + 1);

            if (end == string::npos || end > eol)
                end = (eol > i) ? eol - 1 : i;

            drawSpan(i, end + 1, Light::str);
            i = end + 1;
            continue;
        }

        // identifier
        if (isalpha(static_cast<unsigned char>(c)) || c == '_')
        {
            size_t start = i;

            while (i < code_.size() &&
                   (isalnum(static_cast<unsigned char>(code_[i])) || code_[i] == '_'))
                i++;

            string token = code_.substr(start, i - start);

            sf::Color color = isKeyword(token)   ? Light::keyword
                              : isBuiltin(token) ? Light::builtin
                                                 : Light::text;
            drawSpan(start, i, color);
            continue;
        }

        // number
        if (isdigit(static_cast<unsigned char>(c)))
        {
            size_t start = i;

            while (i < code_.size() &&
                   (isdigit(static_cast<unsigned char>(code_[i])) || code_[i] == '.'))
                i++;

            drawSpan(start, i, Light::number);
            continue;
        }

        drawSpan(i, i + 1, Light::text);
        i++;
    }

    // cursor (blinks)
    if (fmod(cursorClock_.getElapsedTime().asSeconds(), 1.f) < 0.6f)
    {
        size_t start = lineStart(cursorPos_);
        size_t column = cursorPos_ - start;
        size_t lineNumber = count(code_.begin(), code_.begin() + start, '\n');

        float cx = codeLeft + static_cast<float>(column) * cw_;
        float cy = codeTop + static_cast<float>(lineNumber) * lineHeight - scrollOffset_;

        if (cy >= codeTop && cy < codeBottom)
        {
            cursor_.setPosition({cx, cy});
            window.draw(cursor_);
        }
    }

    // build button + status strip
    bevelBox(window, buildBtn_.getGlobalBounds(), Retro::Mint, true);
    sf::Text buildText(*font_, "BUILD", 18);
    buildText.setPosition({590.f, 498.f});
    buildText.setFillColor(Retro::Navy);
    window.draw(buildText);

    sf::RectangleShape status({495.f, 40.f});
    status.setPosition({40.f, 490.f});
    status.setFillColor(Retro::Navy);
    window.draw(status);
    sf::Text st(*font_, "anime.cpp    F5 = build", 14);
    st.setPosition({50.f, 500.f});
    st.setFillColor(Retro::TitleTop);
    window.draw(st);

    // cream output panel
    bevelBox(window, {{40.f, 538.f}, {1175.f, 92.f}}, Retro::Cream, false);
    sf::Text outText(*font_, buildOutput_.substr(0, 300), 12);
    outText.setFillColor(buildOk_ ? sf::Color(20, 110, 60) : sf::Color(190, 30, 60));
    outText.setPosition({50.f, 545.f});
    window.draw(outText);

    // menu bar and dropdown go last so they sit on top of everything
    drawMenus(window);
}