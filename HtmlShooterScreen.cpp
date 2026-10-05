#include "HtmlShooterScreen.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <utility>

using namespace std;

bool HtmlShooterScreen::takeWin()
{
    if (!active_ && outcome_ == 1)
    {
        outcome_ = 0;
        return true;
    }
    return false;
}

void HtmlShooterScreen::open(const sf::Font &mono)
{
    cw = mono.getGlyph(U'M', 14, false).advance;

    active_ = true;
    ended_ = false;
    outcome_ = 0;
    t_ = 0.f;

    playerPos = {fx + fw / 2.f, fy + fh - 60.f};
    playerHP = 100;
    bossHP = bossMax;
    invuln = 0.f;
    hurt = 0.f;
    fireTimer = 0.f;
    ringTimer = 1.f;
    aimTimer = 1.f;
    ringSpin = 0.f;
    mine.clear();
    theirs.clear();

    lines = {"<ship speed=\"6\">",
             "<shot count=\"3\" spread=\"24\" speed=\"10\">",
             "<rate ms=\"250\">",
             ""};

    row = col = 0;
    focused = false;
    pending = Params();
    parse();
    applied = pending;
}

void HtmlShooterScreen::update(float dt)
{
    if (!active_)
        return;

    t_ += dt;
    blink += dt;
    hurt = max(0.f, hurt - dt * 3.f);
    buildGlow = max(0.f, buildGlow - dt * 3.f);
    denyFlash = max(0.f, denyFlash - dt * 2.5f);

    if (ended_)
        return;

    float g = focused ? dt * 0.3f : dt;
    invuln = max(0.f, invuln - g);

    // ---- player ----
    if (!focused)
    {
        sf::Vector2f d{0.f, 0.f};
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            d.y -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            d.y += 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            d.x -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            d.x += 1.f;

        float l = std::sqrt(d.x * d.x + d.y * d.y);
        if (l > 0.f)
            d /= l;

        playerPos += d * applied.shipSpeed * 60.f * g;
    }

    playerPos.x = clamp(playerPos.x, fx + 12.f, fx + fw - 12.f);
    playerPos.y = clamp(playerPos.y, fy + 12.f, fy + fh - 12.f);

    // ---- auto fire ----
    fireTimer -= g * 1000.f;
    if (fireTimer <= 0.f)
    {
        fireTimer = applied.rateMs;

        int n = applied.count;
        float step = (n > 1) ? applied.spread / (n - 1) : 0.f;
        float start = -applied.spread / 2.f;

        for (int i = 0; i < n; i++)
        {
            float rad = (-90.f + start + step * i) * 3.14159265f / 180.f;
            mine.push_back({playerPos + sf::Vector2f(0.f, -14.f),
                            {std::cos(rad) * applied.shotSpeed * 60.f,
                             std::sin(rad) * applied.shotSpeed * 60.f}});
        }
    }

    // ---- boss ----
    bossPos = {fx + fw / 2.f + std::sin(t_ * 0.9f) * 240.f, fy + 110.f};

    ringTimer -= g;
    if (ringTimer <= 0.f)
    {
        ringTimer = 1.6f;
        ringSpin += 0.21f;

        for (int i = 0; i < 12; i++)
        {
            float a = ringSpin + i * 6.2831853f / 12.f;
            theirs.push_back({bossPos, {std::cos(a) * 170.f, std::sin(a) * 170.f}});
        }
    }

    if (bossHP <= bossMax * 6 / 10)
    {
        aimTimer -= g;
        if (aimTimer <= 0.f)
        {
            aimTimer = 0.9f;

            float base = std::atan2(playerPos.y - bossPos.y, playerPos.x - bossPos.x);
            for (int k = -1; k <= 1; k++)
            {
                float a = base + k * 0.22f;
                theirs.push_back({bossPos, {std::cos(a) * 230.f, std::sin(a) * 230.f}});
            }
        }
    }

    // ---- my bullets ----
    for (size_t i = 0; i < mine.size();)
    {
        mine[i].pos += mine[i].vel * g;

        sf::Vector2f p = mine[i].pos;
        float dx = p.x - bossPos.x;
        float dy = p.y - bossPos.y;

        bool hit = dx * dx + dy * dy < 40.f * 40.f;
        bool off = p.x < fx || p.x > fx + fw || p.y < fy || p.y > fy + fh;

        if (hit)
            bossHP--;

        if (hit || off)
        {
            mine.erase(mine.begin() + i);
            continue;
        }
        i++;
    }

    // ---- their bullets ----
    for (size_t i = 0; i < theirs.size();)
    {
        theirs[i].pos += theirs[i].vel * g;

        sf::Vector2f p = theirs[i].pos;
        float dx = p.x - playerPos.x;
        float dy = p.y - playerPos.y;

        bool hit = invuln <= 0.f && dx * dx + dy * dy < 11.f * 11.f;
        bool off = p.x < fx - 20.f || p.x > fx + fw + 20.f ||
                   p.y < fy - 20.f || p.y > fy + fh + 20.f;

        if (hit)
        {
            playerHP -= 8;
            invuln = 1.f;
            hurt = 1.f;
        }

        if (hit || off)
        {
            theirs.erase(theirs.begin() + i);
            continue;
        }
        i++;
    }

    if (bossHP <= 0)
    {
        bossHP = 0;
        ended_ = true;
        outcome_ = 1;
        theirs.clear();
    }
    else if (playerHP <= 0)
    {
        playerHP = 0;
        ended_ = true;
        outcome_ = 2;
    }
}

