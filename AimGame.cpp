#include "AimGame.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include "RhythmMapEditor.h"
#include <algorithm>

namespace Theme
{
    inline const sf::Color bgDeep(5, 11, 20);
    inline const sf::Color bgPanel(9, 19, 34);
    inline const sf::Color neonPink(255, 42, 117);
    inline const sf::Color neonCyan(0, 240, 255);
    inline const sf::Color neonPurple(157, 0, 255);
    inline const sf::Color neonOrange(255, 119, 0);
    inline const sf::Color text(255, 255, 255);
    inline const sf::Color textDim(97, 123, 148);
}

AimGame::AimGame(const RhythmMap *map)
{
    for (int i = 0; i < 40; i++)
        notes.push_back({(i * 2) % 3, (i / 2 + i / 5) % 3, 1.f + i * 0.5f, false});

    endTime = notes.back().time + 1.5f;
    if (map && map->mode == RhythmMap::Mode::Grid && !map->notes.empty())
    {
        notes.clear();

        for (const MapNote &n : map->notes)
            notes.push_back({n.col, n.row, n.time});

        std::sort(notes.begin(), notes.end(),
                  [](const AimNote &a, const AimNote &b)
                  { return a.time < b.time; });

        endTime = notes.back().time + 1.5f;
    }
}

sf::Vector2f AimGame::cellPos(int col, int row) const
{
    return {(col - 1.f) * CELL, (row - 1.f) * CELL};
}

sf::Vector2f AimGame::toScreen(sf::Vector2f p, float z) const
{
    float s = PERSPECTIVE / std::max(z, 0.1f);
    return center + p * s;
}

void AimGame::tryHit()
{
    for (auto &n : notes)
    {
        if (n.done)
            continue;
        if (std::abs(n.time - songTime) > HIT_WINDOW)
            continue;

        sf::Vector2f d = cursor - cellPos(n.col, n.row);
        if (std::hypot(d.x, d.y) < CELL * 0.5f)
        {
            n.done = true;
            hits++;
            score += 100 + 10 * combo;
            combo++;
            return;
        }
    }
}

void AimGame::handleEvent(const sf::Event &e)
{
    if (const auto *k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape)
            quit = true;
        else if (k->code == sf::Keyboard::Key::Z ||
                 k->code == sf::Keyboard::Key::X ||
                 k->code == sf::Keyboard::Key::Space)
            tryHit();
        return;
    }

    if (e.is<sf::Event::MouseButtonPressed>())
        tryHit();
}

void AimGame::update(float dt)
{
    songTime += std::min(dt, 0.05f);
    timeTracker += dt;

    for (auto &n : notes)
        if (!n.done && songTime > n.time + HIT_WINDOW)
        {
            n.done = true;
            combo = 0;
            misses++;
        }
}

// Helper drawing utilities
static void drawRect(sf::RenderWindow &w, float x, float y, float width, float height, sf::Color fill, sf::Color outline = sf::Color::Transparent, float thick = 0.f)
{
    sf::RectangleShape r({width, height});
    r.setPosition({std::floor(x), std::floor(y)});
    r.setFillColor(fill);
    if (thick > 0.f)
    {
        r.setOutlineColor(outline);
        r.setOutlineThickness(thick);
    }
    w.draw(r);
}

static void drawQuad(sf::RenderWindow &w, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d, sf::Color ca, sf::Color cb, sf::Color cc, sf::Color cd)
{
    sf::Vertex v[6] = {
        sf::Vertex{a, ca}, sf::Vertex{b, cb}, sf::Vertex{c, cc},
        sf::Vertex{a, ca}, sf::Vertex{c, cc}, sf::Vertex{d, cd}};
    w.draw(v, 6, sf::PrimitiveType::Triangles);
}

static void drawCircle(sf::RenderWindow &w, sf::Vector2f cpos, float r, sf::Color fill, sf::Color outline = sf::Color::Transparent, float thick = 0.f, size_t points = 24, float rotDeg = 0.f)
{
    sf::CircleShape s(r, points);
    s.setOrigin({r, r});
    s.setPosition(cpos);
    s.setRotation(sf::degrees(rotDeg));
    s.setFillColor(fill);
    if (thick > 0.f)
    {
        s.setOutlineColor(outline);
        s.setOutlineThickness(thick);
    }
    w.draw(s);
}

