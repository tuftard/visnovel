#include "AimGame.h"
#include "RhythmMapEditor.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

// ======================================================================
// Palette + helpers (anonymous namespace: no clash with Theme in RhythmTrack.h)
// ======================================================================
namespace
{
    const sf::Color BG_DEEP(10, 10, 26), GOLD(240, 200, 110), VIOLET(190, 110, 235),
        TEAL(90, 235, 235), RUST(160, 90, 80), AMBER(200, 140, 80),
        STEEL(74, 90, 130), STEEL_LIGHT(130, 150, 200), TEXT(220, 225, 245);

    // one colour set per grid column (teal / violet / gold, same as rhythm lanes)
    const sf::Color BRIGHT[3] = {sf::Color(100, 240, 240), sf::Color(195, 115, 240), sf::Color(250, 210, 115)};
    const sf::Color MID[3] = {sf::Color(30, 140, 150), sf::Color(110, 60, 170), sf::Color(200, 150, 70)};
    const sf::Color DARK[3] = {sf::Color(8, 46, 56), sf::Color(40, 24, 78), sf::Color(96, 64, 36)};

    sf::Color col(sf::Color c, float a)
    {
        c.a = static_cast<std::uint8_t>(std::clamp(a, 0.f, 255.f));
        return c;
    }

