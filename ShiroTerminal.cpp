#include "ShiroTerminal.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace std;
using std::to_string;




ShiroTerminal::ShiroTerminal(
    const sf::Font &monoFont,
    const sf::Font &displayFont)
    : mono(&monoFont), display(&displayFont)
{
    lines = {
        "// ATTACK",
        "",
        "int bullets = 5;",
        "float speed = 8.0f;",
        "float spread = 20.0f;",
        "",
        "// Ctrl+Enter or BUILD to execute"};

    cw = mono->getGlyph(U'M', 14, false).advance;
    parse();
    applied_ = pending_;
}
bool ShiroTerminal::focused() const
{
    return focused_;
}

void ShiroTerminal::blur()
{
    focused_ = false;
}

void ShiroTerminal::setRank(char c)
{
    rank_ = c;
}

const AttackParams &ShiroTerminal::pending() const
{
    return pending_;
}

const AttackParams &ShiroTerminal::applied() const
{
    return applied_;
}
void ShiroTerminal::update(float dt, sf::Vector2f mouse, bool busy)
{
    blink += dt;
    mouse_ = mouse;
    busy_ = busy;
    buildGlow = max(0.f, buildGlow - dt * 3.f);
    denyFlash = max(0.f, denyFlash - dt * 2.5f);
}
bool ShiroTerminal::handleEvent(const sf::Event &e)
{
    if (const auto *m = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (m->button != sf::Mouse::Button::Left)
            return false;

        sf::Vector2f p(m->position);

        if (buildRect.contains(p))
            return tryBuild();

        if (codeRect.contains(p))
        {
            focused_ = true;
            placeCursor(p);
        }
        else
        {
            focused_ = false;
        }

        return false;
    }

    if (const auto *wheel = e.getIf<sf::Event::MouseWheelScrolled>())
    {
        if (codeRect.contains(mouse_))
        {
            scrollRow -= (wheel->delta > 0.f) ? 2 : -2;
            clampScroll();
        }

        return false;
    }

    if (const auto *k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::F5)
            return tryBuild();

        if (!focused_)
            return false;

        if (k->code == sf::Keyboard::Key::Escape)
        {
            focused_ = false;
            return false;
        }

        if (k->control && k->code == sf::Keyboard::Key::Enter)
            return tryBuild();

        if (k->control && k->code == sf::Keyboard::Key::V)
        {
            paste();
            return false;
        }

        switch (k->code)
        {
        case sf::Keyboard::Key::Enter:
            newline();
            break;

        case sf::Keyboard::Key::Backspace:
            backspace();
            break;

        case sf::Keyboard::Key::Delete:
            del();
            break;

        case sf::Keyboard::Key::Tab:
            for (int i = 0; i < 4; i++)
                insertChar(' ');
            break;

        case sf::Keyboard::Key::Left:
            if (col > 0)
                col--;
            else if (row > 0)
            {
                row--;
                col = len(row);
            }
            moved();
            break;

        case sf::Keyboard::Key::Right:
            if (col < len(row))
                col++;
            else if (row + 1 < static_cast<int>(lines.size()))
            {
                row++;
                col = 0;
            }
            moved();
            break;

        case sf::Keyboard::Key::Up:
            if (row > 0)
            {
                row--;
                col = min(col, len(row));
            }
            moved();
            break;

        case sf::Keyboard::Key::Down:
            if (row + 1 < static_cast<int>(lines.size()))
            {
                row++;
                col = min(col, len(row));
            }
            moved();
            break;

        case sf::Keyboard::Key::Home:
            col = 0;
            moved();
            break;

        case sf::Keyboard::Key::End:
            col = len(row);
            moved();
            break;

        default:
            break;
        }

        return false;
    }

    if (focused_)
    {
        if (const auto *t = e.getIf<sf::Event::TextEntered>())
        {
            if (t->unicode >= 32 && t->unicode < 127)
                insertChar(static_cast<char>(t->unicode));
        }
    }

    return false;
}
// ---------------- small helpers ----------------
sf::Color ShiroTerminal::alpha(sf::Color c, float a)
{
    c.a = static_cast<std::uint8_t>(clamp(a, 0.f, 255.f));
    return c;
}