static void drawSpiral(sf::RenderWindow &w, sf::Vector2f cpos, float rMin, float rMax, int arms, float twist, float spin, float squash, sf::Color color)
{
    sf::VertexArray pts(sf::PrimitiveType::Points);
    for (int a = 0; a < arms; a++)
    {
        for (float r = rMin; r < rMax; r += 0.6f)
        {
            float t = (r - rMin) / (rMax - rMin);
            float ang = a * 6.2831853f / static_cast<float>(arms) + r * twist + spin;
            float x = std::floor(cpos.x + std::cos(ang) * r);
            float y = std::floor(cpos.y + std::sin(ang) * r * squash);

            sf::Color c = color;
            c.a = static_cast<std::uint8_t>(std::clamp(30.f + 200.f * (1.f - t), 0.f, 255.f));
            pts.append(sf::Vertex{{x + 0.5f, y + 0.5f}, c});
        }
    }
    w.draw(pts);
}

void AimGame::draw(sf::RenderWindow &w, const sf::Font &font)
{
    using sf::Color;
    w.setView(w.getDefaultView());
    cursor = w.mapPixelToCoords(sf::Mouse::getPosition(w)) - center;

    const Color CYAN = Theme::neonCyan;
    const Color PINK = Theme::neonPink;
    const Color PURPLE = Theme::neonPurple;
    const Color ORANGE = Theme::neonOrange;
    const Color WHITE = Theme::text;
    const Color LANE[3] = {CYAN, PINK, ORANGE};

    auto withA = [](Color c, float a)
    {
        c.a = static_cast<std::uint8_t>(255.f * std::clamp(a, 0.f, 1.f));
        return c;
    };

    auto text = [&](const std::string &s, float x, float y, unsigned size, Color c, bool centered = false)
    {
        sf::Text t(font, s, size);
        t.setFillColor(c);
        if (centered)
        {
            sf::FloatRect b = t.getLocalBounds();
            t.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
        }
        t.setPosition({x, y});
        w.draw(t);
    };

    // ---------- Background & Portals ----------
    w.clear(Theme::bgDeep);

    drawQuad(w, {0.f, 0.f}, {980.f, 0.f}, {980.f, 720.f}, {0.f, 720.f},
             Color(10, 25, 48), Color(10, 25, 48),
             Theme::bgDeep, Theme::bgDeep);

    sf::Vector2f portal(center.x, center.y - 120.f);
    drawSpiral(w, portal, 10.f, 115.f, 5, 0.060f, -timeTracker * 0.45f, 0.8f, Color(130, 70, 190));
    drawSpiral(w, portal, 10.f, 80.f, 3, -0.090f, timeTracker * 0.30f, 0.8f, Color(70, 50, 140));

    float pulse = 0.5f + 0.5f * std::sin(timeTracker * 3.f);
    drawCircle(w, portal, 17.f + pulse * 2.f, withA(PURPLE, 0.1f));
    drawCircle(w, portal, 10.5f, Color(6, 4, 16), CYAN, 2.f, 32);

    // Side borders / panels
    drawRect(w, 0.f, 0.f, 14.f, 720.f, Color(5, 11, 20));
    drawRect(w, 966.f, 0.f, 14.f, 720.f, Color(5, 11, 20));

    // ---------- Header Bar ----------
    drawRect(w, 0.f, 0.f, 1280.f, 35.f, Color(3, 7, 13));
    drawRect(w, 0.f, 35.f, 1280.f, 1.f, withA(CYAN, 0.23f));
    text("SHIRO BATTLE.OS // VISUAL_LANGUAGE = CODE", 15.f, 8.f, 13, CYAN);
    text("SECTOR 07", 890.f, 8.f, 13, CYAN);

    // ---------- HUD Panels ----------
    int total = hits + misses;
    float acc = total > 0 ? static_cast<float>(hits) / total : 1.f;
    char rank = acc >= .95f ? 'S' : acc >= .85f ? 'A'
                                : acc >= .70f   ? 'B'
                                : acc >= .50f   ? 'C'
                                                : 'D';

    auto hudPanel = [&](float x, const std::string &label, const std::string &val, Color vc)
    {
        drawRect(w, x, 50.f, 170.f, 36.f, Color(5, 11, 20, 205), CYAN, 1.f);
        text(label, x + 10.f, 59.f, 14, WHITE);
        text(val, x + 92.f, 59.f, 14, vc);
    };
    hudPanel(20.f, "SCORE:", std::to_string(score), CYAN);
    hudPanel(205.f, "COMBO:", std::to_string(combo) + "x", PINK);
    hudPanel(390.f, "RANK:", std::string(1, rank), PURPLE);

    // ---------- 3x3 Grid Targets ----------
    const sf::Vector2f cs = center + cursor;
    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 3; col++)
        {
            sf::Vector2f p = toScreen(cellPos(col, row), Z_HIT);
            float s = CELL * 0.92f;
            bool hover = std::fabs(cs.x - p.x) < CELL / 2.f && std::fabs(cs.y - p.y) < CELL / 2.f;

            drawRect(w, p.x - s / 2.f, p.y - s / 2.f, s, s,
                     hover ? Color(0, 240, 255, 40) : Color(5, 11, 20, 110),
                     withA(CYAN, hover ? 0.9f : 0.28f), hover ? 2.f : 1.f);
        }
    }

    // ---------- Notes ----------
    for (size_t i = notes.size(); i-- > 0;)
    {
        const AimNote &n = notes[i];
        if (n.done)
            continue;

        float t = (n.time - songTime) / APPROACH;
        if (t > 1.f || t < -HIT_WINDOW / APPROACH)
            continue;

        float z = Z_HIT + t * (Z_FAR - Z_HIT);
        float scale = PERSPECTIVE / z;
        sf::Vector2f p = toScreen(cellPos(n.col, n.row), z);
        float s = CELL * 0.8f * scale;
        float a = std::clamp(1.f - t * 0.6f, 0.3f, 1.f);
        Color c = LANE[n.col % 3];

        drawRect(w, p.x - s * 0.7f, p.y - s * 0.7f, s * 1.4f, s * 1.4f, withA(c, 0.12f * a));
        drawRect(w, p.x - s / 2.f, p.y - s / 2.f, s, s, withA(c, a), withA(WHITE, a), 2.f);
        drawRect(w, p.x - s * 0.09f, p.y - s * 0.09f, s * 0.18f, s * 0.18f, withA(WHITE, a));
    }

    // ---------- Crosshair ----------
    drawRect(w, cs.x - 12.f, cs.y - 1.f, 24.f, 2.f, WHITE);
    drawRect(w, cs.x - 1.f, cs.y - 12.f, 2.f, 24.f, WHITE);
    drawCircle(w, cs, 9.f, Color::Transparent, CYAN, 2.f);

    // ---------- Terminal Panel (Right Side matching attack.cpp layout) ----------
    drawRect(w, 980.f, 36.f, 300.f, 684.f, Theme::bgPanel);
    drawRect(w, 980.f, 36.f, 2.f, 684.f, PINK);
    drawRect(w, 980.f, 36.f, 300.f, 34.f, Color(5, 11, 20));
    text("ROOT // $hiro", 1000.f, 45.f, 12, PINK);
    text("attack.cpp", 1205.f, 45.f, 12, PURPLE);

    // Render code editor contents matching your HTML mockup
    float codeY = 90.f;
    auto codeLine = [&](const std::string &code, Color col)
    {
        text(code, 1000.f, codeY, 12, col);
        codeY += 20.f;
    };
    codeLine("#include <shiro_core.h>", PINK);
    codeY += 10.f;
    codeLine("// ATTACK CONFIGURATION", Theme::textDim);
    codeLine("int noteSpeed = 4;", CYAN);
    codeLine("float spawnRate = 1.2f;", ORANGE);
    codeLine("bool neonGlow = true;", PINK);

    // ---------- Telemetry Box with Glow Lines ----------
    // 1. Base background
    drawRect(w, 980.f, 590.f, 300.f, 130.f, Color(4, 8, 16));

    // 2. Outer glow layers (stacking larger, highly transparent rectangles)
    drawRect(w, 980.f - 3.f, 590.f - 3.f, 300.f + 6.f, 130.f + 6.f, Color::Transparent, withA(CYAN, 0.04f), 3.f);
    drawRect(w, 980.f - 2.f, 590.f - 2.f, 300.f + 4.f, 130.f + 4.f, Color::Transparent, withA(CYAN, 0.08f), 2.f);
    drawRect(w, 980.f - 1.f, 590.f - 1.f, 300.f + 2.f, 130.f + 2.f, Color::Transparent, withA(CYAN, 0.15f), 1.f);

    // 3. Crisp inner border outline
    drawRect(w, 980.f, 590.f, 300.f, 130.f, Color::Transparent, CYAN, 1.f);

    // 4. Top divider line & Text
    drawRect(w, 980.f, 590.f, 300.f, 1.f, withA(CYAN, 0.16f));
    text("TELEMETRY", 1000.f, 602.f, 11, Theme::textDim);
    auto tel = [&](float y, const std::string &label, const std::string &val)
    {
        text(label, 1000.f, y, 11, Theme::textDim);
        text(val, 1200.f, y, 11, CYAN);
    };
    tel(630.f, "NOTE SPEED", "4.0");
    tel(654.f, "SPAWN INTENSITY", "1.2s");

    char buf[32];
    std::snprintf(buf, sizeof buf, "%.0f%%", acc * 100.f);
    tel(678.f, "ACCURACY", buf);
    
}