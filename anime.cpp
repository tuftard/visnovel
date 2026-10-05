// cd "C:\Users\Noah\Desktop\visual novel"

// // g++ anime.cpp Common.cpp ShiroTerminal.cpp RhythmTrack.cpp soraeditor.cpp HtmlShooterScreen.cpp AsciiCity.cpp -I"C:\SFML\include" -L"C:\SFML\lib" -lsfml-graphics -lsfml-window -lsfml-system -o anime.exe
// ---------------- GIT: stop tracking exe + backups ----------------
// Run each line separately in PowerShell, in this folder.
//
// type .gitignore
//   (if it prints nothing, run the next line to fill it)
// Set-Content .gitignore "*.exe`ntest.cpp`nanime_backup*.cpp`nanime_mybackup.cpp`nanime_before_build.cpp"
//
// git rm --cached anime.exe anime_mybackup.cpp anime_backup_cursor.cpp anime_before_build.cpp
// git add .
// git commit -m "stop tracking exe and backups"
// git push
//
// git status   (should say: nothing to commit, working tree clean)
//
// ---------------- GIT: everyday saving ----------------
// git add .
// git commit -m "split HtmlShooterScreen into its own files"
// git push
//
// git restore anime.cpp   (undo uncommitted edits)
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cctype>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <utility>
#include <span>
#include <windows.h>
#include <sstream>
#include <cstdint>
#include "Common.h"
#include "ShiroTerminal.h"
#include "RhythmTrack.h"
#include "soraeditor.h"
#include "AsciiCity.h"
#include "HtmlShooterScreen.h"
#include "PledgeScreen.h"
#include "BattleStats.h"
#include "ResultsScreen.h"
#include "GradeScene.h"
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/View.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <SFML/System/Clock.hpp>

using namespace std;

// =============================================================
// BASE64 DECODER
// =============================================================

vector<unsigned char> decodeBase64(const string &input)
{
    static const string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    vector<unsigned char> output;

    int value = 0;
    int bits = -8;

    for (unsigned char c : input)
    {
        if (c == '=')
            break;

        size_t position = chars.find(c);

        if (position == string::npos)
            continue;

        value = (value << 6) + static_cast<int>(position);
        bits += 6;

        if (bits >= 0)
        {
            output.push_back(
                static_cast<unsigned char>(
                    (value >> bits) & 0xFF));

            bits -= 8;
        }
    }

    return output;
}

// =============================================================
// LOAD PISKEL FRAMES
// =============================================================

bool loadPiskelFrames(
    const string &fileName,
    int layerIndex,
    vector<sf::Texture> &frames,
    int &frameWidth,
    int &frameHeight)
{
    ifstream file(fileName);

    if (!file.is_open())
    {
        cout << "FAILED TO OPEN PISKEL: "
             << fileName << "\n";

        return false;
    }

    string piskel;
    string line;

    while (getline(file, line))
    {
        piskel += line;
    }

    file.close();

    size_t widthPosition =
        piskel.find("\"width\":");

    if (widthPosition == string::npos)
    {
        cout << "FAILED TO FIND PISKEL WIDTH\n";
        return false;
    }

    frameWidth = stoi(
        piskel.substr(
            widthPosition + 8));

    size_t heightPosition =
        piskel.find("\"height\":");

    if (heightPosition == string::npos)
    {
        cout << "FAILED TO FIND PISKEL HEIGHT\n";
        return false;
    }

    frameHeight = stoi(
        piskel.substr(
            heightPosition + 9));

    if (frameWidth <= 0 || frameHeight <= 0)
    {
        cout << "INVALID PISKEL FRAME SIZE\n";
        return false;
    }

    size_t searchPosition = 0;

    for (int i = 0; i <= layerIndex; i++)
    {
        size_t pngStart =
            piskel.find(
                "data:image/png;base64,",
                searchPosition);

        if (pngStart == string::npos)
        {
            cout << "FAILED TO FIND PISKEL LAYER "
                 << layerIndex << "\n";

            return false;
        }

        searchPosition = pngStart + 22;

        if (i == layerIndex)
        {
            size_t dataStart =
                pngStart + 22;

            size_t dataEnd =
                piskel.find(
                    '"',
                    dataStart);

            if (dataEnd == string::npos)
            {
                cout << "FAILED TO READ PISKEL LAYER\n";
                return false;
            }

            string encoded =
                piskel.substr(
                    dataStart,
                    dataEnd - dataStart);

            vector<unsigned char> pngData =
                decodeBase64(encoded);

            if (pngData.empty())
            {
                cout << "FAILED TO DECODE PISKEL LAYER\n";
                return false;
            }

            sf::Image image;

            if (!image.loadFromMemory(
                    pngData.data(),
                    pngData.size()))
            {
                cout << "FAILED TO LOAD PISKEL IMAGE\n";
                return false;
            }

            unsigned int imageWidth =
                image.getSize().x;

            unsigned int imageHeight =
                image.getSize().y;

            if (
                imageWidth <
                    static_cast<unsigned int>(frameWidth) ||
                imageHeight <
                    static_cast<unsigned int>(frameHeight))
            {
                cout << "PISKEL IMAGE IS SMALLER THAN FRAME SIZE\n";
                return false;
            }

            int frameCount =
                static_cast<int>(
                    imageWidth /
                    static_cast<unsigned int>(frameWidth));

            if (frameCount <= 0)
            {
                cout << "NO PISKEL FRAMES FOUND\n";
                return false;
            }

            frames.clear();

            for (int frame = 0; frame < frameCount; frame++)
            {
                sf::Image frameImage({static_cast<unsigned int>(frameWidth),
                                      static_cast<unsigned int>(frameHeight)});

                for (int y = 0; y < frameHeight; y++)
                {
                    for (int x = 0; x < frameWidth; x++)
                    {
                        frameImage.setPixel(
                            {static_cast<unsigned int>(x),
                             static_cast<unsigned int>(y)},
                            image.getPixel({static_cast<unsigned int>(
                                                frame * frameWidth + x),
                                            static_cast<unsigned int>(y)}));
                    }
                }

                sf::Texture frameTexture;

                if (!frameTexture.loadFromImage(frameImage))
                {
                    cout << "FAILED TO LOAD PISKEL FRAME "
                         << frame << "\n";

                    return false;
                }

                frameTexture.setSmooth(false);

                frames.push_back(
                    std::move(frameTexture));
            }

            cout << "LOADED "
                 << frames.size()
                 << " PISKEL FRAMES FROM LAYER "
                 << layerIndex
                 << "\n";

            return true;
        }
    }

    return false;
}

// =============================================================
// OVERWORLD  (tile map, walking, NPC talk, grass encounters)
// =============================================================
class Overworld
{
public:
    enum class Request
    {
        None,
        GrassFight,
        BossFight,
        ShooterFight,
        EnterCity
    };

    Overworld()
    {
        // ---------------- room 0: Shiro's room ----------------
        tiles[0].assign(H, string(W, '.'));
        border(tiles[0]);

        for (int y = 5; y < 11; y++)
            for (int x = 12; x < 21; x++)
                tiles[0][y][x] = 'g';

        for (int y = 3; y < 8; y++)
            tiles[0][y][7] = '#';

        tiles[0][7][W - 1] = 'D'; // exit door (east), locked until Shiro is beaten

        // ---------------- room 1: Web District ----------------
        tiles[1].assign(H, string(W, '.'));
        border(tiles[1]);

        for (int y = 3; y < 10; y++)
        {
            tiles[1][y][11] = '#';
            tiles[1][y][17] = '#';
        }

        tiles[1][7][0] = 'B'; // back door (west)

        Npc shiro;
        shiro.room = 0;
        shiro.pos = {TILE * 22.5f, TILE * 3.5f};
        shiro.name = "SHIRO";
        shiro.color = sf::Color(150, 235, 215);
        shiro.boss = true;
        shiro.lines = {"You want to get past me?",
                       "Then prove you can write real code."};
        shiro.afterLines = {"...Fine. You win. Go on ahead."};
        npcs.push_back(shiro);

        Npc sora;
        sora.room = 0;
        sora.pos = {TILE * 4.5f, TILE * 3.5f};
        sora.name = "SORA";
        sora.color = sf::Color(240, 150, 110);
        sora.lines = {"Tall grass is full of bugs.",
                      "Walk in it if you want to fight."};
        sora.afterLines = sora.lines;
        npcs.push_back(sora);

        Npc izuna;
        izuna.room = 1;
        izuna.pos = {TILE * 22.5f, TILE * 7.5f};
        izuna.name = "IZUNA";
        izuna.color = sf::Color(255, 200, 90);
        izuna.shooter = true;
        izuna.lines = {"Welcome to the Web District.",
                       "Everything here is built out of tags.",
                       "Write the right HTML and your shots will land.",
                       "Ready? Let's shoot some pages."};
        izuna.afterLines = izuna.lines;
        npcs.push_back(izuna);

        respawn();
    }

