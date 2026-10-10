#include "AsciiCity.h"
#include <utility>
#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace std;

AsciiCity::AsciiCity() { generate(); }

void AsciiCity::enter()
{
    if (inside)
        leaveBuilding();
    px = SPAWN_X;
    py = SPAWN_Y;
    t = 0.f;
    talking = false;
    talkIndex = 0;
    toastT = 0.f;
    onDoor = false;

    // a wandering NPC may be standing on the spawn tile; nudge it away
    // so you never walk in already stuck inside someone
    clearSpawn();
}

AsciiCity::Request AsciiCity::handleEvent(const sf::Event &e)
{
    const auto *k = e.getIf<sf::Event::KeyPressed>();
    if (!k)
        return Request::None;

    if (k->code == sf::Keyboard::Key::Escape && !talking)
    {
        if (inside)
        {
            leaveBuilding();
            return Request::None;
        }
        return Request::Exit;
    }

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
        pendingMap = npcs[talkNpc].mapName;
        if (npcs[talkNpc].shooter)
            return Request::Shooter;
        if (npcs[talkNpc].aimGame)
            return Request::AimGame;
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

    // door: show the toast once when you step on it, not every frame
    bool nowOnDoor = kind[static_cast<int>(py) * W + static_cast<int>(px)] == 4;
    if (nowOnDoor && !onDoor)
    {
        if (inside)
            leaveBuilding();
        else if (const Door *dr = doorAt(static_cast<int>(px), static_cast<int>(py)))
            enterBuilding(*dr);

        onDoor = false; // we were moved off the door tile
        return;         // maps were swapped, skip the NPC loop this frame
    }
    onDoor = nowOnDoor;

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

        // never step onto any tile the player's hitbox touches
        // (the old center-tile check could trap you between corners)
        if (freeTile(nx, ny) && kind[ny * W + nx] != 4 && !overlapsPlayer(nx, ny))
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
    int x0 = static_cast<int>(floor(ox / CW)); // floor, not truncate: ox can be negative
    int y0 = static_cast<int>(floor(oy / CH));

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
        glyph(va, font, n.fixed ? '*' : '&', n.x * CW - ox, n.y * CH - oy, n.color);

    glyph(va, font, '@', px * CW - ox, py * CH - oy, sf::Color::White);

    sf::RenderStates st;
    st.texture = &font.getTexture(SIZE); // after glyphs are cached
    w.draw(va, st);

    // ---------------- HUD ----------------
    label(w, font, "NEO-KOWLOON // WEB DISTRICT", 20.f, 16.f, 16, sf::Color(255, 45, 149));
    label(w, font, "WASD move   SHIFT run   E talk   ESC leave", 20.f, 40.f, 12,
          sf::Color(159, 233, 255));
    label(w, font, placeName, 20.f, 16.f, 16, sf::Color(255, 45, 149));
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
    const float r = PLAYER_R;
    for (float dx : {-r, r})
        for (float dy : {-r, r})
            if (solidAt(static_cast<int>(std::floor(x + dx)),
                        static_cast<int>(std::floor(y + dy))))
                return true;
    return false;
}

// true if a tile overlaps the player's hitbox (same box hit() tests)
bool AsciiCity::overlapsPlayer(int tx, int ty) const
{
    return std::fabs(px - (tx + .5f)) < .5f + PLAYER_R &&
           std::fabs(py - (ty + .5f)) < .5f + PLAYER_R;
}

// push any wandering NPC off the player's current spot to the nearest free ground tile
void AsciiCity::clearSpawn()
{
    for (auto &n : npcs)
    {
        if (n.fixed || !overlapsPlayer(n.x, n.y))
            continue;

        bool moved = false;

        for (int r = 1; r <= 8 && !moved; r++)
            for (int dy = -r; dy <= r && !moved; dy++)
                for (int dx = -r; dx <= r && !moved; dx++)
                {
                    int nx = n.x + dx;
                    int ny = n.y + dy;

                    if (nx < 0 || ny < 0 || nx >= W || ny >= H)
                        continue;
                    if (kind[ny * W + nx] != 0 || !freeTile(nx, ny) || overlapsPlayer(nx, ny))
                        continue;

                    n.x = nx;
                    n.y = ny;
                    moved = true;
                }
    }
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

                const char *nm = NAMES[static_cast<int>(rnd() * 14)];
                sf::Color sc = NEON[static_cast<int>(rnd() * 6)];

                int dxp = (ax + bx) / 2;
                put(dxp, by, 'D', 4, sf::Color(255, 210, 63), false);
                doors.push_back({dxp, by, nm, static_cast<std::uint32_t>(rnd() * 1000000.f) + 1u});
                sign(nm, (ax + bx + 1) / 2, ay + 2, sc);
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
                   "Ready? Let's shoot some pages."};
    npcs.push_back(izuna);

    Npc oldman;
    oldman.x = 50;
    oldman.y = 32;
    oldman.name = "OLD MAN";
    oldman.color = sf::Color(57, 255, 136);
    oldman.fixed = true;
    oldman.aimGame = true;
    oldman.mapName = "aim_oldman"; // maps/aim_oldman.txt (grid map)
    oldman.lines = {"Hungry... Traveler",
                    "The ramen place on the west side never closes.",
                    "Tell them I sent you and get some andsign soup",
                    "...You've got quick hands. Think you can keep up with the beat?"};
    npcs.push_back(oldman);

    static const char *PN[] = {"KAI", "MIRA", "ROOK", "NOVA", "ZED", "LUNA", "ECHO",
                               "VEX", "JUNO", "HALO", "PIXEL", "CIPHER", "RIN", "ONYX"};

    static const vector<string> TALK = {
        "Rain never stops here. I never wanna stop either, pain or happiness",
        "I need to keep going tho",
        "Can't wait here, nor there",
        "Just cant't stay here",
        "Catch me somewhere else"};

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

