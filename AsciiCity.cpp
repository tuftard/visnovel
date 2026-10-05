#include "AsciiCity.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace std;

AsciiCity::AsciiCity() { generate(); }

void AsciiCity::enter()
{
    px = 52.5f;
    py = 34.5f;
    talking = false;
    toastT = 0.f;
}

AsciiCity::Request AsciiCity::handleEvent(const sf::Event &e)
{
    const auto *k = e.getIf<sf::Event::KeyPressed>();
    if (!k)
        return Request::None;

    if (k->code == sf::Keyboard::Key::Escape && !talking)
        return Request::Exit;

    bool confirm = k->code == sf::Keyboard::Key::E ||
                   k->code == sf::Keyboard::Key::Enter ||
                   k->code == sf::Keyboard::Key::Space;
    if (!confirm)
        return Request::None;

    if (!talking)
    {
        int n = nearestNpc(2.5f);
        if (n >= 0)
        {
            talkNpc = n;
            talkIndex = 0;
            talking = true;
        }
        return Request::None;
    }

    talkIndex++;
    if (talkIndex >= npcs[talkNpc].lines.size())
    {
        talking = false;
        if (npcs[talkNpc].shooter)
            return Request::Shooter;
    }
    return Request::None;
}

void AsciiCity::update(float dt)
{
    t += dt;
    if (toastT > 0.f)
        toastT -= dt;
    if (talking)
        return;

    sf::Vector2f d{0.f, 0.f};
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
        d.y -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
        d.y += 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        d.x -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        d.x += 1.f;

    float len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len > 0.f)
        d /= len;

    float sp = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ? 11.f : 7.f;
    sp *= min(dt, 0.05f);

    if (!hit(px + d.x * sp, py))
        px += d.x * sp;
    if (!hit(px, py + d.y * sp))
        py += d.y * sp;

    if (kind[static_cast<int>(py) * W + static_cast<int>(px)] == 4)
    {
        toastText = "DOOR LOCKED - finish the code first.";
        toastT = 2.f;
    }

    // wandering NPCs
    for (auto &n : npcs)
    {
        if (n.fixed)
            continue;

        n.timer -= dt;
        if (n.timer > 0.f)
            continue;

        n.timer = 0.4f + (rand() % 120) / 100.f;

        static const int dx[4] = {1, -1, 0, 0};
        static const int dy[4] = {0, 0, 1, -1};
        int r = rand() % 4;
        int nx = n.x + dx[r];
        int ny = n.y + dy[r];

        if (freeTile(nx, ny) && kind[ny * W + nx] != 4 &&
            !(nx == static_cast<int>(px) && ny == static_cast<int>(py)))
        {
            n.x = nx;
            n.y = ny;
        }
    }
}

void AsciiCity::draw(sf::RenderWindow &w, const sf::Font &font) const
{
    w.setView(w.getDefaultView());
    w.clear(sf::Color(5, 6, 15));

    const float mapW = W * CW;
    const float mapH = H * CH;

    float ox = (mapW <= 1280.f) ? (mapW - 1280.f) / 2.f
                                : clamp(px * CW - 640.f, 0.f, mapW - 1280.f);
    float oy = (mapH <= 720.f) ? (mapH - 720.f) / 2.f
                               : clamp(py * CH - 360.f, 0.f, mapH - 720.f);
    int x0 = static_cast<int>(ox / CW);
    int y0 = static_cast<int>(oy / CH);

    sf::VertexArray va(sf::PrimitiveType::Triangles);

    for (int y = y0; y < y0 + 38; y++)
    {
        if (y < 0 || y >= H)
            continue;

        for (int x = x0; x < x0 + 119; x++)
        {
            if (x < 0 || x >= W)
                continue;

            int i = y * W + x;
            char c = tile[i];
            if (c == ' ')
                continue;

            float dist = std::sqrt((x - px) * (x - px) + (y - py) * (y - py));
            sf::Color color = col[i];
            float a;

            if (kind[i] == 2) // window: some of them flicker
            {
                int n = (x * 7919 + y * 104729) % 97;
                bool lit = n > 22 || (static_cast<int>(t / 1.4f) + n) % 6 < 3;
                color = lit ? win[i] : col[i];
                a = lit ? max(.5f, 1.25f - dist / 40.f) : .7f;
            }
            else if (kind[i] == 3)
                a = 1.f;
            else if (kind[i] == 0)
                a = clamp(1.15f - dist / 20.f, .14f, 1.f);
            else
                a = clamp(1.3f - dist / 34.f, .35f, 1.f);

            color.a = static_cast<std::uint8_t>(255.f * a);
            glyph(va, font, c, x * CW - ox, y * CH - oy, color);
        }
    }

    for (const auto &n : npcs)
        glyph(va, font, n.fixed ? '^' : '&', n.x * CW - ox, n.y * CH - oy, n.color);

    glyph(va, font, '@', px * CW - ox, py * CH - oy, sf::Color::White);

    sf::RenderStates st;
    st.texture = &font.getTexture(SIZE); // after glyphs are cached
    w.draw(va, st);

    // ---------------- HUD ----------------
    label(w, font, "NEO-KOWLOON // WEB DISTRICT", 20.f, 16.f, 16, sf::Color(255, 45, 149));
    label(w, font, "WASD move   SHIFT run   E talk   ESC leave", 20.f, 40.f, 12,
          sf::Color(159, 233, 255));

    if (!talking && nearestNpc(2.5f) >= 0)
        label(w, font, "[E] talk", 600.f, 640.f, 16, sf::Color(240, 200, 110));

    if (toastT > 0.f)
        label(w, font, toastText, 20.f, 680.f, 16, sf::Color(255, 210, 63));

    if (talking && talkIndex < npcs[talkNpc].lines.size())
    {
        sf::RectangleShape box({1160.f, 140.f});
        box.setPosition({60.f, 540.f});
        box.setFillColor(sf::Color(10, 10, 25, 235));
        box.setOutlineColor(sf::Color(255, 255, 255, 180));
        box.setOutlineThickness(2.f);
        w.draw(box);

        label(w, font, npcs[talkNpc].name, 80.f, 552.f, 20, npcs[talkNpc].color);
        label(w, font, npcs[talkNpc].lines[talkIndex], 80.f, 590.f, 22, sf::Color::White);
        label(w, font, "[E]", 1170.f, 650.f, 14, sf::Color(240, 200, 110));
    }
}