    sf::Color mix(sf::Color a, sf::Color b, float t)
    {
        auto m = [&](std::uint8_t x, std::uint8_t y)
        { return static_cast<std::uint8_t>(x + (y - x) * t); };
        return sf::Color(m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a));
    }

    char rankOf(float acc)
    {
        return acc >= .95f ? 'S' : acc >= .85f ? 'A'
                               : acc >= .70f   ? 'B'
                               : acc >= .50f   ? 'C'
                                               : 'D';
    }

    sf::Color rankColor(char r)
    {
        switch (r)
        {
        case 'S':
            return GOLD;
        case 'A':
            return TEAL;
        case 'B':
            return VIOLET;
        case 'C':
            return AMBER;
        default:
            return RUST;
        }
    }

    void rect(sf::RenderTarget &t, float x, float y, float w, float h, sf::Color fill,
              sf::Color outline = sf::Color::Transparent, float thick = 0.f)
    {
        sf::RectangleShape r({w, h});
        r.setPosition({std::floor(x), std::floor(y)});
        r.setFillColor(fill);
        if (thick > 0.f)
        {
            r.setOutlineColor(outline);
            r.setOutlineThickness(thick);
        }
        t.draw(r);
    }

    void quad(sf::RenderTarget &t, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d,
              sf::Color ca, sf::Color cb, sf::Color cc, sf::Color cd)
    {
        sf::Vertex v[6] = {sf::Vertex{a, ca}, sf::Vertex{b, cb}, sf::Vertex{c, cc},
                           sf::Vertex{a, ca}, sf::Vertex{c, cc}, sf::Vertex{d, cd}};
        t.draw(v, 6, sf::PrimitiveType::Triangles);
    }

    void quad(sf::RenderTarget &t, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d,
              sf::Color all)
    {
        quad(t, a, b, c, d, all, all, all, all);
    }

    void line(sf::RenderTarget &t, sf::Vector2f a, sf::Vector2f b, sf::Color c)
    {
        const sf::Vector2f h(0.5f, 0.5f);
        sf::Vertex v[2] = {sf::Vertex{a + h, c}, sf::Vertex{b + h, c}};
        t.draw(v, 2, sf::PrimitiveType::Lines);
    }

    void circle(sf::RenderTarget &t, sf::Vector2f c, float r, sf::Color fill,
                sf::Color outline = sf::Color::Transparent, float thick = 0.f,
                size_t points = 24, float rotDeg = 0.f)
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
        t.draw(s);
    }

    void spiral(sf::RenderTarget &t, sf::Vector2f c, float rMin, float rMax, int arms,
                float twist, float spin, float squash, sf::Color color)
    {
        sf::VertexArray pts(sf::PrimitiveType::Points);
        for (int a = 0; a < arms; a++)
            for (float r = rMin; r < rMax; r += 0.6f)
            {
                float k = (r - rMin) / (rMax - rMin);
                float ang = a * 6.2831853f / static_cast<float>(arms) + r * twist + spin;
                float x = std::floor(c.x + std::cos(ang) * r);
                float y = std::floor(c.y + std::sin(ang) * r * squash);
                pts.append(sf::Vertex{{x + 0.5f, y + 0.5f}, col(color, 30.f + 200.f * (1.f - k))});
            }
        t.draw(pts);
    }

    void frame(sf::RenderTarget &t, float x, float y, float w, float h, sf::Color fill, sf::Color border)
    {
        rect(t, x, y, w, h, fill, border, 1.f);
        rect(t, x + 1.f, y + 1.f, w - 2.f, 1.f, col(border, 90.f));
        rect(t, x - 1.f, y - 1.f, 1.f, 1.f, BG_DEEP);
        rect(t, x + w, y - 1.f, 1.f, 1.f, BG_DEEP);
        rect(t, x - 1.f, y + h, 1.f, 1.f, BG_DEEP);
        rect(t, x + w, y + h, 1.f, 1.f, BG_DEEP);
    }

    void bar(sf::RenderTarget &t, float x, float y, float w, float h, float frac,
             sf::Color fill, sf::Color hi, bool rightAnchored, sf::Color border)
    {
        frac = std::clamp(frac, 0.f, 1.f);
        rect(t, x, y, w, h, sf::Color(14, 12, 34), border, 1.f);
        float fw = std::floor((w - 2.f) * frac);
        float fx = rightAnchored ? x + w - 1.f - fw : x + 1.f;
        if (fw > 0.f)
        {
            rect(t, fx, y + 1.f, fw, h - 2.f, fill);
            rect(t, fx, y + 1.f, fw, 1.f, hi);
        }
    }

    void magicCircle(sf::RenderTarget &t, sf::Vector2f c, float r, sf::Color a, sf::Color b,
                     float dir, float time)
    {
        sf::CircleShape base(r, 28);
        base.setOrigin({r, r});
        base.setScale({1.1f, 0.28f});
        base.setPosition({c.x, c.y + r * 1.15f});
        base.setFillColor(col(a, 90.f));
        t.draw(base);

        circle(t, c, r + 4.f, col(a, 18.f));
        circle(t, c, r, sf::Color::Blue, col(a, 200.f), 1.f, 8, time * 20.f * dir);
        circle(t, c, r * 0.72f, sf::Color::Transparent, col(b, 220.f), 1.f, 6, -time * 35.f * dir);
        circle(t, c, r * 0.4f, col(b, 60.f), col(a, 230.f), 1.f, 4, time * 50.f * dir);
        circle(t, c, 1.5f, sf::Color(255, 255, 255), sf::Color::Transparent, 0.f, 6);
    }

    // text drawn on the real window so it stays sharp; position is in canvas units
    void put(sf::RenderWindow &w, const sf::Font &f, const std::string &s, sf::Vector2f cpos,
             unsigned size, sf::Color c, int align, float px)
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
        t.setPosition(cpos * px);
        w.draw(t);
    }
}

// ======================================================================
// Game logic
// ======================================================================
AimGame::AimGame(const RhythmMap *map) : canvas(sf::Vector2u{CW, CH})
{
    for (int i = 0; i < 40; i++)
        notes.push_back({(i * 2) % 3, (i / 2 + i / 5) % 3, 1.f + i * 0.5f, false});

    if (map && map->mode == RhythmMap::Mode::Grid && !map->notes.empty())
    {
        notes.clear();

        for (const MapNote &n : map->notes)
            notes.push_back({std::clamp(n.col, 0, 2), std::clamp(n.row, 0, 2), n.time});

        std::sort(notes.begin(), notes.end(),
                  [](const AimNote &a, const AimNote &b)
                  { return a.time < b.time; });
    }

    endTime = notes.back().time + 1.5f;
}

sf::Vector2f AimGame::cellPos(int c, int r) const
{
    return {GX + (c - 1.f) * CELL, GY + (r - 1.f) * CELL};
}

// s = 1 -> at the grid, small s -> near the portal
sf::Vector2f AimGame::project(sf::Vector2f target, float s) const
{
    sf::Vector2f v{VX, VY};
    return v + (target - v) * s;
}

