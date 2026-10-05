#define NOMINMAX
#include "soraeditor.h"

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

SoraEditor::SoraEditor(const sf::Font &monoFont) : font_(&monoFont)
{
    cw_ = font_->getGlyph(U'M', 14, false).advance;

    background_.setSize({1200.f, 620.f});
    background_.setPosition({30.f, 20.f});
    background_.setFillColor(sf::Color(32, 32, 31, 250));

    titleBar_.setSize({1200.f, 28.f});
    titleBar_.setPosition({30.f, 20.f});
    titleBar_.setFillColor(sf::Color::Blue);

    closeBtn_.setSize({16.f, 16.f});
    closeBtn_.setPosition({1198.f, 26.f});
    closeBtn_.setFillColor(sf::Color(220, 50, 50));

    buildBtn_.setSize({150.f, 40.f});
    buildBtn_.setPosition({545.f, 490.f});
    buildBtn_.setFillColor(sf::Color(0, 110, 180));
    buildBtn_.setOutlineColor(sf::Color(0, 0, 255));
    buildBtn_.setOutlineThickness(1.f);

    cursor_.setSize({2.f, 16.f});
    cursor_.setFillColor(sf::Color(0, 220, 255));
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

    cursorPos_ = 0;
}

void SoraEditor::open()
{
    open_ = true;
    scrollOffset_ = 0.f;
    cursorPos_ = 0;
    lastCursor_ = static_cast<size_t>(-1);
    cursorClock_.restart();
}

void SoraEditor::close()
{
    open_ = false;
}

// =============================================================
// SAVE / BUILD
// =============================================================

bool SoraEditor::save() const
{
    ofstream out("test.cpp");

    if (!out.is_open())
        return false;

    out << code_;
    return true;
}

bool SoraEditor::build()
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

    // test code may use cin, so give it its own console window
    system("start \"\" cmd /k test.exe");
    return true;
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

    code_.insert(cursorPos_, formatted);
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
        code_.insert(cursorPos_, 1, '\n');
        cursorPos_++;
    }

    code_.insert(cursorPos_, 1, static_cast<char>(unicode));
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

    switch (k.code)
    {
    case Key::F5:
        build();
        break;

    case Key::Home:
        cursorPos_ = lineStart(cursorPos_);
        break;

    case Key::End:
    {
        size_t end = code_.find('\n', cursorPos_);
        cursorPos_ = (end == string::npos) ? code_.size() : end;
        break;
    }

    case Key::Tab:
        code_.insert(cursorPos_, "    ");
        cursorPos_ += 4;
        break;

    case Key::Backspace:
        if (cursorPos_ > 0)
        {
            code_.erase(cursorPos_ - 1, 1);
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
        code_.insert(cursorPos_, nl);
        cursorPos_ += nl.size();
        break;
    }

    case Key::Delete:
        if (cursorPos_ < code_.size())
            code_.erase(cursorPos_, 1);
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

    if (const auto *m = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (m->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f p(m->position);

            if (closeBtn_.getGlobalBounds().contains(p))
            {
                close();
                return true;
            }

            if (buildBtn_.getGlobalBounds().contains(p))
            {
                build();
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
        onText(t->unicode);
        return true;
    }

    if (const auto *k = e.getIf<sf::Event::KeyPressed>())
    {
        onKey(*k);
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
// DRAW
// =============================================================

void SoraEditor::draw(sf::RenderWindow &window)
{
    if (!open_)
        return;

    window.draw(background_);
    window.draw(titleBar_);

    sf::Text title(*font_);
    title.setString("Disboard IDE - [anime.cpp]");
    title.setCharacterSize(13);
    title.setPosition({30.f, 25.f});
    title.setFillColor(sf::Color::White);
    window.draw(title);

    window.draw(closeBtn_);

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

            drawRun(code_.substr(i, end - i), Syntax::comment);
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

            drawRun(code_.substr(i, end - i + 1), Syntax::green);
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

            sf::Color color = isKeyword(token)   ? Syntax::keyword
                              : isBuiltin(token) ? Syntax::orange
                                                 : Syntax::text;
            drawRun(token, color);
            continue;
        }

        // number
        if (isdigit(static_cast<unsigned char>(c)))
        {
            size_t start = i;

            while (i < code_.size() &&
                   (isdigit(static_cast<unsigned char>(code_[i])) || code_[i] == '.'))
                i++;

            drawRun(code_.substr(start, i - start), Syntax::orange);
            continue;
        }

        drawRun(string(1, c), Syntax::text);
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

    // build button
    window.draw(buildBtn_);

    sf::Text buildText(*font_, "BUILD", 18);
    buildText.setPosition({590.f, 498.f});
    window.draw(buildText);

    sf::Text outText(*font_, buildOutput_.substr(0, 300), 12);
    outText.setFillColor(buildOk_ ? sf::Color(0, 235, 190) : sf::Color(255, 90, 110));
    outText.setPosition({45.f, 540.f});
    window.draw(outText);
}