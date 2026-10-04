#include "ShiroTerminal.h"

#include <cstdlib>

using std::remove;
using std::strtof;
using std::to_string;

bool isKeyword(const string &token)
{
    static const vector<string> keywords = {
        "include", "int", "bool", "float", "char", "double", "void", "string",
        "using", "namespace", "if", "else", "while", "for", "return", "true", "false",
        "const", "static", "auto", "continue", "break", "class", "struct"};

    for (const auto &k : keywords)
    {
        if (token == k)
            return true;
    }

    return false;
}

string getClipboardText();
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