    void respawn()
    {
        room = 0;
        pos = {TILE * 3.f, TILE * 8.f};
        stepAccum = 0.f;
        talking = false;
    }

    void returnFromCity()
    {
        room = 0;
        pos = {TILE * (W - 2.f), TILE * 7.5f};
        stepAccum = 0.f;
    }

    void setUnlocked(bool u) { unlocked = u; }

    Request handleEvent(const sf::Event &e, bool beaten)
    {
        const auto *k = e.getIf<sf::Event::KeyPressed>();

        if (!k)
            return Request::None;

        bool confirm = k->code == sf::Keyboard::Key::E ||
                       k->code == sf::Keyboard::Key::Enter ||
                       k->code == sf::Keyboard::Key::Space;

        if (!confirm)
            return Request::None;

        if (!talking)
        {
            int n = nearestNpc(64.f);

            if (n >= 0)
            {
                talkNpc = n;
                talkIndex = 0;
                talking = true;
                talkLines = (npcs[n].boss && beaten) ? npcs[n].afterLines
                                                     : npcs[n].lines;
            }
            return Request::None;
        }

        talkIndex++;

        if (talkIndex >= talkLines.size())
        {
            talking = false;

            if (npcs[talkNpc].boss && !beaten)
                return Request::BossFight;

            if (npcs[talkNpc].shooter)
                return Request::ShooterFight;
        }

        return Request::None;
    }

    Request update(float dt)
    {
        if (talking)
            return Request::None;

        sf::Vector2f dir{0.f, 0.f};

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            dir.y -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            dir.y += 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            dir.x -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            dir.x += 1.f;

        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);

        if (len > 0.f)
            dir /= len;

        bool running = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift);
        float speed = running ? 230.f : 150.f;

        sf::Vector2f step = dir * speed * min(dt, 0.05f);
        sf::Vector2f old = pos;
        sf::Vector2f p = old;

        if (!blocked({p.x + step.x, p.y}))
            p.x += step.x;
        if (!blocked({p.x, p.y + step.y}))
            p.y += step.y;

        pos = p;

        float mx = p.x - old.x;
        float my = p.y - old.y;
        float moved = std::sqrt(mx * mx + my * my);

        if (moved > 0.f)
            walkTime += dt;

        // ---- doors ----
        char under = tileAtPixel(p);

        if (under == 'D' && unlocked && room == 0)
        {
            return Request::EnterCity;
        }

        if (under == 'B' && room == 1)
        {
            room = 0;
            pos = {TILE * (W - 1.8f), TILE * 7.5f};
            return Request::None;
        }

        // ---- grass encounters ----
        if (moved > 0.f && under == 'g')
        {
            stepAccum += moved;

            if (stepAccum >= TILE)
            {
                stepAccum = 0.f;

                if (rand() % 100 < 20)
                    return Request::GrassFight;
            }
        }

        return Request::None;
    }

    void draw(sf::RenderWindow &w, const sf::Font &font, int hp, int maxHp,
              bool beaten, float transition) const
    {
        w.clear(sf::Color(8, 8, 18));

        const float screenW = 1280.f;
        const float screenH = 720.f;
        float worldW = W * TILE;
        float worldH = H * TILE;

        float camX = clamp(pos.x, screenW / 2.f, max(screenW / 2.f, worldW - screenW / 2.f));
        float camY = clamp(pos.y, screenH / 2.f, max(screenH / 2.f, worldH - screenH / 2.f));

        if (worldW <= screenW)
            camX = worldW / 2.f;
        if (worldH <= screenH)
            camY = worldH / 2.f;

        sf::View camera(sf::Vector2f(camX, camY), sf::Vector2f(screenW, screenH));
        w.setView(camera);

        for (int y = 0; y < H; y++)
        {
            for (int x = 0; x < W; x++)
            {
                char t = tileAt(x, y);
                sf::Color c(40, 46, 70);

                if (t == '#')
                    c = sf::Color(20, 22, 40);
                else if (t == 'g')
                    c = sf::Color(30, 90, 70);
                else if (t == 'D')
                    c = unlocked ? sf::Color(240, 200, 110) : sf::Color(110, 40, 50);
                else if (t == 'B')
                    c = sf::Color(90, 235, 235);
                else if ((x + y) % 2 == 0)
                    c = (room == 1) ? sf::Color(52, 44, 84) : sf::Color(44, 50, 76);
                else if (room == 1)
                    c = sf::Color(44, 38, 72);

                box(w, x * TILE, y * TILE, TILE, TILE, c);
            }
        }

        struct Sprite
        {
            sf::Vector2f p;
            sf::Color c;
            string label;
            bool player;
        };

        vector<Sprite> sprites;
        sprites.push_back({pos, sf::Color(90, 235, 235), "", true});

        for (const auto &n : npcs)
            if (n.room == room)
                sprites.push_back({n.pos, n.color, n.name, false});

        sort(sprites.begin(), sprites.end(),
             [](const Sprite &a, const Sprite &b)
             { return a.p.y < b.p.y; });

        for (const auto &s : sprites)
        {
            float bob = s.player ? std::sin(walkTime * 14.f) * 1.5f : 0.f;

            box(w, s.p.x - 16.f, s.p.y - 44.f + bob, 32.f, 44.f, s.c,
                sf::Color::White, 1.f);

            if (!s.label.empty())
                label(w, font, s.label, s.p.x - 22.f, s.p.y - 64.f, 12, sf::Color::White);
        }

        // ---------------- screen-space UI ----------------
        w.setView(w.getDefaultView());

        box(w, 20.f, 20.f, 200.f, 16.f, sf::Color(10, 10, 30),
            sf::Color(130, 150, 200), 1.f);
        box(w, 21.f, 21.f, 198.f * hp / max(1, maxHp), 14.f, sf::Color(40, 200, 210));
        label(w, font, "HP " + to_string(hp) + "/" + to_string(maxHp), 20.f, 42.f, 14,
              sf::Color::White);

        if (room == 1)
            label(w, font, "WEB DISTRICT - talk to Izuna (east side)", 20.f, 64.f, 12,
                  sf::Color(190, 110, 235));
        else if (!beaten)
            label(w, font, "Find Shiro (top right) to fight the boss", 20.f, 64.f, 12,
                  sf::Color(240, 200, 110));
        else
            label(w, font, "Shiro defeated! The east door is open.", 20.f, 64.f, 12,
                  sf::Color(0, 235, 190));

        if (!talking && nearestNpc(64.f) >= 0)
            label(w, font, "[E] talk", screenW / 2.f - 30.f, screenH - 60.f, 16,
                  sf::Color(240, 200, 110));

        if (talking && talkIndex < talkLines.size())
        {
            box(w, 60.f, screenH - 190.f, screenW - 120.f, 150.f,
                sf::Color(10, 10, 25, 235), sf::Color(255, 255, 255, 180), 2.f);
            box(w, 80.f, screenH - 214.f, 170.f, 36.f,
                sf::Color(10, 10, 25, 245), sf::Color(255, 255, 255, 180), 2.f);

            label(w, font, npcs[talkNpc].name, 96.f, screenH - 208.f, 20, sf::Color::White);
            label(w, font, talkLines[talkIndex], 90.f, screenH - 160.f, 22, sf::Color::White);
            label(w, font, "[E]", screenW - 130.f, screenH - 70.f, 14, sf::Color(240, 200, 110));
        }

        if (transition >= 0.f)
        {
            bool flash = static_cast<int>(transition * 14.f) % 2 == 0;

            box(w, 0.f, 0.f, screenW, screenH,
                flash ? sf::Color(255, 255, 255, 90) : sf::Color(0, 0, 0, 90));

            float bar = (screenH / 2.f) * clamp(transition, 0.f, 1.f);
            box(w, 0.f, 0.f, screenW, bar, sf::Color::Black);
            box(w, 0.f, screenH - bar, screenW, bar, sf::Color::Black);
        }
    }