void HtmlShooterScreen::handleEvent(const sf::Event &e)
{
    if (!active_)
        return;

    // keeps the BUILD button hover highlight working
    if (const auto *mm = e.getIf<sf::Event::MouseMoved>())
    {
        mouse = sf::Vector2f(mm->position);
        return;
    }

    if (const auto *m = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (m->button != sf::Mouse::Button::Left)
            return;

        sf::Vector2f p(m->position);

        if (buildRect.contains(p))
        {
            build();
            return;
        }

        if (codeRect.contains(p))
        {
            focused = true;

            int r = static_cast<int>((p.y - textTop) / lineH);
            row = clamp(r, 0, static_cast<int>(lines.size()) - 1);
            col = clamp(static_cast<int>(std::lround((p.x - textLeft) / cw)), 0, len(row));
            blink = 0.f;
        }
        else
        {
            focused = false;
        }
        return;
    }

    if (const auto *k = e.getIf<sf::Event::KeyPressed>())
    {
        if (ended_)
        {
            bool confirm = k->code == sf::Keyboard::Key::E ||
                           k->code == sf::Keyboard::Key::Enter ||
                           k->code == sf::Keyboard::Key::Space;
            if (confirm)
                active_ = false;
            return;
        }

        if (k->code == sf::Keyboard::Key::F5)
        {
            build();
            return;
        }

        if (k->code == sf::Keyboard::Key::Escape)
        {
            if (focused)
                focused = false;
            else
            {
                outcome_ = 2;
                active_ = false;
            }
            return;
        }

        if (!focused)
            return;

        if (k->control && k->code == sf::Keyboard::Key::Enter)
        {
            build();
            return;
        }

        switch (k->code)
        {
        case sf::Keyboard::Key::Enter:
            if (static_cast<int>(lines.size()) < maxRows)
            {
                string tail = lines[row].substr(col);
                lines[row].erase(col);
                lines.insert(lines.begin() + row + 1, tail);
                row++;
                col = 0;
                parse();
            }
            break;
        case sf::Keyboard::Key::Backspace:
            if (col > 0)
            {
                lines[row].erase(col - 1, 1);
                col--;
            }
            else if (row > 0)
            {
                int prev = len(row - 1);
                if (prev + len(row) <= maxCols)
                {
                    lines[row - 1] += lines[row];
                    lines.erase(lines.begin() + row);
                    row--;
                    col = prev;
                }
            }
            parse();
            break;
        case sf::Keyboard::Key::Left:
            if (col > 0)
                col--;
            else if (row > 0)
            {
                row--;
                col = len(row);
            }
            break;
        case sf::Keyboard::Key::Right:
            if (col < len(row))
                col++;
            else if (row + 1 < static_cast<int>(lines.size()))
            {
                row++;
                col = 0;
            }
            break;
        case sf::Keyboard::Key::Up:
            if (row > 0)
            {
                row--;
                col = min(col, len(row));
            }
            break;
        case sf::Keyboard::Key::Down:
            if (row + 1 < static_cast<int>(lines.size()))
            {
                row++;
                col = min(col, len(row));
            }
            break;
        default:
            break;
        }
        blink = 0.f;
        return;
    }

    if (focused)
    {
        if (const auto *te = e.getIf<sf::Event::TextEntered>())
        {
            if (te->unicode >= 32 && te->unicode < 127 && len(row) < maxCols)
            {
                lines[row].insert(col, 1, static_cast<char>(te->unicode));
                col++;
                blink = 0.f;
                parse();
            }
        }
    }
}

