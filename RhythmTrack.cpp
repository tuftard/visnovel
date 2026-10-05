#include "RhythmTrack.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace std;
using std::to_string;

RhythmTrack::RhythmTrack() : canvas(sf::Vector2u{CW, CH}) {}

void RhythmTrack::update(float dt, int playerLane)
{
    time += dt;

    for (float &g : laneGlow)
        g = max(0.f, g - dt * 4.f);

    hurtFlash = max(0.f, hurtFlash - dt * 3.f);

    playerLaneF += (static_cast<float>(playerLane) - playerLaneF) *
                   min(1.f, dt * 18.f);

    for (auto &s : sparks)
    {
        s.pos += s.vel * dt;
        s.vel.y += 140.f * dt;
        s.life -= dt;
    }

    sparks.erase(
        remove_if(sparks.begin(), sparks.end(),
                  [](const Spark &s)
                  { return s.life <= 0.f; }),
        sparks.end());
}

void RhythmTrack::flash(int lane)
{
    lane = clamp(lane, 0, 3);
    laneGlow[lane] = 1.f;

    sf::Vector2f p = pS(laneCenter(static_cast<float>(lane)), 1.f);

    for (int i = 0; i < 14; i++)
    {
        Spark s;
        s.pos = p;
        s.vel = {(rand() % 100 - 50) * 1.2f,
                 -(20.f + static_cast<float>(rand() % 90))};
        s.life = 0.3f + static_cast<float>(rand() % 30) / 100.f;
        s.maxLife = s.life;
        s.color = (i % 3 == 0) ? sf::Color(255, 255, 255)
                               : laneBright[lane];
        sparks.push_back(s);
    }
}

sf::Vector2f RhythmTrack::playerScreenPos() const
{
    return origin + pS(laneCenter(playerLaneF), 1.17f) * PX;
}

void RhythmTrack::draw(sf::RenderWindow &window,
                       const vector<RhythmNote> &notes,
                       const BattleHud &hud,
                       const sf::Font &font)
{
    canvas.clear(Theme::bgDeep);

    drawBackdrop();
    drawPortal();
    drawTrack();
    drawNotes(notes);
    drawHitZone();
    drawCrackle();
    drawSparks();
    drawPlayer();
    drawHud(hud);

    if (hurtFlash > 0.f)
        rect(0.f, 0.f, fW, fH, col(sf::Color(255, 30, 60), hurtFlash * 60.f));

    canvas.display();

    sf::Sprite sprite(canvas.getTexture());
    sprite.setPosition(origin);
    sprite.setScale({PX, PX});
    window.draw(sprite);

    drawOverlayText(window, hud, font);
}

// ---------------- helpers ----------------
sf::Color RhythmTrack::col(sf::Color c, float a)
{
    c.a = static_cast<std::uint8_t>(clamp(a, 0.f, 255.f));
    return c;
}

sf::Color RhythmTrack::mix(sf::Color a, sf::Color b, float t)
{
    auto m = [&](std::uint8_t x, std::uint8_t y)
    {
        return static_cast<std::uint8_t>(x + (y - x) * t);
    };
    return sf::Color(m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a));
}

sf::Vector2f RhythmTrack::pS(float lanePos, float s)
{
    return {vanishX + lanePos * halfHit * s,
            vanishY + (hitY - vanishY) * s};
}

sf::Vector2f RhythmTrack::pZ(float lanePos, float z)
{
    return pS(lanePos, zHit / max(z, 20.f));
}

void RhythmTrack::rect(float x, float y, float w, float h, sf::Color fill,
                       sf::Color outline, float thick)
{
    sf::RectangleShape r({w, h});
    r.setPosition({std::floor(x), std::floor(y)});
    r.setFillColor(fill);

    if (thick > 0.f)
    {
        r.setOutlineColor(outline);
        r.setOutlineThickness(thick);
    }

    canvas.draw(r);
}

void RhythmTrack::quad(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d,
                       sf::Color ca, sf::Color cb, sf::Color cc, sf::Color cd)
{
    sf::Vertex v[6] = {
        sf::Vertex{a, ca}, sf::Vertex{b, cb}, sf::Vertex{c, cc},
        sf::Vertex{a, ca}, sf::Vertex{c, cc}, sf::Vertex{d, cd}};

    canvas.draw(v, 6, sf::PrimitiveType::Triangles);
}