// ---------------------------------------------------------------
// helpers
// ---------------------------------------------------------------

float AsciiCity::rnd()
{
    seed = seed * 1664525u + 1013904223u;
    return static_cast<float>(seed >> 8) / 16777216.f; // always < 1.0
}

bool AsciiCity::solidAt(int x, int y) const
{
    if (x < 0 || y < 0 || x >= W || y >= H)
        return true;
    if (solidMap[y * W + x])
        return true;
    for (const auto &n : npcs)
        if (n.x == x && n.y == y)
            return true;
    return false;
}

bool AsciiCity::hit(float x, float y) const
{
    const float r = 0.3f;
    for (float dx : {-r, r})
        for (float dy : {-r, r})
            if (solidAt(static_cast<int>(std::floor(x + dx)),
                        static_cast<int>(std::floor(y + dy))))
                return true;
    return false;
}

int AsciiCity::nearestNpc(float range) const
{
    int best = -1;
    float bestD = range * range;
    for (int i = 0; i < static_cast<int>(npcs.size()); i++)
    {
        float dx = px - (npcs[i].x + .5f), dy = py - (npcs[i].y + .5f);
        float d = dx * dx + dy * dy;
        if (d < bestD)
        {
            bestD = d;
            best = i;
        }
    }
    return best;
}

// one textured quad from the font atlas
void AsciiCity::glyph(sf::VertexArray &va, const sf::Font &f, char c,
                      float x, float y, sf::Color color)
{
    sf::Glyph g = f.getGlyph(static_cast<char32_t>(c), SIZE, false);

    sf::Vector2f p0{x + g.bounds.position.x, y + BASE + g.bounds.position.y};
    sf::Vector2f p1{p0.x + g.bounds.size.x, p0.y + g.bounds.size.y};
    sf::Vector2f t0(g.textureRect.position);
    sf::Vector2f t1 = t0 + sf::Vector2f(g.textureRect.size);

    va.append(sf::Vertex{p0, color, t0});
    va.append(sf::Vertex{{p1.x, p0.y}, color, {t1.x, t0.y}});
    va.append(sf::Vertex{p1, color, t1});
    va.append(sf::Vertex{p0, color, t0});
    va.append(sf::Vertex{p1, color, t1});
    va.append(sf::Vertex{{p0.x, p1.y}, color, {t0.x, t1.y}});
}

void AsciiCity::label(sf::RenderWindow &w, const sf::Font &f, const string &s,
                      float x, float y, unsigned size, sf::Color c)
{
    sf::Text txt(f, s, size);
    txt.setFillColor(c);
    txt.setPosition({x, y});
    w.draw(txt);
}