private:
    struct Npc
    {
        int room = 0;
        sf::Vector2f pos;
        string name;
        sf::Color color{200, 120, 220};
        bool boss = false;
        bool shooter = false;
        vector<string> lines;
        vector<string> afterLines;
    };

    static constexpr float TILE = 48.f;
    static constexpr int W = 28;
    static constexpr int H = 14;

    vector<string> tiles[2]; // '#' wall  '.' floor  'g' grass  'D' exit door  'B' back door
    vector<Npc> npcs;

    int room = 0;
    bool unlocked = false;

    sf::Vector2f pos{TILE * 3.f, TILE * 8.f};
    float stepAccum = 0.f;
    float walkTime = 0.f;

    bool talking = false;
    int talkNpc = 0;
    size_t talkIndex = 0;
    vector<string> talkLines;

    static void border(vector<string> &t)
    {
        for (int x = 0; x < W; x++)
            t[0][x] = t[H - 1][x] = '#';
        for (int y = 0; y < H; y++)
            t[y][0] = t[y][W - 1] = '#';
    }

    char tileAt(int tx, int ty) const
    {
        if (tx < 0 || ty < 0 || tx >= W || ty >= H)
            return '#';
        return tiles[room][ty][tx];
    }

    char tileAtPixel(sf::Vector2f p) const
    {
        return tileAt(static_cast<int>(std::floor(p.x / TILE)),
                      static_cast<int>(std::floor(p.y / TILE)));
    }

    bool solid(char t) const
    {
        return t == '#' || (t == 'D' && !unlocked);
    }

    bool blocked(sf::Vector2f p) const
    {
        const float halfW = 12.f;

        for (float dx : {-halfW, halfW})
            for (float dy : {-8.f, 0.f})
                if (solid(tileAtPixel({p.x + dx, p.y + dy})))
                    return true;

        for (const auto &n : npcs)
        {
            if (n.room != room)
                continue;

            float ex = p.x - n.pos.x;
            float ey = p.y - n.pos.y;

            if (ex * ex + ey * ey < 28.f * 28.f)
                return true;
        }

        return false;
    }

    int nearestNpc(float range) const
    {
        int best = -1;
        float bestD = range * range;

        for (int i = 0; i < static_cast<int>(npcs.size()); i++)
        {
            if (npcs[i].room != room)
                continue;

            float dx = pos.x - npcs[i].pos.x;
            float dy = pos.y - npcs[i].pos.y;
            float d = dx * dx + dy * dy;

            if (d < bestD)
            {
                bestD = d;
                best = i;
            }
        }
        return best;
    }

    static void box(sf::RenderWindow &w, float x, float y, float wd, float h,
                    sf::Color fill, sf::Color outline = sf::Color::Transparent,
                    float thick = 0.f)
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

    static void label(sf::RenderWindow &w, const sf::Font &f, const string &s,
                      float x, float y, unsigned size, sf::Color c)
    {
        sf::Text t(f, s, size);
        t.setFillColor(c);
        t.setPosition({x, y});
        w.draw(t);
    }
};


// horizontal CRT lines over a rectangle
sf::VertexArray makeScanlines(sf::FloatRect area, float spacing, sf::Color color)
{
    sf::VertexArray v(sf::PrimitiveType::Lines);

    for (float y = area.position.y; y < area.position.y + area.size.y; y += spacing)
    {
        v.append(sf::Vertex{{area.position.x, y + 0.5f}, color});
        v.append(sf::Vertex{{area.position.x + area.size.x, y + 0.5f}, color});
    }

    return v;
}

// dashed border like the "ようこそ" box
void drawDashedRect(sf::RenderWindow &w, sf::FloatRect r, sf::Color c,
                    float dash = 6.f, float gap = 4.f, float t = 2.f)
{
    sf::RectangleShape d;
    d.setFillColor(c);

    float right = r.position.x + r.size.x;
    float bottom = r.position.y + r.size.y;

    for (float x = r.position.x; x < right; x += dash + gap)
    {
        d.setSize({min(dash, right - x), t});
        d.setPosition({x, r.position.y});
        w.draw(d);
        d.setPosition({x, bottom - t});
        w.draw(d);
    }

    for (float y = r.position.y; y < bottom; y += dash + gap)
    {
        d.setSize({t, min(dash, bottom - y)});
        d.setPosition({r.position.x, y});
        w.draw(d);
        d.setPosition({right - t, y});
        w.draw(d);
    }
}

// translucent dark-teal panel + scanlines + orange dashed border
void drawRetroPanel(sf::RenderWindow &w, sf::FloatRect r)
{
    sf::RectangleShape fill(r.size);
    fill.setPosition(r.position);
    fill.setFillColor(sf::Color(15, 45, 45, 200));
    w.draw(fill);

    w.draw(makeScanlines(r, 3.f, sf::Color(0, 0, 0, 70)));
    drawDashedRect(w, r, sf::Color(235, 140, 50));
}
// =============================================================
// NO GAME NO LIFE UI STYLE
// =============================================================

struct CharTheme
{
    sf::Color body, edge, accent, text;
};

namespace Themes
{
    inline const CharTheme sora{{16, 12, 40, 228}, {230, 55, 80}, {252, 208, 72}, {255, 255, 255}};
    inline const CharTheme shiro{{255, 244, 250, 235}, {255, 105, 170}, {255, 255, 255}, {150, 40, 105}};
    inline const CharTheme tet{{30, 10, 50, 228}, {255, 40, 120}, {70, 200, 255}, {255, 255, 255}};
}

inline sf::Color withA(sf::Color c, int a)
{
    c.a = static_cast<std::uint8_t>(a);
    return c;
}

inline void fillRect(sf::RenderWindow &w, float x, float y, float wd, float h, sf::Color c)
{
    sf::RectangleShape r({wd, h});
    r.setPosition({x, y});
    r.setFillColor(c);
    w.draw(r);
}

inline void drawDiamond(sf::RenderWindow &w, sf::Vector2f c, float s, sf::Color fill, sf::Color line)
{
    sf::RectangleShape d({s, s});
    d.setOrigin({s / 2.f, s / 2.f});
    d.setPosition(c);
    d.setRotation(sf::degrees(45.f));
    d.setFillColor(fill);
    d.setOutlineColor(line);
    d.setOutlineThickness(1.f);
    w.draw(d);
}

// scrolling chess-checker strip
inline void drawChecker(sf::RenderWindow &w, float x, float y, float width,
                        float h, sf::Color a, sf::Color b, float time)
{
    const float cell = h;
    float off = fmod(time * 20.f, cell * 2.f);

    for (float cx = -off; cx < width; cx += cell)
    {
        int idx = static_cast<int>(floor((cx + off) / cell));
        float left = max(cx, 0.f);
        float cw = min(cx + cell, width) - left;

        if (cw > 0.f)
            fillRect(w, x + left, y, cw, h, idx % 2 == 0 ? a : b);
    }
}

inline void drawDialogueBox(sf::RenderWindow &w, sf::FloatRect r,
                            const CharTheme &t, float time)
{
    float x = r.position.x, y = r.position.y;
    float wd = r.size.x, h = r.size.y;

    // soft glow
    for (int i = 4; i >= 1; i--)
        fillRect(w, x - i * 3.f, y - i * 3.f, wd + i * 6.f, h + i * 6.f,
                 withA(t.edge, 12));

    fillRect(w, x, y, wd, h, t.body);

    // checker strip on top
    drawChecker(w, x, y, wd, 8.f, t.edge, t.accent, time);

    // side + bottom border
    fillRect(w, x, y, 3.f, h, t.edge);
    fillRect(w, x + wd - 3.f, y, 3.f, h, t.edge);
    fillRect(w, x, y + h - 3.f, wd, 3.f, t.edge);

    // thin inner line
    sf::RectangleShape inner({wd - 20.f, h - 30.f});
    inner.setPosition({x + 10.f, y + 18.f});
    inner.setFillColor(sf::Color::Transparent);
    inner.setOutlineColor(withA(t.accent, 90));
    inner.setOutlineThickness(1.f);
    w.draw(inner);

    // corner diamonds
    for (sf::Vector2f c : {sf::Vector2f(x, y), sf::Vector2f(x + wd, y),
                           sf::Vector2f(x, y + h), sf::Vector2f(x + wd, y + h)})
        drawDiamond(w, c, 10.f, t.accent, t.body);

    // blinking "next" arrow
    if (fmod(time, 1.f) < 0.6f)
    {
        sf::ConvexShape arrow(3);
        arrow.setPoint(0, {x + wd - 40.f, y + h - 28.f});
        arrow.setPoint(1, {x + wd - 24.f, y + h - 28.f});
        arrow.setPoint(2, {x + wd - 32.f, y + h - 16.f});
        arrow.setFillColor(t.accent);
        w.draw(arrow);
    }
}

inline void drawNameTag(sf::RenderWindow &w, sf::Vector2f pos, sf::Vector2f size,
                        const CharTheme &t, const string &label,
                        const sf::Font &font, unsigned charSize)
{
    const float sk = 16.f; // slant

    sf::ConvexShape tag(4);
    tag.setPoint(0, {pos.x + sk, pos.y});
    tag.setPoint(1, {pos.x + size.x, pos.y});
    tag.setPoint(2, {pos.x + size.x - sk, pos.y + size.y});
    tag.setPoint(3, {pos.x, pos.y + size.y});
    tag.setFillColor(t.body);
    tag.setOutlineColor(t.edge);
    tag.setOutlineThickness(3.f);
    w.draw(tag);

    // accent underline
    fillRect(w, pos.x + sk, pos.y + size.y + 5.f, size.x - sk * 2.f, 3.f, t.accent);

    sf::Text txt(font, label, charSize);
    txt.setFillColor(t.text);

    sf::FloatRect b = txt.getLocalBounds();
    txt.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
    txt.setPosition({pos.x + size.x / 2.f, pos.y + size.y / 2.f});
    w.draw(txt);
}