float AimGame::accuracy() const
{
    int total = hits + misses;
    return total > 0 ? static_cast<float>(hits) / static_cast<float>(total) : 1.f;
}

void AimGame::burst(sf::Vector2f p, sf::Color c, int n)
{
    for (int i = 0; i < n; i++)
    {
        float a = static_cast<float>(rand() % 628) / 100.f;
        float sp = 25.f + static_cast<float>(rand() % 50);

        Spark s;
        s.pos = p;
        s.vel = {std::cos(a) * sp, std::sin(a) * sp - 15.f};
        s.life = 0.3f + static_cast<float>(rand() % 30) / 100.f;
        s.maxLife = s.life;
        s.color = (i % 3 == 0) ? sf::Color(255, 255, 255) : c;
        sparks.push_back(s);
    }
}

void AimGame::tryHit()
{
    AimNote *best = nullptr;
    float bestDt = 1e9f;

    for (auto &n : notes)
    {
        if (n.done)
            continue;

        float dt = std::abs(n.time - songTime);
        if (dt > HIT_WINDOW)
            continue;

        sf::Vector2f d = cursor - cellPos(n.col, n.row);
        if (std::hypot(d.x, d.y) > CELL * 0.55f)
            continue;

        if (dt < bestDt)
        {
            best = &n;
            bestDt = dt;
        }
    }

    if (!best)
        return;

    best->done = true;
    hits++;

    bool perfect = bestDt < 0.05f;
    score += 100 + 10 * combo + (perfect ? 50 : 0);
    combo++;

    sf::Vector2f p = cellPos(best->col, best->row);
    cellGlow[best->row * 3 + best->col] = 1.f;
    burst(p, BRIGHT[best->col], 16);

    fbText = perfect ? "WOW" : "Ok";
    fbColor = perfect ? GOLD : TEAL;
    fbTime = 0.6f;
    fbPos = p + sf::Vector2f(0.f, -CELL * 0.8f);
}

void AimGame::handleEvent(const sf::Event &e)
{
    if (const auto *m = e.getIf<sf::Event::MouseMoved>())
    {
        cursor = sf::Vector2f(m->position) / PX;
        return;
    }

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

    if (const auto *b = e.getIf<sf::Event::MouseButtonPressed>())
    {
        cursor = sf::Vector2f(b->position) / PX;
        tryHit();
    }
}

void AimGame::update(float dt)
{
    songTime += std::min(dt, 0.05f);
    timeTracker += dt;

    for (float &g : cellGlow)
        g = std::max(0.f, g - dt * 4.f);

    hurtFlash = std::max(0.f, hurtFlash - dt * 3.f);
    fbTime = std::max(0.f, fbTime - dt);

    for (auto &n : notes)
        if (!n.done && songTime > n.time + HIT_WINDOW)
        {
            n.done = true;
            combo = 0;
            misses++;
            hurtFlash = 1.f;

            fbText = "MISS";
            fbColor = sf::Color(255, 90, 110);
            fbTime = 0.6f;
            fbPos = cellPos(n.col, n.row) + sf::Vector2f(0.f, -CELL * 0.8f);
        }

    for (auto &s : sparks)
    {
        s.pos += s.vel * dt;
        s.vel.y += 120.f * dt;
        s.life -= dt;
    }

    sparks.erase(std::remove_if(sparks.begin(), sparks.end(),
                                [](const Spark &s)
                                { return s.life <= 0.f; }),
                 sparks.end());
}

// ======================================================================
// Drawing
// ======================================================================
void AimGame::draw(sf::RenderWindow &w, const sf::Font &font)
{
    w.setView(w.getDefaultView());

    canvas.clear(BG_DEEP);

    drawBackdrop();
    drawTunnel();
    drawGrid();
    drawNotes();
    drawSparks();
    drawCrosshair();
    drawHud();

    if (hurtFlash > 0.f)
        rect(canvas, 0.f, 0.f, FW, FH, col(sf::Color(255, 30, 60), hurtFlash * 50.f));

    canvas.display();

    sf::Sprite sprite(canvas.getTexture());
    sprite.setScale({PX, PX});
    w.draw(sprite);

    drawOverlayText(w, font);
}