void AsciiCity::generate()
{
    tile.assign(N, ' ');
    col.assign(N, sf::Color::Black);
    win.assign(N, sf::Color::Black);
    kind.assign(N, 0);
    solidMap.assign(N, 0);

    auto put = [&](int x, int y, char c, unsigned char k, sf::Color color, bool s)
    {
        int i = y * W + x;
        tile[i] = c;
        kind[i] = k;
        col[i] = color;
        solidMap[i] = s ? 1 : 0;
    };

    auto sign = [&](const string &txt, int cx, int y, sf::Color color)
    {
        int sx = cx - static_cast<int>(txt.size()) / 2;
        for (size_t n = 0; n < txt.size(); n++)
            put(sx + static_cast<int>(n), y, txt[n], 3, color, true);
    };

    // streets + sidewalks
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
        {
            bool hs = y % 18 < 4, vs = x % 20 < 4;
            if (hs || vs)
            {
                char c = '.';
                sf::Color cc(40, 60, 90);
                if (hs && !vs && y % 18 == 1 && x % 4 < 2)
                {
                    c = '-';
                    cc = sf::Color(122, 106, 36);
                }
                else if (vs && !hs && x % 20 == 1 && y % 4 < 2)
                {
                    c = '|';
                    cc = sf::Color(122, 106, 36);
                }
                put(x, y, c, 0, cc, false);
            }
            else
                put(x, y, ',', 0, sf::Color(50, 66, 96), false);
        }

    static const char *NAMES[] = {
        "NEO-RAMEN", "HTML//BAR", "DATA PAWN", "ARCADE-X", "SHIRO.CPP",
        "VOID MART", "NOODLE-404", "GLITCH LAB", "<DIV> HOTEL",
        "CYBER-DOJO", "IZUNA.HTML", "NEON TEA", "BYTE CLUB", "SORA CHESS"};

    const sf::Color walls[4] = {{20, 26, 51}, {26, 22, 51}, {16, 34, 46}, {36, 19, 34}};

    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 5; i++)
        {
            int x0 = 20 * i + 4, y0 = 18 * j + 4;

            if (i == 2 && j == 1) // plaza
            {
                for (int y = y0; y < y0 + 14; y++)
                    for (int x = x0; x < x0 + 16; x++)
                        put(x, y, (x + y) % 2 ? '+' : ',', 0, sf::Color(52, 71, 107), false);

                int cx = x0 + 8, cy = y0 + 7;
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++)
                        put(cx + dx, cy + dy, '~', 0, sf::Color(43, 143, 214), true);
                put(cx, cy, '*', 3, sf::Color(255, 45, 149), true);
                sign("WEB DISTRICT", cx, cy - 4, sf::Color(0, 229, 255));
            }
            else if (rnd() < .16f) // park
            {
                for (int y = y0; y < y0 + 14; y++)
                    for (int x = x0; x < x0 + 16; x++)
                    {
                        if (rnd() < .16f && y < y0 + 13)
                            put(x, y, 'Y', 0, sf::Color(47, 174, 106), true);
                        else
                            put(x, y, rnd() < .5f ? '"' : '\'', 0, sf::Color(42, 122, 76), false);
                    }
            }
            else // building
            {
                int ax = x0 + static_cast<int>(rnd() * 2);
                int bx = x0 + 15 - static_cast<int>(rnd() * 2);
                int ay = y0 + static_cast<int>(rnd() * 5);
                int by = y0 + 13;

                sf::Color wall = walls[static_cast<int>(rnd() * 4)];
                sf::Color wc = NEON[static_cast<int>(rnd() * 6)];
                sf::Color trim = NEON[static_cast<int>(rnd() * 6)];

                for (int y = ay; y <= by; y++)
                    for (int x = ax; x <= bx; x++)
                    {
                        if (y == ay)
                            put(x, y, '=', 1, trim, true);
                        else if (x == ax || x == bx)
                            put(x, y, '|', 1, sf::Color(51, 67, 111), true);
                        else if ((x + y) % 2 == 0)
                        {
                            put(x, y, '+', 2, wall, true);
                            win[y * W + x] = wc;
                        }
                        else
                            put(x, y, '#', 1, wall, true);
                    }

                put((ax + bx) / 2, by, 'D', 4, sf::Color(255, 210, 63), false);
                sign(NAMES[static_cast<int>(rnd() * 14)], (ax + bx + 1) / 2, ay + 2,
                     NEON[static_cast<int>(rnd() * 6)]);
            }
        }

    // Izuna stands by the fountain
    Npc izuna;
    izuna.x = 52;
    izuna.y = 32;
    izuna.name = "IZUNA";
    izuna.color = sf::Color(255, 200, 90);
    izuna.fixed = true;
    izuna.shooter = true;
    izuna.lines = {"Welcome to the Web District.",
                   "Everything here is built out of tags.",
                   "Write the right HTML and your shots will land.",
                   "Ready? Let's shoot some pages."};
    npcs.push_back(izuna);

    static const char *PN[] = {"KAI", "MIRA", "ROOK", "NOVA", "ZED", "LUNA", "ECHO",
                               "VEX", "JUNO", "HALO", "PIXEL", "CIPHER", "RIN", "ONYX"};
    static const vector<string> TALK = {
        "Rain never stops here. Neither do the ads.",
        "Heard the arcade on the east side is still up.",
        "Don't trust the noodle shop named 404.",
        "I sell memory. Cheap. Slightly used.",
        "The fountain isn't water. Don't ask."};

    for (int n = 0; n < 14; n++)
    {
        int x, y, tries = 0;
        do
        {
            x = static_cast<int>(rnd() * W);
            y = static_cast<int>(rnd() * H);
        } while ((!freeTile(x, y) || kind[y * W + x] != 0) && ++tries < 500);

        Npc npc;
        npc.x = x;
        npc.y = y;
        npc.name = PN[n];
        npc.color = NEON[n % 6];
        npc.lines = TALK;
        npc.timer = rnd();
        npcs.push_back(npc);
    }
}