inline void drawNGNLButton(sf::RenderWindow &w, sf::FloatRect r, const string &label,
                           const sf::Font &font, const CharTheme &t,
                           bool hover, float time)
{
    if (hover)
        for (int i = 3; i >= 1; i--)
            fillRect(w, r.position.x - i * 3.f, r.position.y - i * 3.f,
                     r.size.x + i * 6.f, r.size.y + i * 6.f, withA(t.edge, 25));

    fillRect(w, r.position.x, r.position.y, r.size.x, r.size.y,
             hover ? sf::Color(40, 20, 60, 240) : t.body);

    drawChecker(w, r.position.x, r.position.y, r.size.x, 5.f, t.edge, t.accent, time);
    drawChecker(w, r.position.x, r.position.y + r.size.y - 5.f, r.size.x, 5.f,
                t.accent, t.edge, time);

    fillRect(w, r.position.x, r.position.y, 3.f, r.size.y, t.edge);
    fillRect(w, r.position.x + r.size.x - 3.f, r.position.y, 3.f, r.size.y, t.edge);

    sf::Text txt(font, label, 20);
    txt.setFillColor(hover ? t.accent : t.text);

    sf::FloatRect b = txt.getLocalBounds();
    txt.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
    txt.setPosition({r.position.x + r.size.x / 2.f, r.position.y + r.size.y / 2.f});
    w.draw(txt);
}


// =============================================================
// MAIN
// =============================================================