void RhythmTrack::quad(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d,
                       sf::Color all)
{
    quad(a, b, c, d, all, all, all, all);
}

void RhythmTrack::tri(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Color color)
{
    sf::Vertex v[3] = {sf::Vertex{a, color}, sf::Vertex{b, color},
                       sf::Vertex{c, color}};
    canvas.draw(v, 3, sf::PrimitiveType::Triangles);
}

void RhythmTrack::line(sf::Vector2f a, sf::Vector2f b, sf::Color c)
{
    const sf::Vector2f h(0.5f, 0.5f);
    sf::Vertex v[2] = {sf::Vertex{a + h, c}, sf::Vertex{b + h, c}};
    canvas.draw(v, 2, sf::PrimitiveType::Lines);
}

void RhythmTrack::circle(sf::Vector2f c, float r, sf::Color fill,
                         sf::Color outline, float thick,
                         size_t points, float rotDeg)
{
    sf::CircleShape s(r, points);
    s.setOrigin({r, r});
    s.setPosition(c);
    s.setRotation(sf::degrees(rotDeg));
    s.setFillColor(fill);

    if (thick > 0.f)
    {
        s.setOutlineColor(outline);
        s.setOutlineThickness(thick);
    }

    canvas.draw(s);
}

void RhythmTrack::spiral(sf::Vector2f c, float rMin, float rMax, int arms, float twist,
                         float spin, float squash, sf::Color color)
{
    sf::VertexArray pts(sf::PrimitiveType::Points);

    for (int a = 0; a < arms; a++)
    {
        for (float r = rMin; r < rMax; r += 0.6f)
        {
            float t = (r - rMin) / (rMax - rMin);
            float ang = a * 6.2831853f / static_cast<float>(arms) +
                        r * twist + spin;

            float x = std::floor(c.x + std::cos(ang) * r);
            float y = std::floor(c.y + std::sin(ang) * r * squash);

            pts.append(sf::Vertex{{x + 0.5f, y + 0.5f},
                                  col(color, 30.f + 200.f * (1.f - t))});
        }
    }

    canvas.draw(pts);
}

void RhythmTrack::frame(float x, float y, float w, float h, sf::Color fill,
                        sf::Color border)
{
    rect(x, y, w, h, fill, border, 1.f);
    rect(x + 1.f, y + 1.f, w - 2.f, 1.f, col(border, 90.f));

    // knocked-out corners (pixel-art rounded look)
    rect(x - 1.f, y - 1.f, 1.f, 1.f, Theme::bgDeep);
    rect(x + w, y - 1.f, 1.f, 1.f, Theme::bgDeep);
    rect(x - 1.f, y + h, 1.f, 1.f, Theme::bgDeep);
    rect(x + w, y + h, 1.f, 1.f, Theme::bgDeep);
}

void RhythmTrack::bar(float x, float y, float w, float h, float frac, sf::Color fill,
                      sf::Color hi, bool rightAnchored, sf::Color border)
{
    frac = clamp(frac, 0.f, 1.f);

    rect(x, y, w, h, sf::Color(14, 12, 34), border, 1.f);

    float fw = std::floor((w - 2.f) * frac);
    float fx = rightAnchored ? x + w - 1.f - fw : x + 1.f;

    if (fw > 0.f)
    {
        rect(fx, y + 1.f, fw, h - 2.f, fill);
        rect(fx, y + 1.f, fw, 1.f, hi);
    }
}

void RhythmTrack::magicCircle(sf::Vector2f c, float r, sf::Color a, sf::Color b,
                              float dir)
{
    sf::CircleShape base(r, 28);
    base.setOrigin({r, r});
    base.setScale({1.1f, 0.28f});
    base.setPosition({c.x, c.y + r * 1.15f});
    base.setFillColor(col(a, 90.f));
    canvas.draw(base);

    circle(c, r + 4.f, col(a, 18.f));
    circle(c, r, sf::Color::Transparent, col(a, 200.f), 1.f, 8,
           time * 20.f * dir);
    circle(c, r * 0.72f, sf::Color::Transparent, col(b, 220.f), 1.f, 6,
           -time * 35.f * dir);
    circle(c, r * 0.4f, col(b, 60.f), col(a, 230.f), 1.f, 4,
           time * 50.f * dir);
    circle(c, 1.5f, sf::Color(255, 255, 255), sf::Color::Transparent, 0.f, 6);
}