void HtmlShooterScreen::draw(sf::RenderWindow &w, const sf::Font &display, const sf::Font &mono) const
{
    if (!active_)
        return;

    w.setView(w.getDefaultView());

    box(w, 0.f, 0.f, 1280.f, 720.f, sf::Color(6, 8, 20));

    // ---------------- header ----------------
    put(w, display, "HTML WORLD SHOOTER", 24.f, 14.f, 28, sf::Color(240, 200, 110));
    put(w, mono, "IZUNA // WEB DISTRICT", 1256.f, 24.f, 12, sf::Color(190, 110, 235), 2);

    // HP bars
    box(w, fx, 54.f, 300.f, 8.f, sf::Color(14, 12, 34), sf::Color(40, 200, 210), 1.f);
    box(w, fx, 54.f, 300.f * playerHP / 100.f, 8.f, sf::Color(40, 200, 210));
    box(w, fx + fw - 300.f, 54.f, 300.f, 8.f, sf::Color(14, 12, 34), sf::Color(255, 0, 127), 1.f);
    float bf = static_cast<float>(bossHP) / bossMax;
    box(w, fx + fw - 300.f * bf, 54.f, 300.f * bf, 8.f, sf::Color(255, 0, 127));

    // ---------------- play field ----------------
    box(w, fx, fy, fw, fh, sf::Color(4, 6, 16), sf::Color(0, 190, 220, 120), 2.f);

    for (int i = 0; i < 18; i++) // scrolling "page grid"
    {
        float y = fy + std::fmod(i * 40.f + t_ * 60.f, fh);
        box(w, fx, y, fw, 1.f, sf::Color(20, 40, 70, 120));
    }

    // boss
    {
        sf::CircleShape c(34.f);
        c.setOrigin({34.f, 34.f});
        c.setPosition(bossPos);
        c.setFillColor(sf::Color(40, 24, 78));
        c.setOutlineColor(sf::Color(255, 200, 90));
        c.setOutlineThickness(3.f);
        w.draw(c);
        put(w, mono, "</>", bossPos.x, bossPos.y - 8.f, 16, sf::Color(255, 200, 90), 1);
        put(w, mono, "IZUNA", bossPos.x, bossPos.y - 58.f, 12, sf::Color::White, 1);
    }

    // my bullets
    for (const auto &b : mine)
    {
        sf::CircleShape c(4.f);
        c.setOrigin({4.f, 4.f});
        c.setPosition(b.pos);
        c.setFillColor(sf::Color(0, 229, 255));
        w.draw(c);
    }

    // their bullets
    for (const auto &b : theirs)
    {
        sf::CircleShape c(6.f);
        c.setOrigin({6.f, 6.f});
        c.setPosition(b.pos);
        c.setFillColor(sf::Color(255, 60, 140));
        c.setOutlineColor(sf::Color::White);
        c.setOutlineThickness(1.f);
        w.draw(c);
    }

    // player (blinks while invulnerable)
    if (invuln <= 0.f || std::fmod(t_, 0.2f) < 0.1f)
    {
        sf::ConvexShape s(3);
        s.setPoint(0, {playerPos.x, playerPos.y - 14.f});
        s.setPoint(1, {playerPos.x - 11.f, playerPos.y + 10.f});
        s.setPoint(2, {playerPos.x + 11.f, playerPos.y + 10.f});
        s.setFillColor(sf::Color(215, 245, 255));
        s.setOutlineColor(sf::Color(0, 229, 255));
        s.setOutlineThickness(2.f);
        w.draw(s);

        sf::CircleShape core(3.f);
        core.setOrigin({3.f, 3.f});
        core.setPosition(playerPos);
        core.setFillColor(sf::Color(255, 60, 140));
        w.draw(core);
    }

    if (hurt > 0.f)
        box(w, fx, fy, fw, fh, sf::Color(255, 30, 60, static_cast<std::uint8_t>(hurt * 60.f)));

    // ---------------- editor panel ----------------
    const sf::Color pink(255, 0, 127), cyan(0, 229, 255), dim(110, 145, 175);

    box(w, 730.f, 70.f, 532.f, 620.f, sf::Color(2, 6, 13, 248),
        focused ? pink : sf::Color(255, 0, 127, 110), 2.f);

    put(w, mono, "ROOT // IZUNA.HTML", 748.f, 84.f, 12, pink);
    put(w, mono, focused ? "EDITING - TIME SLOWED" : "CLICK CODE TO EDIT", 1246.f, 85.f, 10,
        focused ? sf::Color(240, 200, 110) : dim, 2);
    box(w, 742.f, 105.f, 504.f, 1.f, sf::Color(255, 0, 127, 100));

    box(w, 746.f, 118.f, 500.f, 180.f, sf::Color(1, 4, 10),
        focused ? sf::Color(0, 229, 255, 200) : sf::Color(20, 85, 110, 130), 1.f);

    for (int r = 0; r < static_cast<int>(lines.size()); r++)
    {
        float y = textTop + r * lineH;

        put(w, mono, to_string(r + 1), 780.f, y + 3.f, 10,
            (focused && r == row) ? cyan : sf::Color(45, 85, 110), 2);

        for (const auto &d : diags)
            if (d.line == r)
                box(w, 748.f, y + 3.f, 3.f, 14.f, d.error ? sf::Color(255, 90, 110) : sf::Color(240, 170, 70));

        drawLine(w, mono, lines[r], textLeft, y);
    }

    if (focused && std::fmod(blink, 1.f) < 0.6f)
        box(w, textLeft + col * cw, textTop + row * lineH + 1.f, 2.f, 16.f, cyan);

    // diagnostics strip
    const Diag *err = nullptr;
    const Diag *warn = nullptr;
    for (const auto &d : diags)
    {
        if (d.error && !err)
            err = &d;
        if (!d.error && !warn)
            warn = &d;
    }

    box(w, 746.f, 306.f, 500.f, 28.f, sf::Color(3, 8, 15),
        denyFlash > 0.f ? sf::Color(255, 90, 110, static_cast<std::uint8_t>(255 * denyFlash))
                        : sf::Color(0, 190, 220, 70),
        1.f);

    if (err)
        put(w, mono, "ERR   line " + to_string(err->line + 1) + ": " + err->msg, 758.f, 314.f, 10,
            sf::Color(255, 90, 110));
    else if (warn)
        put(w, mono, "WARN  line " + to_string(warn->line + 1) + ": " + warn->msg, 758.f, 314.f, 10,
            sf::Color(240, 170, 70));
    else
        put(w, mono, "OK    renders clean", 758.f, 314.f, 10, sf::Color(0, 235, 190));

    // build button
    bool hover = buildRect.contains(mouse);
    sf::Color fill = hasError ? sf::Color(24, 8, 14) : (hover ? sf::Color(44, 10, 32) : sf::Color(6, 10, 20));
    sf::Color border = hasError ? sf::Color(120, 40, 60) : (hover ? sf::Color(255, 90, 170) : pink);

    box(w, 746.f, 345.f, 500.f, 60.f, fill, border, 2.f);
    put(w, display, hasError ? "FIX ERRORS" : "BUILD // RENDER", 996.f, 355.f, 20,
        hasError ? sf::Color(170, 90, 100) : sf::Color(245, 245, 255), 1);
    put(w, mono, "CTRL+ENTER / F5", 996.f, 385.f, 9, dim, 1);

    if (buildGlow > 0.f)
        box(w, 746.f, 345.f, 500.f, 60.f, sf::Color(255, 255, 255, static_cast<std::uint8_t>(buildGlow * 120.f)));

    // applied values
    box(w, 746.f, 425.f, 500.f, 120.f, sf::Color(3, 8, 15, 230), sf::Color(0, 190, 220, 80), 1.f);
    put(w, mono, "LIVE BUILD", 760.f, 433.f, 10, cyan);

    char buf[96];
    snprintf(buf, sizeof buf, "ship speed  %.1f", applied.shipSpeed);
    put(w, mono, buf, 760.f, 456.f, 12, sf::Color(220, 225, 245));
    snprintf(buf, sizeof buf, "shot        %d bullets / %d deg / spd %.1f", applied.count,
             static_cast<int>(applied.spread), applied.shotSpeed);
    put(w, mono, buf, 760.f, 476.f, 12, sf::Color(220, 225, 245));
    snprintf(buf, sizeof buf, "rate        %d ms", static_cast<int>(applied.rateMs));
    put(w, mono, buf, 760.f, 496.f, 12, sf::Color(220, 225, 245));

    put(w, mono, "TAGS: <ship speed> <shot count spread speed> <rate ms>", 760.f, 522.f, 9, dim);

    put(w, mono, "WASD move   |   click code to edit   |   ESC release / quit", 748.f, 660.f, 9, dim);

    // ---------------- end overlay ----------------
    if (ended_)
    {
        box(w, 0.f, 0.f, 1280.f, 720.f, sf::Color(0, 0, 0, 170));

        bool won = outcome_ == 1;
        put(w, display, won ? "PAGE RENDERED" : "404 - PAGE NOT FOUND", 640.f, 290.f, 52,
            won ? sf::Color(240, 200, 110) : sf::Color(255, 90, 110), 1);

        if (std::fmod(t_, 1.f) < 0.7f)
            put(w, mono, "[E] Continue", 640.f, 380.f, 16, sf::Color(90, 235, 235), 1);
    }
}