void AimGame::drawBackdrop()
{
    quad(canvas, {0.f, 0.f}, {FW, 0.f}, {FW, FH}, {0.f, FH},
         sf::Color(36, 22, 70), sf::Color(36, 22, 70),
         sf::Color(10, 12, 30), sf::Color(10, 12, 30));

    sf::Vector2f portal{VX, VY};

    spiral(canvas, portal, 10.f, 105.f, 5, 0.060f, -timeTracker * 0.45f, 0.8f, sf::Color(130, 70, 190));
    spiral(canvas, portal, 10.f, 80.f, 3, -0.090f, timeTracker * 0.30f, 0.8f, sf::Color(70, 50, 140));

    // colonnade behind the portal
    for (int i = 0; i < 5; i++)
    {
        float lx = 88.f + i * 9.f;
        float rx = FW - 88.f - 5.f - i * 9.f;

        for (float px : {lx, rx})
        {
            rect(canvas, px, 20.f, 5.f, 28.f, sf::Color(20, 24, 52));
            rect(canvas, px, 20.f, 1.f, 28.f, sf::Color(44, 54, 96));
            rect(canvas, px - 1.f, 18.f, 7.f, 3.f, sf::Color(30, 36, 74));
        }
    }
    rect(canvas, 84.f, 15.f, FW - 168.f, 3.f, sf::Color(26, 30, 62));

    // side pillars
    rect(canvas, 0.f, 0.f, 14.f, FH, sf::Color(14, 16, 38));
    rect(canvas, FW - 14.f, 0.f, 14.f, FH, sf::Color(14, 16, 38));
    rect(canvas, 13.f, 0.f, 1.f, FH, sf::Color(40, 48, 90));
    rect(canvas, FW - 14.f, 0.f, 1.f, FH, sf::Color(40, 48, 90));

    for (float y = 20.f; y < FH; y += 30.f)
    {
        rect(canvas, 0.f, y, 14.f, 2.f, sf::Color(30, 36, 72));
        rect(canvas, FW - 14.f, y, 14.f, 2.f, sf::Color(30, 36, 72));
    }

    // floating summoning circles
    magicCircle(canvas, {40.f, 72.f}, 14.f, GOLD, TEAL, 1.f, timeTracker);
    magicCircle(canvas, {FW - 40.f, 72.f}, 14.f, VIOLET, sf::Color(235, 170, 255), -1.f, timeTracker);

    // portal
    float pulse = 0.5f + 0.5f * std::sin(timeTracker * 3.f);
    circle(canvas, portal, 17.f + pulse * 2.f, col(VIOLET, 26.f), sf::Color::Transparent, 0.f, 32);
    circle(canvas, portal, 13.f, col(VIOLET, 50.f), sf::Color::Transparent, 0.f, 32);
    circle(canvas, portal, 10.5f, sf::Color(6, 4, 16), sf::Color(200, 120, 235), 2.f, 32);
    circle(canvas, portal, 8.f, sf::Color::Transparent, sf::Color(110, 60, 160, 200), 1.f, 32);

    for (int k = 0; k < 6; k++)
    {
        float ang = timeTracker * 1.2f + k * 1.047f;
        rect(canvas, portal.x + std::cos(ang) * 12.f, portal.y + std::sin(ang) * 12.f,
             1.f, 1.f, sf::Color(255, 220, 255));
    }
}

// guide lines + scrolling frames that rush from the portal toward the grid
void AimGame::drawTunnel()
{
    const float half = CELL * 1.5f + 3.f;
    const sf::Vector2f corners[4] = {{GX - half, GY - half}, {GX + half, GY - half}, {GX + half, GY + half}, {GX - half, GY + half}};

    for (const auto &c : corners)
        line(canvas, project(c, 0.12f), project(c, 1.f), col(VIOLET, 70.f));

    for (int k = 0; k < 5; k++)
    {
        float z = Z_HIT + std::fmod(std::fmod(k * 200.f - timeTracker * 300.f, 1000.f) + 1000.f, 1000.f);
        float s = Z_HIT / z;
        sf::Color c(150, 200, 255, static_cast<std::uint8_t>(std::clamp(15.f + s * 60.f, 0.f, 90.f)));

        for (int i = 0; i < 4; i++)
            line(canvas, project(corners[i], s), project(corners[(i + 1) % 4], s), c);
    }
}