// ---------------- scene ----------------
void RhythmTrack::drawBackdrop()
{
    quad({0.f, 0.f}, {fW, 0.f}, {fW, fH}, {0.f, fH},
         sf::Color(36, 22, 70), sf::Color(36, 22, 70),
         sf::Color(10, 12, 30), sf::Color(10, 12, 30));

    sf::Vector2f portal{vanishX, vanishY};

    spiral(portal, 10.f, 115.f, 5, 0.060f, -time * 0.45f, 0.8f,
           sf::Color(130, 70, 190));
    spiral(portal, 10.f, 80.f, 3, -0.090f, time * 0.30f, 0.8f,
           sf::Color(70, 50, 140));

    // colonnade behind the portal
    for (int i = 0; i < 5; i++)
    {
        float lx = 58.f + i * 9.f;
        float rx = fW - 58.f - 5.f - i * 9.f;

        for (float px : {lx, rx})
        {
            rect(px, 31.f, 5.f, 30.f, sf::Color(20, 24, 52));
            rect(px, 31.f, 1.f, 30.f, sf::Color(44, 54, 96));
            rect(px - 1.f, 29.f, 7.f, 3.f, sf::Color(30, 36, 74));
        }
    }

    rect(54.f, 27.f, fW - 108.f, 3.f, sf::Color(26, 30, 62));

    // side pillars
    rect(0.f, 0.f, 14.f, fH, sf::Color(14, 16, 38));
    rect(fW - 14.f, 0.f, 14.f, fH, sf::Color(14, 16, 38));
    rect(13.f, 0.f, 1.f, fH, sf::Color(40, 48, 90));
    rect(fW - 14.f, 0.f, 1.f, fH, sf::Color(40, 48, 90));

    for (float y = 20.f; y < fH; y += 38.f)
    {
        rect(0.f, y, 14.f, 2.f, sf::Color(30, 36, 72));
        rect(fW - 14.f, y, 14.f, 2.f, sf::Color(30, 36, 72));
    }

    // floating summoning circles
    magicCircle({47.f, 52.f}, 16.f, Theme::gold, Theme::teal, 1.f);
    magicCircle({182.f, 50.f}, 17.f, Theme::violet,
                sf::Color(235, 170, 255), -1.f);
}

void RhythmTrack::drawPortal()
{
    sf::Vector2f p{vanishX, vanishY};
    float pulse = 0.5f + 0.5f * std::sin(time * 3.f);

    circle(p, 17.f + pulse * 2.f, col(Theme::violet, 26.f),
           sf::Color::Transparent, 0.f, 32);
    circle(p, 13.f, col(Theme::violet, 50.f), sf::Color::Transparent, 0.f, 32);
    circle(p, 10.5f, sf::Color(6, 4, 16), sf::Color(200, 120, 235), 2.f, 32);
    circle(p, 8.f, sf::Color::Transparent, sf::Color(110, 60, 160, 200),
           1.f, 32);

    for (int k = 0; k < 6; k++)
    {
        float ang = time * 1.2f + k * 1.047f;
        rect(p.x + std::cos(ang) * 12.f, p.y + std::sin(ang) * 12.f, 1.f,
             1.f, sf::Color(255, 220, 255));
    }
}

