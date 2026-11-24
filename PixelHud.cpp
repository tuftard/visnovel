#include "PixelHud.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>

using std::clamp;

// ---------------- pixel digits ----------------
static const char *GLYPH[10][5] = {
    {"111", "101", "101", "101", "111"}, {"010", "110", "010", "010", "111"},
    {"111", "001", "111", "100", "111"}, {"111", "001", "111", "001", "111"},
    {"101", "101", "111", "001", "001"}, {"111", "100", "111", "001", "111"},
    {"111", "100", "111", "101", "111"}, {"111", "001", "001", "001", "001"},
    {"111", "101", "111", "101", "111"}, {"111", "101", "111", "001", "111"}};

static sf::Color withA(sf::Color c, float a)
{
    c.a = static_cast<std::uint8_t>(clamp(a, 0.f, 255.f));
    return c;
}

static unsigned hash3(unsigned a, unsigned b, unsigned c)
{
    unsigned h = a * 374761393u + b * 668265263u + c * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

static void addPx(sf::VertexArray &va, float x, float y, float w, float h, sf::Color c)
{
    sf::Vector2f a{x, y}, b{x + w, y}, d{x, y + h}, e{x + w, y + h};
    va.append(sf::Vertex{a, c}); va.append(sf::Vertex{b, c}); va.append(sf::Vertex{e, c});
    va.append(sf::Vertex{a, c}); va.append(sf::Vertex{e, c}); va.append(sf::Vertex{d, c});
}

static void addGlyph(sf::VertexArray &va, int digit, float x, float y, sf::Color c)
{
    for (int r = 0; r < 5; r++)
        for (int k = 0; k < 3; k++)
            if (GLYPH[digit][r][k] == '1') addPx(va, x + k, y + r, 1.f, 1.f, c);
}

// =============================================================
// CODE RAIN
// =============================================================
CodeRain::CodeRain(float width, float height) : W(width), H(height)
{
    for (float x = 2.f; x < W - 4.f; x += 6.f)
    {
        if (std::rand() % 10 >= 7) continue; // leave gaps so it isn't a solid wall
        Column c{};
        c.x = x;
        respawn(c, true);
        cols.push_back(c);
    }
}

void CodeRain::respawn(Column &c, bool first)
{
    c.speed = 25.f + static_cast<float>(std::rand() % 45);
    c.len = 7 + std::rand() % 9;
    c.tint = std::rand() % 2;
    c.head = first ? static_cast<float>(std::rand() % static_cast<int>(H))
                   : -static_cast<float>(std::rand() % 90);
}

void CodeRain::update(float dt)
{
    time += dt;
    for (auto &c : cols)
    {
        c.head += c.speed * dt;
        if (c.head - c.len * ROW > H) respawn(c, false);
    }
}

void CodeRain::draw(sf::RenderTarget &target)
{
    sf::VertexArray va(sf::PrimitiveType::Triangles);
    const sf::Color purple{205, 165, 255}, pink{255, 155, 225}, white{255, 235, 255};

    for (std::size_t ci = 0; ci < cols.size(); ci++)
    {
        const Column &c = cols[ci];
        int headRow = static_cast<int>(std::floor(c.head / ROW));

        for (int i = 0; i < c.len; i++)
        {
            int row = headRow - i;
            float y = row * ROW;
            if (y < -5.f || y > H) continue;

            // digit stays glued to its row and only flips now and then
            unsigned epoch = static_cast<unsigned>(time * 0.8f + ci * 0.31f);
            int bit = hash3(static_cast<unsigned>(ci), static_cast<unsigned>(row), epoch) & 1;

            float f = 1.f - static_cast<float>(i) / c.len;
            sf::Color base = c.tint ? pink : purple;
            sf::Color col = (i == 0) ? white : base;
            float a = (i == 0 ? 190.f : 12.f + 100.f * f * f) * clamp(intensity, 0.f, 1.f);

            addGlyph(va, bit, c.x, y, withA(col, a));
        }
    }
    target.draw(va);
}

// =============================================================
// HEALTH BARS
// =============================================================
static void rectPx(sf::RenderTarget &t, float x, float y, float w, float h, sf::Color c)
{
    sf::RectangleShape r({w, h});
    r.setPosition({x, y});
    r.setFillColor(c);
    t.draw(r);
}

// 1px border drawn around (x,y,w,h), fill inside
static void boxPx(sf::RenderTarget &t, float x, float y, float w, float h, sf::Color fill, sf::Color border)
{
    rectPx(t, x - 1.f, y - 1.f, w + 2.f, h + 2.f, border);
    rectPx(t, x, y, w, h, fill);
    rectPx(t, x + 1.f, y + 1.f, w - 2.f, 1.f, withA(border, 90.f)); // inner top shine
}

static void number(sf::RenderTarget &t, int v, float x, float y, sf::Color c, bool rightAlign)
{
    std::string s = std::to_string(std::max(0, v));
    float w = static_cast<float>(s.size()) * 4.f - 1.f;
    float px = rightAlign ? x - w : x;
    sf::VertexArray va(sf::PrimitiveType::Triangles);
    for (char ch : s)
    {
        addGlyph(va, ch - '0', px, y, c);
        px += 4.f;
    }
    t.draw(va);
}

static void hpBar(sf::RenderTarget &t, float x, float y, float w, float h, float frac, float ghostFrac,
                  sf::Color fill, sf::Color hi, bool rightAnchored, sf::Color border)
{
    frac = clamp(frac, 0.f, 1.f);
    ghostFrac = clamp(ghostFrac, 0.f, 1.f);

    boxPx(t, x, y, w, h, sf::Color(14, 12, 34), border);

    float gw = std::floor(w * ghostFrac), fw = std::floor(w * frac);
    float gx = rightAnchored ? x + w - gw : x;
    float fx = rightAnchored ? x + w - fw : x;

    if (gw > fw) rectPx(t, gx, y + 1.f, gw, h - 2.f, sf::Color(235, 235, 255, 150)); // trail
    if (fw > 0.f)
    {
        rectPx(t, fx, y + 1.f, fw, h - 2.f, fill);
        rectPx(t, fx, y + 1.f, fw, 1.f, hi);                              // bright top edge
        rectPx(t, fx, y + 3.f, fw, 1.f, withA(sf::Color::Black, 40.f));   // scanline stripe
        rectPx(t, fx, y + h - 2.f, fw, 1.f, withA(sf::Color::Black, 70.f));
    }
}

void HealthBars::set(int p, int b)
{
    playerHP = clamp(p, 0, 100);
    bossHP = clamp(b, 0, 100);
}

void HealthBars::update(float dt)
{
    time += dt;
    // trail waits a moment, then drains down to the real value
    ghostP = std::max(static_cast<float>(playerHP), ghostP - 35.f * dt);
    ghostB = std::max(static_cast<float>(bossHP), ghostB - 35.f * dt);
}

void HealthBars::draw(sf::RenderTarget &t)
{
    const sf::Color steel{74, 90, 130}, steelLight{130, 150, 200}, rust{160, 90, 80};
    const sf::Color teal{90, 235, 235}, amber{200, 140, 80}, violet{190, 110, 235};

    // ---- player portrait: framed square with a teal orb ----
    boxPx(t, 5.f, 4.f, 20.f, 20.f, {14, 14, 34}, steelLight);
    {
        sf::CircleShape orb(5.f, 14);
        orb.setOrigin({5.f, 5.f});
        orb.setPosition({15.f, 14.f});
        orb.setFillColor({30, 120, 140});
        orb.setOutlineColor(teal);
        orb.setOutlineThickness(1.f);
        t.draw(orb);
        rectPx(t, 13.f, 11.f, 1.f, 1.f, sf::Color::White);
    }

    // ---- player bar (flashes red when low) ----
    sf::Color pFill{40, 200, 210}, pHi{140, 250, 250};
    if (playerHP <= 25)
    {
        float k = 0.5f + 0.5f * std::sin(time * 10.f);
        auto lerp = [k](sf::Color a, sf::Color b) {
            return sf::Color(static_cast<std::uint8_t>(a.r + (b.r - a.r) * k),
                             static_cast<std::uint8_t>(a.g + (b.g - a.g) * k),
                             static_cast<std::uint8_t>(a.b + (b.b - a.b) * k));
        };
        pFill = lerp(pFill, {235, 70, 90});
        pHi = lerp(pHi, {255, 150, 160});
    }
    hpBar(t, 28.f, 7.f, 64.f, 8.f, playerHP / 100.f, ghostP / 100.f, pFill, pHi, false, steel);
    number(t, playerHP, 31.f, 8.5f, {230, 255, 255, 170}, false);

    // ---- life pips under the portrait ----
    for (int i = 0; i < 3; i++)
    {
        bool lit = playerHP > i * 34;
        sf::Color c = (i == 2) ? amber : teal;
        boxPx(t, 6.f + i * 7.f, 27.f, 5.f, 5.f, lit ? withA(c, 230.f) : sf::Color(24, 24, 50), steel);
    }

    // ---- boss portrait: dark orb with pulsing violet ring ----
    float pulse = 0.5f + 0.5f * std::sin(time * 2.5f);
    boxPx(t, 209.f, 4.f, 19.f, 20.f, {14, 10, 28}, rust);
    {
        sf::CircleShape orb(5.f, 16);
        orb.setOrigin({5.f, 5.f});
        orb.setPosition({218.5f, 14.f});
        orb.setFillColor({8, 4, 18});
        orb.setOutlineColor(withA(violet, 160.f + 90.f * pulse));
        orb.setOutlineThickness(2.f);
        t.draw(orb);
    }

    // ---- boss bar: drains toward the right side ----
    hpBar(t, 141.f, 7.f, 64.f, 8.f, bossHP / 100.f, ghostB / 100.f, {170, 90, 215}, {230, 170, 255}, true, rust);
    number(t, bossHP, 202.f, 8.5f, {255, 235, 255, 170}, true);
}