int main()
{
    // =========================================================
    // GAME VARIABLES
    // =========================================================

    bool gameStarted = false;

    bool shiroSelected = false;

    bool soraSelected = false;

    bool soraSelect1 = false;

    bool shiroBattle = false;

    // =========================================================
    // SHIRO BATTLE
    // =========================================================

    bool shiroCasting = false;

    bool shiroAttacked = false;

    bool attackFired = false;

    int castingFrame = 0;

    sf::Clock castingClock;

    int attackBulletCount = 5;

    float attackSpeed = 8.0f;

    float attackSpread = 20.0f;

    float playerX = 447.f;
    float playerY = 490.f;
    float playerSpeed = 300.f;

    int playerHP = 100;

    int shiroHP = 100;

    BattleHud battleHud;

    // =========================================================
    // RHYTHM VARIABLES
    // =========================================================

    vector<RhythmNote> rhythmNotes =
        {
            {0, 800.f, true},
            {1, 700.f, true},
            {2, 600.f, true},
            {3, 500.f, true},
            {0, 900.f, true},
            {2, 1000.f, true}};

    sf::Clock rhythmClock;

    float rhythmNoteSpeed = 300.f;

    int combo = 0;

    int score = 0;
    int playerLane = 1;
    float playerZ = 150.f;

    vector<sf::CircleShape> playerBullets;

    vector<sf::Vector2f> bulletVelocities;

    // =========================================================
    // CONSOLE
    // =========================================================

    int lines = 20;

    cout << "The Only Way Academy\n";
    cout << "--------------------\n";

    for (int i = 0; i < lines; i++)
    {
        cout << "-";
    }

    cout << "\n";

    // =========================================================
    // WINDOW
    // =========================================================

    sf::RenderWindow window(
        sf::VideoMode({1280, 720}),
        "The Only Way Academy");

    window.setFramerateLimit(200);

    RhythmTrack rhythmTrack;
    sf::Clock uiClock;

    // =========================================================
    // BACKGROUND
    // =========================================================

    sf::Texture backgroundTexture;

    if (!backgroundTexture.loadFromFile("fuckemup.png"))
    {
        cout << "FAILED TO LOAD background1\n";
    }

    backgroundTexture.setSmooth(false);

    sf::Sprite background(backgroundTexture);

    if (backgroundTexture.getSize().x > 0 &&
        backgroundTexture.getSize().y > 0)
    {
        background.setScale({1280.f / backgroundTexture.getSize().x,
                             720.f / backgroundTexture.getSize().y});
    }

    // =========================================================
    // FONTS
    // =========================================================

    sf::Font font;

    if (!font.openFromFile("Hyper Oxide.ttf"))
    {
        cout << "FAILED TO LOAD Hyper Oxide.ttf\n";
    }

    sf::Font editorFont;

    if (!editorFont.openFromFile("FiraCode-Vf.ttf"))
    {
        cout << "FAILED TO LOAD FiraCode-Vf.ttf\n";
    }

    ShiroTerminal terminal(editorFont, font);

    // =========================================================
    // SORA EDITOR (lives in soraeditor.h / soraeditor.cpp)
    // =========================================================

    SoraEditor soraEditor(editorFont);
    soraEditor.loadCommentsFrom("anime.cpp");

    bool soraPassed = false; // set once Sora's code builds clean

    // =========================================================
    // OVERWORLD / RPG STATE
    // =========================================================

    Overworld overworld;
    PledgeScreen pledge; // rules + stakes shown before the boss fight
    ResultsScreen results;
    GradeScene grading;
    HtmlShooterScreen htmlShooter;
    BattleStats stats;
    AsciiCity city;
    bool cityActive = false;      // true while walking around the ASCII city
    bool overworldActive = false; // true once you leave the dialogue
    bool prevBattle = false;      // lets us detect "battle just ended"
    bool beatShiro = false;       // story flag
    bool beatIzuna = false;
    bool lastWasBoss = false;
    float resultTimer = 0.f;
    bool resultWon = false;
    string resultText = "";
    bool introBoss = false; // which fight the transition leads to
    float introTimer = 0.f; // battle transition countdown
    const float introLength = 0.8f;

    // resets all battle state and starts a fight
    auto startBattle = [&](bool boss)
    {
        stats = BattleStats{};
        rhythmNotes =
            {
                {0, 800.f, true},
                {1, 700.f, true},
                {2, 600.f, true},
                {3, 500.f, true},
                {0, 900.f, true},
                {2, 1000.f, true}};

        combo = 0;
        shiroHP = boss ? 100 : 40; // grass bugs are weaker than Shiro

        if (playerHP <= 0)
            playerHP = 50;

        playerBullets.clear();
        bulletVelocities.clear();

        shiroCasting = false;
        attackFired = false;

        terminal.blur();
        lastWasBoss = boss;

        rhythmClock.restart();
        shiroBattle = true;
    };

    // =========================================================
    // TITLE
    // =========================================================

    sf::Text title(font);

    title.setString("THE ONLY WAY");
    title.setCharacterSize(52);
    title.setPosition({400.f, 105.f});
    title.setFillColor(sf::Color::Black);

    // =========================================================
    // PLAY BUTTON
    // =========================================================

    sf::RectangleShape playButton;

    playButton.setSize({250.f, 70.f});
    playButton.setPosition({515.f, 330.f});
    playButton.setFillColor(sf::Color(10, 10, 25, 220));
    playButton.setOutlineColor(sf::Color(255, 255, 255, 180));
    playButton.setOutlineThickness(2.f);

    sf::Text playText(font);

    playText.setString("PLAY GAME");
    playText.setCharacterSize(30);
    playText.setFillColor(sf::Color::White);

    sf::FloatRect playBounds = playText.getLocalBounds();

    playText.setOrigin({playBounds.position.x + playBounds.size.x / 2.f,
                        playBounds.position.y + playBounds.size.y / 2.f});

    playText.setPosition({640.f, 365.f});

    // =========================================================
    // SHIRO
    // =========================================================

    sf::Texture shiroTexture;

    if (!shiroTexture.loadFromFile("shiro.png"))
    {
        cout << "FAILED TO LOAD shiro.png\n";
    }

    shiroTexture.setSmooth(false);

    sf::Sprite shiro(shiroTexture);

    shiro.setOrigin({shiro.getLocalBounds().size.x / 2.f,
                     shiro.getLocalBounds().size.y / 2.f});

    // =========================================================
    // SHIRO ATTACKED
    // =========================================================

    sf::Texture shiroAttackedTexture;

    if (!shiroAttackedTexture.loadFromFile("afterAttack.png"))
    {
        cout << "FAILED TO LOAD afterAttack.png\n";
    }
    else
    {
        cout << "LOADED afterAttack.png\n";
    }

    shiroAttackedTexture.setSmooth(false);

    sf::Sprite shiroAttackedSprite(shiroAttackedTexture);

    shiroAttackedSprite.setOrigin({shiroAttackedSprite.getLocalBounds().size.x / 2.f,
                                   shiroAttackedSprite.getLocalBounds().size.y / 2.f});

    shiroAttackedSprite.setPosition({380.f, 300.f});
    shiroAttackedSprite.setScale({1.0f, 1.0f});

    // =========================================================
    // AURA
    // =========================================================

    sf::Texture auraFarm;

    if (!auraFarm.loadFromFile("afterAttack.png"))
    {
        cout << "FAILED TO LOAD auraFarm\n";
    }

    auraFarm.setSmooth(false);

    sf::Sprite shiroUninterested(auraFarm);

    shiroUninterested.setOrigin({shiroUninterested.getLocalBounds().size.x / 2.f,
                                 shiroUninterested.getLocalBounds().size.y / 2.f});

    shiroUninterested.setPosition({600.f, 250.f});
    shiroUninterested.setScale({0.2f, 0.2f});

    // =========================================================
    // SHIRO PISKEL CASTING
    // =========================================================

    vector<sf::Texture> castingFrames0;

    vector<sf::Texture> castingFrames1;

    vector<sf::Texture> castingFrames2;

    vector<sf::Texture> castingFrames3;

    int castingFrameWidth = 0;

    int castingFrameHeight = 0;

    bool castingFramesLoaded = true;

    if (!loadPiskelFrames(
            "simpleAttack.piskel",
            0,
            castingFrames0,
            castingFrameWidth,
            castingFrameHeight))
    {
        cout << "FAILED TO LOAD CASTING FRAMES 0\n";

        castingFramesLoaded = false;
    }

    int unusedWidth = 0;

    int unusedHeight = 0;

    if (!loadPiskelFrames(
            "simpleAttack.piskel",
            1,
            castingFrames1,
            unusedWidth,
            unusedHeight))
    {
        cout << "FAILED TO LOAD CASTING FRAMES 1\n";

        castingFramesLoaded = false;
    }

    if (!loadPiskelFrames(
            "simpleAttack.piskel",
            2,
            castingFrames2,
            unusedWidth,
            unusedHeight))
    {
        cout << "FAILED TO LOAD CASTING FRAMES 2\n";

        castingFramesLoaded = false;
    }

    if (castingFrames0.empty() ||
        castingFrames1.empty())
    {
        cout << "CASTING ANIMATION HAS NO FRAMES\n";

        castingFramesLoaded = false;
    }

    sf::Sprite *shiroCastingLayer0 = nullptr;

    sf::Sprite *shiroCastingLayer1 = nullptr;

    if (castingFramesLoaded)
    {
        shiroCastingLayer0 = new sf::Sprite(castingFrames0[0]);

        shiroCastingLayer1 = new sf::Sprite(castingFrames1[0]);

        shiroCastingLayer0->setOrigin({static_cast<float>(castingFrameWidth) / 2.f,
                                       static_cast<float>(castingFrameHeight) / 2.f});

        shiroCastingLayer1->setOrigin({static_cast<float>(unusedWidth) / 2.f,
                                       static_cast<float>(unusedHeight) / 2.f});

        shiroCastingLayer0->setPosition({365.f, 130.f});

        shiroCastingLayer1->setPosition({365.f, 130.f});

        shiroCastingLayer0->setScale({175.f / static_cast<float>(castingFrameWidth),
                                      180.f / static_cast<float>(castingFrameHeight)});

        shiroCastingLayer1->setScale({195.f / static_cast<float>(unusedWidth),
                                      200.f / static_cast<float>(unusedHeight)});
    }

    // =========================================================
    // NORMAL SHIRO
    // =========================================================

    shiro.setScale({0.23f, 0.4f});

    shiro.setPosition({250.f, 300.f});

    // =========================================================
    // SORA
    // =========================================================

    sf::Texture soraTexture;

    if (!soraTexture.loadFromFile("sora.png"))
    {
        cout << "FAILED TO LOAD sora.png\n";
    }

    soraTexture.setSmooth(false);

    sf::Sprite sora(soraTexture);

    sora.setOrigin({sora.getLocalBounds().size.x / 2.f,
                    sora.getLocalBounds().size.y / 2.f});

    sora.setScale({0.7f, 0.7f});

    sora.setPosition({550.f, 300.f});

    // =========================================================
    // SHIRO FONT
    // =========================================================

    sf::Font shirofont;

    if (!shirofont.openFromFile("Nitro Break.otf"))
    {
        cout << "FAILED TO LOAD Nitro Break.otf\n";
    }

    sf::Text shiroName(shirofont);

    shiroName.setString("Shiro");
    shiroName.setCharacterSize(50);
    shiroName.setPosition({115.f, 200.f});
    shiroName.setFillColor(sf::Color::Black);

    // =========================================================
    // SHIRO CLICK BOX
    // =========================================================

    sf::RectangleShape shiroMouseBox;

    shiroMouseBox.setSize({190.f, 250.f});
    shiroMouseBox.setPosition({55.f, 175.f});
    shiroMouseBox.setFillColor(sf::Color(0, 0, 0, 0));

    // =========================================================
    // DIALOGUE NAME BOX
    // =========================================================

    sf::RectangleShape nameBox;

    nameBox.setSize({150.f, 45.f});
    nameBox.setPosition({80.f, 375.f});
    nameBox.setFillColor(sf::Color(10, 10, 25, 240));
    nameBox.setOutlineColor(sf::Color(255, 255, 255, 180));
    nameBox.setOutlineThickness(2.f);

    // =========================================================
    // SORA FONT
    // =========================================================

    sf::Font sorafont;

    if (!sorafont.openFromFile("Cyber Blast.otf"))
    {
        cout << "FAILED TO LOAD Cyber Blast.otf\n";
    }

    // =========================================================
    // SORA NAME BOX
    // =========================================================

    sf::RectangleShape soraNameBox;

    soraNameBox.setSize({160.f, 38.f});
    soraNameBox.setPosition({45.f, 405.f});
    soraNameBox.setFillColor(sf::Color(10, 10, 25, 240));
    soraNameBox.setOutlineColor(sf::Color(255, 255, 255, 180));
    soraNameBox.setOutlineThickness(2.f);

    sf::Text soraDialogueName(sorafont);

    soraDialogueName.setString("SORA");
    soraDialogueName.setPosition({75.f, 402.f});
    soraDialogueName.setCharacterSize(25);
    soraDialogueName.setFillColor(sf::Color::White);

    // =========================================================
    // DIALOGUE BUBBLE
    // =========================================================

    sf::RectangleShape textBubble;

    textBubble.setSize({740.f, 140.f});
    textBubble.setPosition({30.f, 430.f});
    textBubble.setFillColor(sf::Color(10, 10, 25, 220));
    textBubble.setOutlineColor(sf::Color(255, 255, 255, 180));
    textBubble.setOutlineThickness(2.f);

    // =========================================================
    // SHIRO SELECT NAME
    // =========================================================

    sf::RectangleShape shiroSelectNameBox;

    shiroSelectNameBox.setSize({90.f, 45.f});
    shiroSelectNameBox.setPosition({75.f, 485.f});
    shiroSelectNameBox.setFillColor(sf::Color(10, 10, 25, 230));
    shiroSelectNameBox.setOutlineColor(sf::Color(255, 255, 255, 180));
    shiroSelectNameBox.setOutlineThickness(2.f);

    sf::Text shiroSelectName(shirofont);

    shiroSelectName.setString("SHIRO");
    shiroSelectName.setCharacterSize(25);
    shiroSelectName.setPosition({95.f, 492.f});
    shiroSelectName.setFillColor(sf::Color::White);

    // =========================================================
    // SORA SELECT NAME
    // =========================================================

    sf::RectangleShape soraSelectNameBox;

    soraSelectNameBox.setSize({90.f, 45.f});
    soraSelectNameBox.setPosition({75.f, 485.f});
    soraSelectNameBox.setFillColor(sf::Color(10, 10, 25, 230));
    soraSelectNameBox.setOutlineColor(sf::Color(255, 255, 255, 180));
    soraSelectNameBox.setOutlineThickness(2.f);

    sf::Text soraSelectName(sorafont);

    soraSelectName.setString("SORA");
    soraSelectName.setCharacterSize(25);
    soraSelectName.setPosition({585.f, 492.f});
    soraSelectName.setFillColor(sf::Color::White);

    // =========================================================
    // DIALOGUE NAME
    // =========================================================

    sf::Text dialogueName(shirofont);

    dialogueName.setString("SHIRO");
    dialogueName.setCharacterSize(25);
    dialogueName.setPosition({75.f, 402.f});
    dialogueName.setFillColor(sf::Color::White);

    // =========================================================
    // SORA MOUSE BOX
    // =========================================================

    sf::RectangleShape soraMouseBox;

    soraMouseBox.setSize({220.f, 300.f});
    soraMouseBox.setPosition({510.f, 175.f});
    soraMouseBox.setFillColor(sf::Color(0, 0, 0, 0));

    // =========================================================
    // DIALOGUE TEXT
    // =========================================================

    sf::Text dialogueText(shirofont);

    dialogueText.setString("sup");
    dialogueText.setCharacterSize(28);
    dialogueText.setPosition({45.f, 450.f});
    dialogueText.setFillColor(sf::Color::White);

    sf::Text dialogueTextS1(sorafont);

    dialogueTextS1.setString("What's up, wanna code... first tell me how you coded this without error");
    dialogueTextS1.setCharacterSize(22);
    dialogueTextS1.setPosition({45.f, 455.f});
    dialogueTextS1.setFillColor(sf::Color(255, 255, 255));

    // =========================================================
    // EDITOR BUTTON (opens the SoraEditor)
    // =========================================================

    sf::RectangleShape editorButton;

    editorButton.setPosition({295.f, 287.f});
    editorButton.setSize({400.f, 40.f});
    editorButton.setFillColor(sf::Color::Black);

    sf::Text editorButtonText(font);

    editorButtonText.setString("Cmon I'm not Joking");
    editorButtonText.setCharacterSize(20);
    editorButtonText.setPosition({375.f, 287.f});

    // =========================================================
    // MAIN LOOP
    // =========================================================
    // dark lines; for orange lines use sf::Color(255, 140, 40, 22)
    sf::VertexArray crt = makeScanlines({{0.f, 0.f}, {1280.f, 720.f}},
                                        3.f, sf::Color(0, 0, 0, 45));
    while (window.isOpen())
    {
        // =====================================================
        // RHYTHM DELTA TIME
        // =====================================================

        float dt = rhythmClock.restart().asSeconds();

        terminal.update(
            dt,
            sf::Vector2f(sf::Mouse::getPosition(window)),
            shiroCasting);

        // time slows while the player is typing in the terminal
        const float slowFactor = 0.25f; // 1.f = no slow-mo
        float gameDt = terminal.focused() ? dt * slowFactor : dt;

        rhythmTrack.update(gameDt, playerLane);

        // =====================================================
        // EVENTS
        // =====================================================

        while (auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }

            // =================================================
            // SORA EDITOR (modal: eats every event while open)
            // =================================================

            if (soraEditor.handleEvent(*event))
                continue;

            // =================================================
            // SHIRO TERMINAL (returns true when BUILD pressed)
            // =================================================

            if (shiroBattle && terminal.handleEvent(*event))
            {
                stats.builds.push_back(terminal.applied());
                attackBulletCount = terminal.applied().bullets;
                attackSpeed = terminal.applied().speed;
                attackSpread = terminal.applied().spread;

                if (castingFramesLoaded)
                {
                    shiroCasting = true;
                    shiroAttacked = false;
                    attackFired = false;
                    castingFrame = 0;
                    castingClock.restart();

                    shiroCastingLayer0->setTexture(castingFrames0[0]);
                    shiroCastingLayer1->setTexture(castingFrames1[0]);
                }
                else
                {
                    attackFired = true; // animation missing: fire anyway
                }
            }

            // =================================================
            // PLEDGE SCREEN + OVERWORLD INPUT
            // =================================================
            // The pledge gets the event first. It's an else-if so the
            // same E press that ends Shiro's dialogue (and opens the
            // pledge) can't also accept it.
            if (grading.active())
            {
                grading.handleEvent(*event);
            }
            else if (htmlShooter.active())
            {
                htmlShooter.handleEvent(*event);
            }
            else if (results.active())
            {
                if (results.handleEvent(*event))
                    grading.open(stats, score, scoreLetter(score));
            }
            else if (pledge.active())
            {
                if (pledge.handleEvent(*event) == PledgeScreen::Result::Accepted)
                {
                    introBoss = true;
                    introTimer = introLength;
                }
            }
            else if (cityActive)
            {
                AsciiCity::Request r = city.handleEvent(*event);

                if (r == AsciiCity::Request::Exit)
                {
                    cityActive = false;
                    overworld.returnFromCity();
                }
                else if (r == AsciiCity::Request::Shooter)
                    htmlShooter.open(editorFont);
            }
            else if (overworldActive && !shiroBattle && introTimer <= 0.f)
            {
                if (overworld.handleEvent(*event, beatShiro) ==
                    Overworld::Request::BossFight)
                {
                    pledge.open(
                        "SHIRO",
                        "Lose, and you start over at the entrance.",
                        {"You may only attack by building code in the terminal.",
                         "Notes that reach your lane cost you HP.",
                         "Time slows while you type, but never stops."});
                }
            }

            // =================================================
            // MOUSE
            // =================================================

            if (const auto *mouseBtn =
                    event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mouseBtn->button == sf::Mouse::Button::Left)
                {
                    auto mouse = sf::Mouse::getPosition(window);

                    float mouseX = static_cast<float>(mouse.x);

                    float mouseY = static_cast<float>(mouse.y);

                    // =================================================
                    // EDITOR BUTTON
                    // =================================================

                    if (soraSelected &&
                        soraSelect1 &&
                        !soraEditor.isOpen() &&
                        editorButton.getGlobalBounds().contains({mouseX, mouseY}))
                    {
                        soraEditor.open();
                        continue;
                    }

                    // =================================================
                    // PLAY GAME
                    // =================================================

                    if (!gameStarted)
                    {
                        if (playButton.getGlobalBounds().contains({mouseX, mouseY}))
                        {
                            gameStarted = true;
                        }
                    }

                    // =================================================
                    // CHARACTER SELECT
                    // =================================================

                    else if (
                        !shiroSelected &&
                        !soraSelected)
                    {
                        if (shiroMouseBox.getGlobalBounds().contains({mouseX, mouseY}))
                        {
                            shiroSelected = true;
                        }
                        else if (soraMouseBox.getGlobalBounds().contains({mouseX, mouseY}))
                        {
                            soraSelected = true;
                        }
                    }

                    // =================================================
                    // SHIRO SECOND CLICK
                    // =================================================

                    else if (
                        shiroSelected &&
                        !soraSelected &&
                        !shiroBattle &&
                        !overworldActive)
                    {
                        sf::FloatRect shiroDialogueClickBox(
                            {300.f, 150.f},
                            {200.f, 250.f});

                        if (shiroDialogueClickBox.contains({mouseX, mouseY}))
                        {
                            overworldActive = true; // walk around first
                            overworld.respawn();

                            rhythmClock.restart();
                        }
                    }

                    // =================================================
                    // SORA SECOND CLICK
                    // =================================================

                    else if (
                        soraSelected &&
                        !soraSelect1)
                    {
                        sf::FloatRect soraDialogueClickBox(
                            {0.f, 0.f},
                            {1280.f, 720.f});

                        if (soraDialogueClickBox.contains({mouseX, mouseY}))
                        {
                            soraSelect1 = true;
                        }
                    }
                }
            }

            // =========================================================
            // RHYTHM INPUT
            // =========================================================

            if (shiroBattle && !soraEditor.isOpen() && !terminal.focused())
            {
                if (const auto *keyEvent =
                        event->getIf<sf::Event::KeyPressed>())
                {
                    int lane = -1;

                    if (keyEvent->code == sf::Keyboard::Key::A)
                    {
                        lane = 0;
                    }
                    else if (keyEvent->code == sf::Keyboard::Key::S)
                    {
                        lane = 1;
                    }
                    else if (keyEvent->code == sf::Keyboard::Key::D)
                    {
                        lane = 2;
                    }
                    else if (keyEvent->code == sf::Keyboard::Key::F)
                    {
                        lane = 3;
                    }

                    if (lane != -1)
                    {
                        playerLane = lane;
                        bool hit = false;

                        for (auto &note : rhythmNotes)
                        {
                            if (!note.active || note.lane != lane)
                            {
                                continue;
                            }

                            if (note.z >= 135.f && note.z <= 210.f)
                            {
                                note.z = 850.f + static_cast<float>(rand() % 500);
                                note.lane = rand() % 4;

                                combo++;
                                stats.notesHit++;
                                stats.longestCombo = max(stats.longestCombo, combo);
                                score += 100 * combo;

                                hit = true;
                                rhythmTrack.flash(lane);

                                break;
                            }
                        }

                        if (!hit)
                        {
                        }
                    }
                }
            }
        }

        // =========================================================
        // SORA EDITOR: react to a finished build
        // =========================================================

        if (soraEditor.takeBuildEvent() && soraEditor.lastBuildOk())
            soraPassed = true;

        // =========================================================
        // RHYTHM UPDATE
        // =========================================================

        if (shiroBattle &&
            shiroHP > 0 &&
            playerHP > 0)
        {
            for (auto &note : rhythmNotes)
            {
                if (!note.active)
                    continue;

                note.z -= rhythmNoteSpeed * gameDt;
                // player sits at about z = 150 / 1.17 = 128 on the track
                if (note.z < 128.f)
                {
                    if (note.lane == playerLane)
                    {
                        // the note hit you
                        rhythmTrack.miss(); // red screen flash
                        combo = 0;
                        playerHP -= 8;
                        stats.hitsTaken++;

                        if (playerHP <= 0)
                        {
                            playerHP = 0;
                            shiroBattle = false;
                        }
                    }

                    // either way, recycle the note (dodged or hit)
                    note.z = 850.f + static_cast<float>(rand() % 500);
                    note.lane = rand() % 4;
                }
            }
        }

        // =========================================================
        // SHIRO CASTING ANIMATION
        // =========================================================

        if (shiroBattle &&
            shiroCasting &&
            castingFramesLoaded)
        {
            if (castingClock.getElapsedTime().asSeconds() >= 1.0f / 12.0f)
            {
                castingClock.restart();

                castingFrame++;

                if (castingFrame >= static_cast<int>(castingFrames0.size()))
                {
                    castingFrame =
                        static_cast<int>(castingFrames0.size()) - 1;

                    shiroCasting = false;

                    attackFired = true;
                }
                else
                {
                    shiroCastingLayer0->setTexture(castingFrames0[castingFrame]);

                    shiroCastingLayer1->setTexture(castingFrames1[castingFrame]);
                }
            }
        }

        // =========================================================
        // SHIRO ATTACK
        // =========================================================

        if (shiroBattle &&
            attackFired)
        {
            playerBullets.clear();

            bulletVelocities.clear();

            float startAngle = -attackSpread / 2.0f;

            float angleStep =
                (attackBulletCount > 1)
                    ? (attackSpread / (attackBulletCount - 1))
                    : 0.0f;

            if (attackSpread >= 360.0f &&
                attackBulletCount > 0)
            {
                angleStep = 360.0f / attackBulletCount;

                startAngle = 0.0f;
            }

            for (int i = 0; i < attackBulletCount; i++)
            {
                sf::CircleShape bullet(5.f);

                bullet.setOrigin({5.f, 5.f});

                bullet.setPosition(rhythmTrack.playerScreenPos());

                bullet.setFillColor(sf::Color(0, 229, 255));

                bullet.setOutlineColor(sf::Color::White);

                bullet.setOutlineThickness(1.f);

                playerBullets.push_back(bullet);

                float currentAngleDeg = startAngle + (i * angleStep);

                sf::Vector2f from = rhythmTrack.playerScreenPos();
                const sf::Vector2f shiroAim{372.f, 168.f};

                float aimDeg = atan2(shiroAim.y - from.y,
                                     shiroAim.x - from.x) *
                               180.f / 3.14159265f;

                float rad = (aimDeg + currentAngleDeg) * 3.14159265f / 180.0f;

                float vx = cos(rad) * attackSpeed;

                float vy = sin(rad) * attackSpeed;

                bulletVelocities.push_back({vx, vy});
            }

            attackFired = false;
        }
        if (shiroBattle)
        {
            const sf::Vector2f shiroPos{372.f, 168.f}; // portal position on screen
            const float shiroRadius = 45.f;

            for (size_t i = 0; i < playerBullets.size();)
            {
                // frame-rate independent: 8 speed = 480 px/sec
                playerBullets[i].move(bulletVelocities[i] * gameDt * 60.f);

                sf::Vector2f p = playerBullets[i].getPosition();
                float dx = p.x - shiroPos.x;
                float dy = p.y - shiroPos.y;
                float reach = shiroRadius + 5.f;

                bool hitShiro = dx * dx + dy * dy < reach * reach;
                bool offscreen = p.x < 0.f || p.x > 720.f ||
                                 p.y < 60.f || p.y > 680.f;

                if (hitShiro)
                {
                    shiroHP -= 3 + combo / 10; // combo makes your code hit harder
                    score += 50;
                }

                if (hitShiro || offscreen)
                {
                    playerBullets.erase(playerBullets.begin() + i);
                    bulletVelocities.erase(bulletVelocities.begin() + i);
                    continue; // don't advance i, the next bullet slid into this slot
                }

                i++;
            }

            if (shiroHP <= 0)
            {
                shiroHP = 0;
                shiroBattle = false;
            }
        }

        // =========================================================
        // OVERWORLD UPDATE
        // =========================================================

        pledge.update(dt);
        results.update(dt);
        grading.update(dt);
        htmlShooter.update(dt);
        if (htmlShooter.takeWin())
            beatIzuna = true;
        overworld.setUnlocked(beatShiro);
        if (resultTimer > 0.f)
            resultTimer -= dt;

        if (overworldActive && !cityActive && !shiroBattle && !pledge.active() && !results.active() && !grading.active() && !htmlShooter.active() && resultTimer <= 0.f)
        {
            if (introTimer > 0.f)
            {
                introTimer -= dt;

                if (introTimer <= 0.f)
                {
                    introTimer = 0.f;
                    startBattle(introBoss);
                }
            }
            else
            {
                Overworld::Request req = overworld.update(dt);

                if (req == Overworld::Request::EnterCity)
                {
                    cityActive = true;
                    city.enter();
                }
                else if (req != Overworld::Request::None)
                {
                    introBoss = (req == Overworld::Request::BossFight);
                    introTimer = introLength;
                }
            }
        }

        if (cityActive && !htmlShooter.active())
            city.update(dt);

        // battle just ended -> back to the overworld
        if (prevBattle && !shiroBattle && overworldActive)
        {
            resultWon = (shiroHP <= 0);
            resultTimer = resultWon ? 0.f : 2.5f;

            resultText = resultWon ? (lastWasBoss ? "Oh Wow... BLank won't ever lose to you though..." : "BUG SQUASHED")
                                   : "YOU FELL OFF THE EDGE PUNK!!!!!";
            if (resultWon)
                results.open(stats, score, resultText);
            if (shiroHP <= 0 && lastWasBoss)
                beatShiro = true;

            if (playerHP <= 0)
            {
                playerHP = 50;
                overworld.respawn();
            }
            else
            {
                playerHP = min(100, playerHP + 25); // small heal after a fight
            }
        }

        prevBattle = shiroBattle;

        // =========================================================
        // RENDER
        // =========================================================

        window.clear();
        float ui = uiClock.getElapsedTime().asSeconds();
        // =========================================================
        // TITLE
        // =========================================================

        if (!gameStarted)
        {
            window.draw(background);

            window.draw(title);

            bool hovPlay = playButton.getGlobalBounds().contains(
                sf::Vector2f(sf::Mouse::getPosition(window)));

            drawNGNLButton(window, playButton.getGlobalBounds(),
                           "PLAY GAME", font, Themes::shiro, hovPlay, ui);
        }

        // =========================================================
        // MAIN GAME
        // =========================================================

        else
        {
            // =====================================================
            // SHIRO BATTLE
            // =====================================================

            if (shiroBattle)
            {
                sf::Color neonCyan(0, 229, 255);

                sf::Color neonPink(255, 0, 127);

                auto rect =
                    [&](sf::Vector2f pos,
                        sf::Vector2f size,
                        sf::Color fill,
                        sf::Color outline = sf::Color::Transparent,
                        float thick = 0.f)
                {
                    sf::RectangleShape r(size);

                    r.setPosition(pos);

                    r.setFillColor(fill);

                    if (thick > 0.f)
                    {
                        r.setOutlineColor(outline);

                        r.setOutlineThickness(thick);
                    }

                    window.draw(r);
                };

                auto txt =
                    [&](const string &s,
                        sf::Vector2f pos,
                        unsigned size,
                        sf::Color c,
                        bool mono = false)
                {
                    sf::Text t(mono ? editorFont : font, s, size);

                    t.setPosition(pos);

                    t.setFillColor(c);

                    window.draw(t);
                };

                auto line =
                    [&](sf::Vector2f a,
                        sf::Vector2f b,
                        sf::Color c)
                {
                    sf::Vertex v[] =
                        {
                            sf::Vertex(a, c),

                            sf::Vertex(b, c)};

                    window.draw(v, 2, sf::PrimitiveType::Lines);
                };

                // =================================================
                // BACKGROUND
                // =================================================

                rect({0.f, 0.f}, {1280.f, 720.f}, sf::Color(1, 3, 8));

                for (int y = 0; y < 720; y += 4)
                {
                    line({0.f, static_cast<float>(y)},

                         {1280.f, static_cast<float>(y)},

                         sf::Color(0, 0, 0, 28));
                }

                // =================================================
                // HEADER
                // =================================================

                rect({0.f, 0.f}, {1280.f, 54.f}, sf::Color(2, 6, 14, 245));

                line({0.f, 53.f}, {1280.f, 53.f}, sf::Color(0, 225, 255, 150));

                txt("SHIRO // BATTLE.OS", {24.f, 10.f}, 20, sf::Color(255, 0, 0));

                txt("VISUAL LANGUAGE = CODE", {315.f, 16.f}, 11, neonCyan, true);

                txt("SECTOR 07", {1040.f, 10.f}, 11, sf::Color(120, 155, 185), true);

                txt("● LIVE", {1170.f, 10.f}, 11, neonPink, true);

                // =================================================
                // COMBAT WINDOW
                // =================================================

                rect({18.f, 70.f}, {690.f, 475.f}, sf::Color(3, 7, 15, 235));

                txt("COMBAT // SHIRO", {34.f, 82.f}, 12, neonCyan, true);

                // =================================================
                // PLAYER HP
                // =================================================

                txt("P1", {36.f, 112.f}, 11, sf::Color(130, 165, 195), true);

                rect({65.f, 110.f},
                     {245.f, 18.f},
                     sf::Color(3, 7, 14, 240),
                     neonCyan,
                     1.f);

                rect({68.f, 114.f},
                     {239.f * (playerHP / 100.f), 10.f},
                     neonCyan);

                txt(to_string(playerHP) + "%",
                    {278.f, 112.f},
                    10,
                    sf::Color(220, 245, 255),
                    true);

                // =================================================
                // SHIRO HP
                // =================================================

                txt("BOSS", {455.f, 112.f}, 11, sf::Color(180, 120, 220), true);

                rect({500.f, 110.f},
                     {180.f, 18.f},
                     sf::Color(3, 7, 14, 240),
                     neonPink,
                     1.f);

                rect({503.f, 114.f},
                     {174.f * (shiroHP / 100.f), 10.f},
                     neonPink);

                // =================================================
                // RHYTHM TRACK
                // =================================================
                battleHud.playerHP = playerHP;
                battleHud.bossHP = shiroHP;
                battleHud.combo = combo;
                battleHud.score = score;
                battleHud.playerLane = playerLane;
                battleHud.bullets = terminal.pending().bullets;
                battleHud.spread = terminal.pending().spread;
                rhythmTrack.draw(
                    window,
                    rhythmNotes,
                    battleHud,
                    editorFont);

                // =================================================
                // RHYTHM UI
                // =================================================

                txt("A", {82.f, 520.f}, 14, neonCyan, true);

                txt("S", {230.f, 520.f}, 14, neonCyan, true);

                txt("D", {378.f, 520.f}, 14, neonCyan, true);

                txt("F", {526.f, 520.f}, 14, neonCyan, true);

                txt("COMBO", {35.f, 565.f}, 10, sf::Color(100, 135, 160), true);

                txt(to_string(combo), {35.f, 582.f}, 20, neonCyan, true);

                txt("SCORE", {130.f, 565.f}, 10, sf::Color(100, 135, 160), true);

                string rankStr(1, scoreLetter(score));
                txt(rankStr, {130.f, 582.f}, 28, letterColor(rankStr[0]), true);

                // =================================================
                // SHIRO CASTING
                // =================================================

                if (shiroCasting &&
                    castingFramesLoaded)
                {
                    window.draw(*shiroCastingLayer0);

                    window.draw(*shiroCastingLayer1);
                }

                // =================================================
                // PLAYER
                // =================================================

                float playerWidth = 620.f / 4.f;

                float playerX =
                    -620.f / 2.f +
                    playerWidth * playerLane +
                    playerWidth / 2.f;

                sf::Vector2f playerPos = rhythmTrack.playerScreenPos();
                rect({playerPos.x - 18.f, playerPos.y - 44.f},
                     {36.f, 44.f},
                     sf::Color(5, 35, 50),
                     neonCyan,
                     2.f);

                rect({playerPos.x - 11.f, playerPos.y - 57.f},
                     {22.f, 18.f},
                     sf::Color(215, 245, 255),
                     neonCyan,
                     1.f);

                txt("PLAYER", {105.f, 458.f}, 10, neonCyan, true);

                // =================================================
                // BULLETS
                // =================================================

                for (auto &bullet : playerBullets)
                {
                    sf::CircleShape glow(11.f);

                    glow.setOrigin({11.f, 11.f});

                    glow.setPosition(bullet.getPosition());

                    glow.setFillColor(sf::Color(0, 220, 255, 28));

                    window.draw(glow);

                    window.draw(bullet);
                }
                terminal.setRank(scoreLetter(score));
                terminal.draw(window);
            }

            // =====================================================
            // OVERWORLD
            // =====================================================

            else if (overworldActive)
            {
                if (cityActive)
                    city.draw(window, editorFont);
                else
                    overworld.draw(
                        window,
                        editorFont,
                        playerHP,
                        100,
                        beatShiro,
                        introTimer > 0.f
                            ? 1.f - introTimer / introLength
                            : -1.f);

                // drawn last so it sits on top of the map
                pledge.draw(window, editorFont);
                results.draw(window, font);
                grading.draw(window, font, editorFont);
                htmlShooter.draw(window, font, editorFont);
                if (resultTimer > 0.f)
                {
                    window.setView(window.getDefaultView());

                    sf::RectangleShape veil({1280.f, 720.f});
                    veil.setFillColor(sf::Color(0, 0, 0, 150));
                    window.draw(veil);

                    sf::Text banner(font, resultText, 56);
                    banner.setFillColor(resultWon ? sf::Color(240, 200, 110)
                                                  : sf::Color(255, 90, 110));
                    sf::FloatRect b = banner.getLocalBounds();
                    banner.setOrigin({b.position.x + b.size.x / 2.f,
                                      b.position.y + b.size.y / 2.f});
                    float maxW = 1180.f;
                    if (b.size.x > maxW)
                        banner.setScale({maxW / b.size.x, maxW / b.size.x});
                    banner.setPosition({640.f, 360.f});
                    window.draw(banner);
                }
            }

            // =====================================================
            // NORMAL GAME
            // =====================================================

            else
            {
                window.draw(background);

                // =================================================
                // CHARACTER SELECT
                // =================================================

                if (!shiroSelected &&
                    !soraSelected)
                {
                    shiro.setScale({0.2f, 0.3f});

                    shiro.setPosition({250.f, 390.f});

                    sora.setScale({0.25f, 0.3f});

                    sora.setPosition({1050.f, 400.f});

                    soraMouseBox.setSize({700.f, 1000.f});

                    soraMouseBox.setPosition({920.f, 200.f});

                    soraSelectNameBox.setSize({90.f, 45.f});

                    soraSelectNameBox.setPosition({900.f, 480.f});

                    soraSelectName.setPosition({925.f, 485.f});

                    window.draw(shiro);

                    window.draw(sora);

                    window.draw(shiroSelectNameBox);

                    window.draw(shiroSelectName);

                    window.draw(soraSelectNameBox);

                    window.draw(soraSelectName);
                }

                // =================================================
                // SHIRO SELECTED
                // =================================================

                else if (shiroSelected &&
                         !soraSelected)
                {
                    shiro.setScale({0.4f, 0.4f});

                    shiro.setPosition({580.f, 450.f});

                    nameBox.setPosition({45.f, 405.f});

                    nameBox.setScale({0.75f, 0.5f});

                    textBubble.setScale({1.65f, 1.5f});

                    window.draw(shiro);

                    drawRetroPanel(window, textBubble.getGlobalBounds());

                    drawDialogueBox(window, textBubble.getGlobalBounds(), Themes::shiro, ui);
                    drawNameTag(window, {50.f, 392.f}, {210.f, 44.f}, Themes::shiro,
                                "SHIRO", shirofont, 24);
                }

                // =================================================
                // SORA SELECTED 1
                // =================================================

                else if (soraSelected &&
                         !soraSelect1)
                {
                    sora.setScale({0.4f, 0.4f});

                    sora.setPosition({580.f, 450.f});

                    soraNameBox.setPosition({45.f, 405.f});

                    soraNameBox.setScale({0.75f, 0.55f});

                    textBubble.setScale({1.65f, 1.5f});

                    window.draw(sora);

                    drawRetroPanel(window, textBubble.getGlobalBounds());
                    drawDialogueBox(window, textBubble.getGlobalBounds(), Themes::sora, ui);
                    drawNameTag(window, {50.f, 392.f}, {210.f, 44.f}, Themes::sora,
                                "SORA", sorafont, 24);
                }

                // =================================================
                // SORA SELECTED 2
                // =================================================

                else if (soraSelected &&
                         soraSelect1)
                {
                    sora.setScale({0.4f, 0.4f});

                    sora.setPosition({580.f, 450.f});

                    soraNameBox.setPosition({45.f, 405.f});

                    soraNameBox.setScale({0.75f, 0.55f});

                    textBubble.setScale({1.65f, 1.5f});

                    dialogueText.setPosition({70.f, 478.f});
                    dialogueTextS1.setPosition({70.f, 478.f});

                    window.draw(sora);

                    drawRetroPanel(window, textBubble.getGlobalBounds());

                    window.draw(soraNameBox);

                    window.draw(soraDialogueName);
                    if (soraPassed)
                        dialogueTextS1.setString("C. Didn't know you were so technical.");
                    window.draw(dialogueTextS1);
                }

                // =================================================
                // EDITOR BUTTON
                // =================================================

                if (soraSelected &&
                    soraSelect1 &&
                    !soraEditor.isOpen())
                {
                    bool hov = editorButton.getGlobalBounds().contains(
                        sf::Vector2f(sf::Mouse::getPosition(window)));

                    drawNGNLButton(window, editorButton.getGlobalBounds(),
                                   "Cmon I'm not Joking", font, Themes::sora, hov, ui);
                }

                // =================================================
                // CODE EDITOR
                // =================================================

                soraEditor.draw(window);
            }
        }

        window.draw(crt);
        window.display();
    }

    // =========================================================
    // CLEANUP
    // =========================================================

    delete shiroCastingLayer0;

    delete shiroCastingLayer1;

    return 0;
}