// ---------------------------------------------------------------
// private helpers
// ---------------------------------------------------------------

string HtmlShooterScreen::trim(const string &s)
{
    size_t a = s.find_first_not_of(" \t");
    if (a == string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t");
    return s.substr(a, b - a + 1);
}

void HtmlShooterScreen::build()
{
    if (hasError)
    {
        denyFlash = 1.f;
        return;
    }

    applied = pending;
    focused = false;
    buildGlow = 1.f;
}

void HtmlShooterScreen::addDiag(int line, const string &msg, bool error)
{
    diags.push_back({line, msg, error});
    if (error)
        hasError = true;
}

// sets target from v, clamping into [lo, hi] with a warning
void HtmlShooterScreen::setClamped(float &target, float v, float lo, float hi,
                                   int line, const string &key)
{
    if (v < lo || v > hi)
    {
        addDiag(line,
                key + " limited to " + to_string(static_cast<int>(lo)) + "-" +
                    to_string(static_cast<int>(hi)),
                false);
        v = clamp(v, lo, hi);
    }
    target = v;
}

void HtmlShooterScreen::parse()
{
    diags.clear();
    hasError = false;

    Params p = pending;

    for (int i = 0; i < static_cast<int>(lines.size()); i++)
    {
        string s = trim(lines[i]);
        if (s.empty())
            continue;

        if (s.front() != '<' || s.back() != '>')
        {
            addDiag(i, "tag must look like <name attr=\"1\">", true);
            continue;
        }

        string inner = trim(s.substr(1, s.size() - 2));
        if (inner.empty())
        {
            addDiag(i, "empty tag", true);
            continue;
        }

        size_t sp = inner.find(' ');
        string name = inner.substr(0, sp);
        string rest = (sp == string::npos) ? "" : inner.substr(sp + 1);

        vector<pair<string, float>> attrs;
        size_t pos = 0;
        bool bad = false;

        while (pos < rest.size())
        {
            while (pos < rest.size() && rest[pos] == ' ')
                pos++;
            if (pos >= rest.size())
                break;

            size_t eq = rest.find('=', pos);
            if (eq == string::npos)
            {
                addDiag(i, "expected name=\"value\"", true);
                bad = true;
                break;
            }

            string key = trim(rest.substr(pos, eq - pos));

            if (eq + 1 >= rest.size() || rest[eq + 1] != '"')
            {
                addDiag(i, "value for '" + key + "' needs quotes", true);
                bad = true;
                break;
            }

            size_t q = rest.find('"', eq + 2);
            if (q == string::npos)
            {
                addDiag(i, "missing closing quote", true);
                bad = true;
                break;
            }

            string val = rest.substr(eq + 2, q - eq - 2);
            char *endp = nullptr;
            float v = strtof(val.c_str(), &endp);

            if (val.empty() || *endp != '\0' || !std::isfinite(v))
            {
                addDiag(i, "'" + val + "' is not a number", true);
                bad = true;
                break;
            }

            attrs.push_back({key, v});
            pos = q + 1;
        }

        if (bad)
            continue;

        if (name != "ship" && name != "shot" && name != "rate")
        {
            addDiag(i, "unknown tag <" + name + ">", true);
            continue;
        }

        for (const auto &a : attrs)
        {
            if (name == "ship" && a.first == "speed")
                setClamped(p.shipSpeed, a.second, 1.f, 12.f, i, "speed");
            else if (name == "shot" && a.first == "count")
            {
                float c = static_cast<float>(p.count);
                setClamped(c, std::round(a.second), 1.f, 9.f, i, "count");
                p.count = static_cast<int>(c);
            }
            else if (name == "shot" && a.first == "spread")
                setClamped(p.spread, a.second, 0.f, 180.f, i, "spread");
            else if (name == "shot" && a.first == "speed")
                setClamped(p.shotSpeed, a.second, 2.f, 20.f, i, "speed");
            else if (name == "rate" && a.first == "ms")
                setClamped(p.rateMs, a.second, 80.f, 1000.f, i, "ms");
            else
                addDiag(i, "<" + name + "> has no attribute '" + a.first + "'", true);
        }
    }

    pending = p;
}

void HtmlShooterScreen::box(sf::RenderWindow &w, float x, float y, float wd, float h,
                            sf::Color fill, sf::Color outline, float thick)
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

void HtmlShooterScreen::put(sf::RenderWindow &w, const sf::Font &f, const string &s,
                            float x, float y, unsigned size, sf::Color c, int align)
{
    sf::Text t(f, s, size);
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

// HTML syntax colouring: brackets grey, tag names cyan, attrs orange, values green
void HtmlShooterScreen::drawLine(sf::RenderWindow &w, const sf::Font &f, const string &s,
                                 float x, float y) const
{
    const sf::Color gray(120, 150, 175), cyan(0, 229, 255), orange(230, 164, 92),
        green(160, 200, 80), plain(215, 225, 235);

    int state = 0; // 0 outside, 1 tag name, 2 attributes, 3 inside quotes
    size_t start = 0;
    sf::Color runCol = plain;

    auto flush = [&](size_t end)
    {
        if (end > start)
            put(w, f, s.substr(start, end - start), x + start * cw, y + 1.f, 14, runCol);
        start = end;
    };

    for (size_t i = 0; i < s.size(); i++)
    {
        char c = s[i];
        sf::Color colr = plain;

        if (state == 3)
        {
            colr = green;
            if (c == '"')
                state = 2;
        }
        else if (c == '<')
        {
            colr = gray;
            state = 1;
        }
        else if (c == '>')
        {
            colr = gray;
            state = 0;
        }
        else if (c == '"')
        {
            colr = green;
            state = 3;
        }
        else if (state == 1)
        {
            if (c == ' ')
            {
                state = 2;
                colr = plain;
            }
            else
                colr = cyan;
        }
        else if (state == 2)
        {
            colr = (c == '=') ? gray : (c == ' ' ? plain : orange);
        }

        if (i == 0)
            runCol = colr;
        else if (colr != runCol)
        {
            flush(i);
            runCol = colr;
        }
    }

    flush(s.size());
}