void RhythmTrack::drawTrack()
{
    const float sNear = 1.5f;
    const float sFar = 0.07f;

    // lane fills
    for (int lane = 0; lane < 4; lane++)
    {
        float l0 = -1.f + 0.5f * lane;
        float l1 = l0 + 0.5f;

        sf::Color nearCol = mix(laneDark[lane], laneMid[lane], 0.55f);
        sf::Color farCol = col(laneDark[lane], 90.f);

        quad(pS(l0, sNear), pS(l1, sNear), pS(l1, sFar), pS(l0, sFar),
             nearCol, nearCol, farCol, farCol);

        // key-press light beam
        float g = laneGlow[lane];

        if (g > 0.f)
        {
            sf::Color a = col(laneBright[lane], 90.f * g);
            sf::Color b = col(laneBright[lane], 0.f);

            quad(pS(l0, 1.f), pS(l1, 1.f), pS(l1, 0.35f), pS(l0, 0.35f),
                 a, a, b, b);
        }
    }

    // scrolling beat lines
    for (int k = 0; k < 7; k++)
    {
        float z = 110.f + fmod(fmod(k * 100.f - time * 300.f, 700.f) + 700.f,
                               700.f);
        float s = zHit / z;

        line(pS(-1.f, s), pS(1.f, s),
             sf::Color(150, 200, 255,
                       static_cast<std::uint8_t>(clamp(20.f + s * 45.f, 0.f,
                                                       90.f))));
    }

    // lane dividers
    for (int i = 0; i <= 4; i++)
    {
        float l = -1.f + 0.5f * i;
        sf::Color c;

        if (i == 0)
            c = Theme::teal;
        else if (i == 4)
            c = Theme::gold;
        else if (i == 2)
            c = sf::Color(240, 190, 240);
        else
            c = sf::Color(120, 100, 170, 150);

        line(pS(l, sNear), pS(l, sFar), c);

        if (i == 0 || i == 4)
        {
            float off = (i == 0) ? -1.f : 1.f;

            line(pS(l, sNear) + sf::Vector2f(off, 0.f),
                 pS(l, sFar) + sf::Vector2f(off, 0.f), col(c, 90.f));
        }
    }
}

void RhythmTrack::drawNotes(const vector<RhythmNote> &notes)
{
    for (const auto &n : notes)
    {
        if (!n.active || n.z > zFar)
            continue;

        int l = clamp(n.lane, 0, 3);
        float s = zHit / max(n.z, 20.f);
        float fade = clamp((zFar - n.z) / 140.f, 0.f, 1.f);
        float a = 255.f * fade;

        sf::Vector2f c = pZ(laneCenter(static_cast<float>(l)), n.z);

        float laneW = halfHit * s * 0.5f;
        float w = max(3.f, std::floor(laneW * 0.56f));
        float h = std::floor(w * 1.35f);
        float x = std::floor(c.x - w / 2.f);
        float y = std::floor(c.y - h / 2.f);

        // motion trail toward the portal
        sf::Vector2f t0 = pZ(laneCenter(static_cast<float>(l)), n.z + 110.f);

        quad({c.x - w * 0.35f, y}, {c.x + w * 0.35f, y},
             {t0.x + 0.5f, t0.y}, {t0.x - 0.5f, t0.y},
             col(laneBright[l], 90.f * fade),
             col(laneBright[l], 90.f * fade),
             col(laneBright[l], 0.f), col(laneBright[l], 0.f));

        // tarot-style card
        sf::Color body = mix(laneDark[l], laneMid[l], 0.6f);

        rect(x, y, w, h, col(body, a), col(laneBright[l], a), 1.f);
        rect(x, y, w, 1.f, col(sf::Color(255, 255, 255), a * 0.5f));

        if (w >= 8.f)
        {
            float gw = std::floor(w * 0.5f);
            float gx = x + std::floor((w - gw) / 2.f);
            float gy = y + std::floor(h * 0.3f);

            rect(gx, gy, gw, gw, sf::Color::Transparent,
                 col(laneBright[l], a), 1.f);
            rect(gx + std::floor(gw / 2.f), gy + std::floor(gw / 2.f), 1.f,
                 1.f, col(sf::Color(255, 255, 255), a));
        }
        else
        {
            rect(x + std::floor(w / 2.f), y + std::floor(h / 2.f), 1.f, 1.f,
                 col(sf::Color(255, 255, 255), a));
        }
    }
}