const AsciiCity::Door *AsciiCity::doorAt(int x, int y) const
{
    for (const auto &d : doors)
        if (d.x == x && d.y == y)
            return &d;
    return nullptr;
}

void AsciiCity::enterBuilding(const Door &d)
{
    savedPos = {px, py};

    cTile = std::move(tile);
    cCol = std::move(col);
    cWin = std::move(win);
    cKind = std::move(kind);
    cSolid = std::move(solidMap);
    cNpcs = std::move(npcs);
    npcs.clear();

    generateInterior(d); // fills tile/col/win/kind/solidMap/npcs
    inside = true;
    placeName = d.name;
}

void AsciiCity::leaveBuilding()
{
    tile = std::move(cTile);
    col = std::move(cCol);
    win = std::move(cWin);
    kind = std::move(cKind);
    solidMap = std::move(cSolid);
    npcs = std::move(cNpcs);

    inside = false;
    placeName = "NEO-KOWLOON // WEB DISTRICT";
    talking = false;

    // savedPos is the door tile; step down onto the street below it
    px = savedPos.x;
    py = savedPos.y + 1.f;
    clearSpawn(); // in case a wandering NPC is standing there
}

void AsciiCity::generateInterior(const Door &d)
{
    tile.assign(N, ' ');
    col.assign(N, sf::Color::Black);
    win.assign(N, sf::Color::Black);
    kind.assign(N, 0);
    solidMap.assign(N, 1); // everything solid until carved out

    auto put = [&](int x, int y, char c, unsigned char k, sf::Color color, bool s)
    {
        int i = y * W + x;
        tile[i] = c;
        kind[i] = k;
        col[i] = color;
        solidMap[i] = s ? 1 : 0;
    };

    seed = d.seed; // same building = same interior every visit

    const int RX = 40, RY = 28, RW = 24, RH = 14;
    sf::Color trim = NEON[static_cast<int>(rnd() * 6)];
    sf::Color accent = NEON[static_cast<int>(rnd() * 6)];

    for (int y = RY; y < RY + RH; y++)
        for (int x = RX; x < RX + RW; x++)
        {
            bool edge = x == RX || x == RX + RW - 1 || y == RY || y == RY + RH - 1;
            if (edge)
                put(x, y, y == RY ? '=' : '|', 1, y == RY ? trim : sf::Color(51, 67, 111), true);
            else
                put(x, y, (x + y) % 2 ? ':' : '.', 0, sf::Color(44, 56, 86), false);
        }

    // shop name on the back wall
    int sx = RX + RW / 2 - static_cast<int>(d.name.size()) / 2;
    for (size_t n = 0; n < d.name.size(); n++)
        put(sx + static_cast<int>(n), RY, d.name[n], 3, accent, true);

    // counter
    for (int x = RX + 4; x < RX + RW - 4; x++)
        put(x, RY + 3, '=', 1, accent, true);

    // a few random tables / plants in the front half
    for (int n = 0; n < 6; n++)
    {
        int x = RX + 2 + static_cast<int>(rnd() * (RW - 4));
        int y = RY + 6 + static_cast<int>(rnd() * 5);
        bool plant = rnd() < .4f;
        put(x, y, plant ? 'Y' : 'o', 1, plant ? sf::Color(47, 174, 106) : sf::Color(180, 140, 90), true);
    }

    // exit door, bottom center
    int ex = RX + RW / 2, ey = RY + RH - 1;
    put(ex, ey, 'D', 4, sf::Color(255, 210, 63), false);
    // keep the tiles in front of the door clear
    put(ex, ey - 1, '.', 0, sf::Color(44, 56, 86), false);
    put(ex, ey - 2, '.', 0, sf::Color(44, 56, 86), false);

    // shopkeeper behind the counter
    Npc keeper;
    keeper.x = RX + RW / 2;
    keeper.y = RY + 2;
    keeper.name = "KEEPER";
    keeper.color = accent;
    keeper.fixed = true;
    keeper.lines = {"Welcome to " + d.name + ".",
                    "Nothing for sale yet. Come back later."};
    npcs.push_back(keeper);

    px = ex + .5f;
    py = ey - 1.5f;
}