string ShiroTerminal::fmt(float v)
{
    char b[24];
    snprintf(b, sizeof b, "%.1f", v);
    return b;
}

string ShiroTerminal::trim(const string &s)
{
    size_t a = s.find_first_not_of(" \t");
    if (a == string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t");
    return s.substr(a, b - a + 1);
}

int ShiroTerminal::len(int r) const
{
    return static_cast<int>(lines[r].size());
}

void ShiroTerminal::box(sf::RenderWindow &w, float x, float y, float wd, float h,
                        sf::Color fill, sf::Color outline, float thick) const
{
    sf::RectangleShape r({wd, h});
    r.setPosition({x, y});
    r.setFillColor(fill);

    if (thick > 0.f)
    {
        r.setOutlineColor(outline);
        r.setOutlineThickness(thick);
    }
    w.draw(r);
}

void ShiroTerminal::text(sf::RenderWindow &w, const string &s, float x, float y,
                         unsigned size, sf::Color c, int align, bool useDisplay) const
{
    sf::Text t(useDisplay ? *display : *mono, s, size);
    t.setFillColor(c);

    sf::FloatRect b = t.getLocalBounds();
    float ox = b.position.x;

    if (align == 1)
        ox += b.size.x / 2.f;
    else if (align == 2)
        ox += b.size.x;

    t.setOrigin({ox, 0.f});
    t.setPosition({x, y});
    w.draw(t);
}

void ShiroTerminal::drawCode(sf::RenderWindow &w, const string &s, float x, float y) const
{
    size_t i = 0;

    while (i < s.size())
    {
        if (s[i] == ' ')
        {
            i++;
            continue;
        }

        size_t start = i;
        sf::Color c(215, 225, 235);

        if (s[i] == '/' && i + 1 < s.size() && s[i + 1] == '/')
        {
            i = s.size();
            c = Syntax::comment;
        }
        else if (isalpha(static_cast<unsigned char>(s[i])) || s[i] == '_')
        {
            while (i < s.size() &&
                   (isalnum(static_cast<unsigned char>(s[i])) || s[i] == '_'))
                i++;

            string tok = s.substr(start, i - start);

            if (isKeyword(tok))
                c = Syntax::keyword;
            else if (tok == "bullets" || tok == "speed" || tok == "spread")
                c = cyan;
        }
        else if (isdigit(static_cast<unsigned char>(s[i])))
        {
            while (i < s.size() &&
                   (isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.'))
                i++;

            if (i < s.size() && (s[i] == 'f' || s[i] == 'F'))
                i++;

            c = Syntax::orange;
        }
        else
        {
            i++;
            c = sf::Color(120, 150, 175);
        }

        text(w, s.substr(start, i - start), x + start * cw, y + 1, 14, c);
    }
}

// ---------------- editing ----------------
void ShiroTerminal::edited()
{
    parse();
    moved();
}

void ShiroTerminal::moved()
{
    blink = 0.f;
    ensureVisible();
}

void ShiroTerminal::ensureVisible()
{
    if (row < scrollRow)
        scrollRow = row;
    if (row >= scrollRow + visibleRows)
        scrollRow = row - visibleRows + 1;
    clampScroll();
}

void ShiroTerminal::clampScroll()
{
    int maxScroll = max(0, static_cast<int>(lines.size()) - visibleRows);
    scrollRow = clamp(scrollRow, 0, maxScroll);
}

void ShiroTerminal::insertChar(char c)
{
    if (len(row) >= maxCols)
        return;

    lines[row].insert(col, 1, c);
    col++;
    edited();
}

void ShiroTerminal::newline()
{
    if (static_cast<int>(lines.size()) >= maxRows)
        return;

    size_t indentPos = lines[row].find_first_not_of(' ');
    int indent = (indentPos == string::npos) ? len(row) : static_cast<int>(indentPos);
    indent = min(indent, col);

    string tail = lines[row].substr(col);
    lines[row].erase(col);
    lines.insert(lines.begin() + row + 1, string(indent, ' ') + tail);

    row++;
    col = indent;
    edited();
}

void ShiroTerminal::backspace()
{
    if (col > 0)
    {
        lines[row].erase(col - 1, 1);
        col--;
    }
    else if (row > 0)
    {
        int prevLen = len(row - 1);
        if (prevLen + len(row) > maxCols)
            return;

        lines[row - 1] += lines[row];
        lines.erase(lines.begin() + row);
        row--;
        col = prevLen;
    }
    edited();
}

void ShiroTerminal::del()
{
    if (col < len(row))
    {
        lines[row].erase(col, 1);
    }
    else if (row + 1 < static_cast<int>(lines.size()))
    {
        if (len(row) + len(row + 1) > maxCols)
            return;

        lines[row] += lines[row + 1];
        lines.erase(lines.begin() + row + 1);
    }
    edited();
}

void ShiroTerminal::paste()
{
    string s = getClipboardText();
    s.erase(remove(s.begin(), s.end(), '\r'), s.end());

    for (char c : s)
    {
        if (c == '\n')
            newline();
        else if (c == '\t')
        {
            for (int i = 0; i < 4; i++)
                insertChar(' ');
        }
        else if (c >= 32 && c < 127)
            insertChar(c);
    }
}

void ShiroTerminal::placeCursor(sf::Vector2f p)
{
    int r = scrollRow + static_cast<int>((p.y - textTop) / lineH);
    r = clamp(r, 0, static_cast<int>(lines.size()) - 1);

    int c = static_cast<int>(std::lround((p.x - textLeft) / cw));
    c = clamp(c, 0, len(r));

    row = r;
    col = c;
    blink = 0.f;
}

bool ShiroTerminal::tryBuild()
{
    if (busy_)
        return false;

    if (hasError)
    {
        denyFlash = 1.f;
        return false;
    }

    applied_ = pending_;
    focused_ = false;
    buildGlow = 1.f;
    return true;
}

// ---------------- parsing ----------------
void ShiroTerminal::addDiag(int line, const string &msg, bool error)
{
    diags.push_back({line, msg, error});
    if (error)
        hasError = true;
}

void ShiroTerminal::parse()
{
    diags.clear();
    hasError = false;

    AttackParams p = pending_; // lines with errors keep the last good value

    for (int i = 0; i < static_cast<int>(lines.size()); i++)
    {
        string s = lines[i];

        size_t c = s.find("//");
        if (c != string::npos)
            s.erase(c);

        s = trim(s);
        if (s.empty())
            continue;

        if (s.back() != ';')
        {
            addDiag(i, "missing ';'", true);
            continue;
        }
        s.pop_back();

        size_t eq = s.find('=');
        if (eq == string::npos)
        {
            addDiag(i, "expected 'name = value;'", true);
            continue;
        }

        string left = trim(s.substr(0, eq));
        string right = trim(s.substr(eq + 1));

        size_t sp = left.find_last_of(" \t");
        string name = (sp == string::npos) ? left : left.substr(sp + 1);

        if (!right.empty() && (right.back() == 'f' || right.back() == 'F'))
            right.pop_back();

        char *endp = nullptr;
        float v = strtof(right.c_str(), &endp);

        if (right.empty() || *endp != '\0' || !std::isfinite(v))
        {
            addDiag(i, "'" + right + "' is not a number", true);
            continue;
        }

        if (name == "bullets")
        {
            int b = static_cast<int>(lround(clamp(v, -1000.f, 1000.f)));
            if (b < 1 || b > 50)
            {
                addDiag(i, "bullets limited to 1-50", false);
                b = clamp(b, 1, 50);
            }
            p.bullets = b;
        }
        else if (name == "speed")
        {
            if (v < 0.1f || v > 30.f)
            {
                addDiag(i, "speed limited to 0.1-30", false);
                v = clamp(v, 0.1f, 30.f);
            }
            p.speed = v;
        }
        else if (name == "spread")
        {
            if (v < 0.f || v > 360.f)
            {
                addDiag(i, "spread limited to 0-360", false);
                v = clamp(v, 0.f, 360.f);
            }
            p.spread = v;
        }
        else
        {
            addDiag(i, "unknown variable '" + name + "'", true);
        }
    }

    pending_ = p;
}

// ---------------- draw ----------------
void ShiroTerminal::draw(sf::RenderWindow &w) const
{
    const bool changed = !(pending_ == applied_);

    const Diag *firstErr = nullptr;
    const Diag *firstWarn = nullptr;
    for (const auto &d : diags)
    {
        if (d.error && !firstErr)
            firstErr = &d;
        if (!d.error && !firstWarn)
            firstWarn = &d;
    }

    // panel + header
    box(w, 730, 70, 532, 590, sf::Color(2, 6, 13, 248),
        focused_ ? pink : alpha(pink, 110), 2.f);

    text(w, "ROOT // SHIRO_TERMINAL", 748, 84, 12, pink);
    text(w, string("attack.cpp") + (changed ? " *" : ""), 925, 86, 10,
         changed ? gold : dim);
    text(w, focused_ ? "EDITING - TIME SLOWED" : "CLICK CODE TO EDIT",
         1246, 85, 10, focused_ ? gold : dim, 2);
    box(w, 742, 105, 504, 1, alpha(pink, 100));

    // code box
    box(w, 746, 118, 500, 300, sf::Color(1, 4, 10),
        focused_ ? alpha(cyan, 200) : sf::Color(20, 85, 110, 130), 1.f);
    box(w, 746, 118, 42, 300, sf::Color(3, 9, 17));

    if (focused_ && row >= scrollRow && row < scrollRow + visibleRows)
        box(w, 788, textTop + (row - scrollRow) * lineH - 1, 458, lineH,
            alpha(cyan, 16));

    int last = min(static_cast<int>(lines.size()), scrollRow + visibleRows);

    for (int r = scrollRow; r < last; r++)
    {
        float y = textTop + (r - scrollRow) * lineH;
        bool cur = focused_ && r == row;

        text(w, to_string(r + 1), 780, y + 3, 10,
             cur ? cyan : sf::Color(45, 85, 110), 2);

        for (const auto &d : diags)
        {
            if (d.line != r)
                continue;

            sf::Color c = d.error ? red : amber;
            box(w, 748, y + 3, 3, 14, c);

            if (d.error)
                box(w, textLeft, y + lineH - 3,
                    max(1.f, len(r) * cw), 1, alpha(red, 200));
        }

        drawCode(w, lines[r], textLeft, y);
    }

    if (focused_ && row >= scrollRow && row < scrollRow + visibleRows &&
        std::fmod(blink, 1.f) < 0.6f)
    {
        box(w, textLeft + col * cw,
            textTop + (row - scrollRow) * lineH + 1, 2, 16, cyan);
    }

    if (static_cast<int>(lines.size()) > visibleRows)
    {
        float total = static_cast<float>(lines.size());
        float th = 292.f * visibleRows / total;
        float ty = 122.f + 292.f * scrollRow / total;
        box(w, 1240, 122, 3, 292, sf::Color(10, 25, 35));
        box(w, 1240, ty, 3, th, alpha(cyan, 140));
    }

    // diagnostics strip
    sf::Color stripBorder = denyFlash > 0.f ? alpha(red, 255 * denyFlash)
                                            : sf::Color(0, 190, 220, 70);
    box(w, 746, 424, 500, 28, sf::Color(3, 8, 15), stripBorder, 1.f);

    if (firstErr)
        text(w, "ERR   line " + to_string(firstErr->line + 1) + ": " + firstErr->msg,
             758, 432, 10, red);
    else if (firstWarn)
        text(w, "WARN  line " + to_string(firstWarn->line + 1) + ": " + firstWarn->msg,
             758, 432, 10, amber);
    else
        text(w, "OK    compiles clean", 758, 432, 10, teal);

    // build button
    bool hover = buildRect.contains(mouse_);
    string label, sub;
    sf::Color fill, border, labelCol, subCol;

    if (busy_)
    {
        label = "CASTING...";
        sub = "SHIRO IS CHARGING THE SPELL";
        fill = sf::Color(30, 22, 8);
        border = amber;
        labelCol = amber;
        subCol = sf::Color(150, 110, 60);
    }
    else if (hasError)
    {
        label = "FIX ERRORS";
        sub = "BUILD DISABLED - SEE LINE " + to_string(firstErr ? firstErr->line + 1 : 0);
        fill = sf::Color(24, 8, 14);
        border = sf::Color(120, 40, 60);
        labelCol = sf::Color(170, 90, 100);
        subCol = sf::Color(110, 60, 70);
    }
    else
    {
        label = "BUILD // EXECUTE";
        sub = to_string(pending_.bullets) + " BULLETS  |  SPREAD " +
              to_string(static_cast<int>(pending_.spread)) + " DEG  |  CTRL+ENTER";
        fill = hover ? sf::Color(44, 10, 32) : sf::Color(6, 10, 20);
        border = hover ? sf::Color(255, 90, 170) : pink;
        labelCol = sf::Color(245, 245, 255);
        subCol = sf::Color(110, 155, 175);
    }

    box(w, 746, 460, 500, 66, fill, border, 2.f);
    text(w, label, 996, 472, 20, labelCol, 1, true);
    text(w, sub, 996, 504, 9, subCol, 1);

    if (buildGlow > 0.f)
        box(w, 746, 460, 500, 66, alpha(sf::Color::White, buildGlow * 120.f));

    // telemetry
    box(w, 746, 545, 500, 88, sf::Color(3, 8, 15, 230), sf::Color(0, 190, 220, 80), 1.f);
    text(w, "TELEMETRY", 760, 553, 10, cyan);
    text(w, string("RANK  ") + rank_, 850, 551, 13, letterColor(rank_));
    text(w, "BAR = BUILT   PINK TICK = TYPED, NOT BUILT", 1236, 555, 8, dim, 2);

    auto stat = [&](float x, const string &name, const string &value,
                    const string &pendValue, bool differs,
                    float fa, float fp, sf::Color c)
    {
        text(w, name, x, 574, 9, dim);
        text(w, value, x, 586, 16, c);

        if (differs)
            text(w, "> " + pendValue, x + 72, 592, 10, pink);

        box(w, x, 614, 140, 5, sf::Color(10, 22, 32));
        box(w, x, 614, 140.f * clamp(fa, 0.f, 1.f), 5, alpha(c, 200));

        if (differs)
            box(w, x + 140.f * clamp(fp, 0.f, 1.f) - 1.f, 611, 2, 11, pink);
    };

    stat(760, "BULLETS  1-50",
         to_string(applied_.bullets), to_string(pending_.bullets),
         applied_.bullets != pending_.bullets,
         (applied_.bullets - 1) / 49.f, (pending_.bullets - 1) / 49.f, cyan);

    stat(920, "SPEED  0.1-30",
         fmt(applied_.speed), fmt(pending_.speed),
         applied_.speed != pending_.speed,
         (applied_.speed - 0.1f) / 29.9f, (pending_.speed - 0.1f) / 29.9f, teal);

    stat(1080, "SPREAD  0-360",
         to_string(static_cast<int>(applied_.spread)),
         to_string(static_cast<int>(pending_.spread)),
         applied_.spread != pending_.spread,
         applied_.spread / 360.f, pending_.spread / 360.f, gold);

    text(w, "ESC release   |   CTRL+ENTER / F5 build   |   click code to edit",
         748, 641, 9, dim);
}