void RhythmTrack::drawHitZone()
{
    const float s0 = 0.93f;
    const float s1 = 1.07f;

    sf::Color base(12, 10, 30, 240);

    quad(pS(-1.04f, s1), pS(1.04f, s1), pS(1.04f, s0), pS(-1.04f, s0), base);

    for (int lane = 0; lane < 4; lane++)
    {
        float l0 = -1.f + 0.5f * lane + 0.03f;
        float l1 = l0 + 0.44f;
        float g = laneGlow[lane];

        sf::Color pad =
            mix(laneDark[lane], laneBright[lane], 0.15f + 0.85f * g);

        quad(pS(l0, s1 - 0.01f), pS(l1, s1 - 0.01f), pS(l1, s0 + 0.01f),
             pS(l0, s0 + 0.01f), pad);

        line(pS(l0, s1), pS(l1, s1), laneMid[lane]);
        line(pS(l0, s0), pS(l1, s0), laneMid[lane]);
        line(pS(l0, s1), pS(l0, s0), laneMid[lane]);
        line(pS(l1, s1), pS(l1, s0), laneMid[lane]);

        sf::Vector2f c = pS(laneCenter(static_cast<float>(lane)), 1.f);

        circle(c, 2.5f + g * 2.f, col(laneBright[lane], 140.f + 115.f * g),
               sf::Color::Transparent, 0.f, 4);

        if (g > 0.f)
        {
            circle(c, 4.f + (1.f - g) * 12.f, sf::Color::Transparent,
                   col(laneBright[lane], g * 220.f), 1.f, 16);
        }
    }

    // hit line
    line(pS(-1.04f, 1.f), pS(1.04f, 1.f), sf::Color(245, 235, 210, 230));

    // winged ends
    sf::Vector2f L = pS(-1.04f, 1.f);
    sf::Vector2f R = pS(1.04f, 1.f);

    tri(L + sf::Vector2f(0.f, -4.f), L + sf::Vector2f(-14.f, 0.f),
        L + sf::Vector2f(0.f, 4.f), laneMid[0]);
    tri(L + sf::Vector2f(-2.f, -2.f), L + sf::Vector2f(-9.f, 0.f),
        L + sf::Vector2f(-2.f, 2.f), laneBright[0]);

    tri(R + sf::Vector2f(0.f, -4.f), R + sf::Vector2f(14.f, 0.f),
        R + sf::Vector2f(0.f, 4.f), laneMid[3]);
    tri(R + sf::Vector2f(2.f, -2.f), R + sf::Vector2f(9.f, 0.f),
        R + sf::Vector2f(2.f, 2.f), laneBright[3]);

    // gold studs between lanes
    for (int i = 1; i <= 3; i++)
    {
        circle(pS(-1.f + 0.5f * i, 1.f), 2.f, Theme::gold,
               sf::Color::Transparent, 0.f, 4);
    }
}

void RhythmTrack::drawCrackle()
{
    for (int side = 0; side < 2; side++)
    {
        float lp = side == 0 ? -1.f : 1.f;
        sf::Color c = side == 0 ? Theme::teal : Theme::gold;

        for (int k = 0; k < 7; k++)
        {
            float s = 0.55f + 0.12f * k;
            float flick = std::sin(time * 9.f + k * 2.1f + side * 4.f);

            if (flick < 0.2f)
                continue;

            sf::Vector2f p = pS(lp, s);
            float off = (side == 0 ? -1.f : 1.f) * (2.f + 3.f * flick);

            rect(p.x + off, p.y, 1.f, 2.f, col(c, 200.f * flick));
            rect(p.x + off * 0.5f, p.y + 2.f, 1.f, 1.f, col(c, 160.f));
        }
    }
}

void RhythmTrack::drawSparks()
{
    for (const auto &s : sparks)
    {
        float k = s.life / s.maxLife;
        float size = k > 0.5f ? 2.f : 1.f;

        rect(s.pos.x, s.pos.y, size, size, col(s.color, 255.f * k));
    }
}

void RhythmTrack::drawPlayer()
{
    sf::Vector2f p = pS(laneCenter(playerLaneF), 1.17f);
    int lane = clamp(static_cast<int>(std::lround(playerLaneF)), 0, 3);

    spiral(p, 2.f, 13.f, 3, 0.35f, time * 4.f, 0.45f, laneBright[lane]);

    circle(p, 8.f, col(laneBright[lane], 35.f));
    circle(p, 4.5f, laneMid[lane], laneBright[lane], 1.f, 16);
    circle(p, 2.5f, sf::Color(230, 250, 255), sf::Color::Transparent, 0.f, 12);
}