void AimGame::drawGrid()
{
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++)
        {
            sf::Vector2f p = cellPos(c, r);
            float g = cellGlow[r * 3 + c];
            float hs = CELL * 0.46f;

            bool hover = std::abs(cursor.x - p.x) < CELL / 2.f && std::abs(cursor.y - p.y) < CELL / 2.f;

            sf::Color pad = mix(DARK[c], BRIGHT[c], 0.12f + 0.85f * g + (hover ? 0.12f : 0.f));
            pad.a = 220;

            quad(canvas, {p.x - hs, p.y - hs}, {p.x + hs, p.y - hs},
                 {p.x + hs, p.y + hs}, {p.x - hs, p.y + hs}, pad);

            sf::Color edge = hover ? BRIGHT[c] : MID[c];
            line(canvas, {p.x - hs, p.y - hs}, {p.x + hs, p.y - hs}, edge);
            line(canvas, {p.x + hs, p.y - hs}, {p.x + hs, p.y + hs}, edge);
            line(canvas, {p.x + hs, p.y + hs}, {p.x - hs, p.y + hs}, edge);
            line(canvas, {p.x - hs, p.y + hs}, {p.x - hs, p.y - hs}, edge);

            // gold corner studs
            for (float dx : {-hs, hs})
                for (float dy : {-hs, hs})
                    rect(canvas, p.x + dx - 1.f, p.y + dy - 1.f, 2.f, 2.f, GOLD);

            circle(canvas, p, 1.5f + g * 2.f, col(BRIGHT[c], 140.f + 115.f * g),
                   sf::Color::Transparent, 0.f, 4);

            if (g > 0.f)
                circle(canvas, p, 4.f + (1.f - g) * 12.f, sf::Color::Transparent,
                       col(BRIGHT[c], g * 220.f), 1.f, 16);
        }
}

void AimGame::drawNotes()
{
    // far to near so close notes sit on top
    for (size_t i = notes.size(); i-- > 0;)
    {
        const AimNote &n = notes[i];
        if (n.done)
            continue;

        float t = (n.time - songTime) / APPROACH;
        if (t > 1.f || t < -HIT_WINDOW / APPROACH)
            continue;

        float z = Z_HIT + t * (Z_FAR - Z_HIT);
        float s = Z_HIT / std::max(z, 20.f);
        float fade = std::clamp((1.f - t) / 0.15f, 0.f, 1.f);
        float a = 255.f * fade;

        sf::Vector2f target = cellPos(n.col, n.row);
        sf::Vector2f c = project(target, s);
        int l = n.col;

        float w = std::max(3.f, std::floor(CELL * 0.66f * s));
        float h = std::floor(w * 1.3f);
        float x = std::floor(c.x - w / 2.f);
        float y = std::floor(c.y - h / 2.f);

        // motion trail toward the portal
        sf::Vector2f t0 = project(target, Z_HIT / (z + 160.f));
        quad(canvas, {c.x - w * 0.35f, y}, {c.x + w * 0.35f, y},
             {t0.x + 0.5f, t0.y}, {t0.x - 0.5f, t0.y},
             col(BRIGHT[l], 90.f * fade), col(BRIGHT[l], 90.f * fade),
             col(BRIGHT[l], 0.f), col(BRIGHT[l], 0.f));

        // tarot-style card
        sf::Color body = mix(DARK[l], MID[l], 0.6f);
        rect(canvas, x, y, w, h, col(body, a), col(BRIGHT[l], a), 1.f);
        rect(canvas, x, y, w, 1.f, col(sf::Color(255, 255, 255), a * 0.5f));

        if (w >= 8.f)
        {
            float gw = std::floor(w * 0.5f);
            float gx = x + std::floor((w - gw) / 2.f);
            float gy = y + std::floor(h * 0.3f);

            rect(canvas, gx, gy, gw, gw, sf::Color::Transparent, col(BRIGHT[l], a), 1.f);
            rect(canvas, gx + std::floor(gw / 2.f), gy + std::floor(gw / 2.f), 1.f, 1.f,
                 col(sf::Color(255, 255, 255), a));
        }
        else
            rect(canvas, x + std::floor(w / 2.f), y + std::floor(h / 2.f), 1.f, 1.f,
                 col(sf::Color(255, 255, 255), a));
    }
}