// ---------------- HUD ----------------
void RhythmTrack::drawHud(const BattleHud &h)
{
    // player portrait + HP
    frame(5.f, 4.f, 20.f, 20.f, sf::Color(14, 14, 34), Theme::steelLight);
    circle({15.f, 14.f}, 5.f, sf::Color(30, 120, 140), Theme::teal, 1.f, 14);
    rect(13.f, 11.f, 1.f, 1.f, sf::Color(255, 255, 255));

    bar(26.f, 8.f, 66.f, 6.f, h.playerHP / 100.f, sf::Color(40, 200, 210),
        sf::Color(140, 250, 250), false, Theme::steel);

    // life pips under portrait
    for (int i = 0; i < 3; i++)
    {
        bool lit = h.playerHP > i * 34;
        sf::Color c = (i == 2) ? Theme::amber : Theme::teal;

        rect(6.f + i * 7.f, 27.f, 5.f, 5.f,
             lit ? col(c, 230.f) : sf::Color(24, 24, 50), Theme::steel, 1.f);
    }

    // boss portrait + HP
    float pulse = 0.5f + 0.5f * std::sin(time * 2.5f);

    frame(209.f, 4.f, 19.f, 20.f, sf::Color(14, 10, 28), Theme::rust);
    circle({218.5f, 14.f}, 5.f, sf::Color(8, 4, 18),
           col(Theme::violet, 160.f + 90.f * pulse), 2.f, 16);

    bar(144.f, 8.f, 65.f, 6.f, h.bossHP / 100.f, sf::Color(170, 90, 215),
        sf::Color(230, 170, 255), true, Theme::rust);

    // attack preview panel
    frame(7.f, 70.f, 40.f, 42.f, sf::Color(36, 22, 44), Theme::amber);

    {
        sf::Vector2f o{12.f, 91.f};
        int n = clamp(h.bullets, 1, 50);

        float startAngle = -h.spread / 2.f;
        float step = (n > 1) ? h.spread / static_cast<float>(n - 1) : 0.f;

        if (h.spread >= 360.f)
        {
            step = 360.f / static_cast<float>(n);
            startAngle = 0.f;
        }

        rect(o.x - 1.f, o.y - 1.f, 3.f, 3.f, sf::Color(255, 255, 255));

        for (int i = 0; i < n; i++)
        {
            float rad = (startAngle + step * i) * 3.14159265f / 180.f;

            for (float d : {9.f, 16.f, 23.f})
            {
                float px = o.x + std::cos(rad) * d;
                float py = o.y + std::sin(rad) * d;

                if (px < 9.f || px > 44.f || py < 72.f || py > 109.f)
                    continue;

                rect(px, py, 2.f, 2.f,
                     col(Theme::gold, d < 20.f ? 255.f : 150.f));
            }
        }
    }

    // lane key slots
    for (int i = 0; i < 4; i++)
    {
        float g = laneGlow[i];

        sf::Color fill = mix(laneDark[i], laneBright[i], 0.1f + 0.7f * g);

        rect(8.f + i * 9.f, 119.f, 8.f, 8.f, fill, laneMid[i], 1.f);
    }

    // combo meter
    bar(62.f, 186.f, 112.f, 9.f, min(h.combo, 50) / 50.f,
        sf::Color(210, 160, 70), sf::Color(255, 225, 140), false,
        Theme::steel);
}

void RhythmTrack::text(sf::RenderWindow &w, const sf::Font &f, const string &s,
                       sf::Vector2f cpos, unsigned size, sf::Color c, int align)
{
    sf::Text t(f, s, size);
    t.setFillColor(c);

    sf::FloatRect b = t.getLocalBounds();
    float ox = b.position.x;

    if (align == 1)
        ox += b.size.x / 2.f;
    else if (align == 2)
        ox += b.size.x;

    t.setOrigin({ox, b.position.y + b.size.y / 2.f});
    t.setPosition(origin + cpos * PX);

    w.draw(t);
}

void RhythmTrack::drawOverlayText(sf::RenderWindow &w, const BattleHud &h,
                                  const sf::Font &f)
{
    text(w, f, "P1  " + to_string(h.playerHP), {28.f, 11.f}, 11, Theme::text);
    text(w, f, "SHIRO", {147.f, 11.f}, 11, Theme::text);
    text(w, f, string("RANK ") + scoreLetter(h.score), {208.f, 18.5f}, 11,
         letterColor(scoreLetter(h.score)), 2);
    text(w, f, "ATK PREVIEW", {27.f, 66.f}, 9, Theme::gold, 1);
    text(w, f, "COMBO x" + to_string(h.combo), {118.f, 190.5f}, 11,
         sf::Color(255, 255, 255), 1);

    const char *keys[4] = {"A", "S", "D", "F"};

    for (int i = 0; i < 4; i++)
        text(w, f, keys[i], {12.f + i * 9.f, 123.f}, 12, laneBright[i], 1);
}