void AimGame::drawSparks()
{
    for (const auto &s : sparks)
    {
        float k = s.life / s.maxLife;
        float size = k > 0.5f ? 2.f : 1.f;
        rect(canvas, s.pos.x, s.pos.y, size, size, col(s.color, 255.f * k));
    }
}

void AimGame::drawCrosshair()
{
    sf::Vector2f p{std::floor(cursor.x), std::floor(cursor.y)};
    sf::Color white(255, 255, 255);

    line(canvas, p + sf::Vector2f(-7.f, 0.f), p + sf::Vector2f(-3.f, 0.f), white);
    line(canvas, p + sf::Vector2f(3.f, 0.f), p + sf::Vector2f(7.f, 0.f), white);
    line(canvas, p + sf::Vector2f(0.f, -7.f), p + sf::Vector2f(0.f, -3.f), white);
    line(canvas, p + sf::Vector2f(0.f, 3.f), p + sf::Vector2f(0.f, 7.f), white);

    circle(canvas, p, 4.5f, sf::Color::Transparent, TEAL, 1.f, 16);
    rect(canvas, p.x, p.y, 1.f, 1.f, white);
}

void AimGame::drawHud()
{
    float acc = accuracy();
    char rk = rankOf(acc);

    // accuracy (top-left)
    frame(canvas, 5.f, 4.f, 20.f, 20.f, sf::Color(14, 14, 34), STEEL_LIGHT);
    circle(canvas, {15.f, 14.f}, 5.f, sf::Color(30, 120, 140), TEAL, 1.f, 14);
    rect(canvas, 13.f, 11.f, 1.f, 1.f, sf::Color(255, 255, 255));

    bar(canvas, 26.f, 8.f, 66.f, 6.f, acc, sf::Color(40, 200, 210),
        sf::Color(140, 250, 250), false, STEEL);

    // song progress + rank (top-right)
    float prog = endTime > 0.f ? std::clamp(songTime / endTime, 0.f, 1.f) : 0.f;

    frame(canvas, FW - 25.f, 4.f, 20.f, 20.f, sf::Color(14, 10, 28), RUST);
    circle(canvas, {FW - 15.f, 14.f}, 7.f, sf::Color(8, 4, 18),
           col(rankColor(rk), 200.f), 1.f, 16);

    bar(canvas, FW - 26.f - 66.f, 8.f, 66.f, 6.f, prog, sf::Color(170, 90, 215),
        sf::Color(230, 170, 255), true, RUST);

    // combo meter (bottom)
    bar(canvas, (FW - 112.f) / 2.f, 164.f, 112.f, 9.f, std::min(combo, 50) / 50.f,
        sf::Color(210, 160, 70), sf::Color(255, 225, 140), false, STEEL);
}

void AimGame::drawOverlayText(sf::RenderWindow &w, const sf::Font &f)
{
    float acc = accuracy();
    char rk = rankOf(acc);
    char buf[32];

    std::snprintf(buf, sizeof buf, "ACC %.0f%%", acc * 100.f);
    put(w, f, buf, {28.f, 11.f}, 13, TEXT, 0, PX);

    put(w, f, "SCORE " + std::to_string(score), {FW / 2.f, 11.f}, 16, GOLD, 1, PX);
    put(w, f, "SONG", {FW - 29.f, 11.f}, 13, TEXT, 2, PX);
    put(w, f, std::string(1, rk), {FW - 15.f, 14.f}, 16, rankColor(rk), 1, PX);

    put(w, f, "COMBO x" + std::to_string(combo), {FW / 2.f, 168.5f}, 13,
        sf::Color(255, 255, 255), 1, PX);

    if (fbTime > 0.f)
    {
        float a = std::clamp(fbTime / 0.3f, 0.f, 1.f) * 255.f;
        sf::Vector2f p = fbPos + sf::Vector2f(0.f, -(0.6f - fbTime) * 20.f);
        put(w, f, fbText, p, 20, col(fbColor, a), 1, PX);
    }
}