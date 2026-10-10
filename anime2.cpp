// cd "C:\Users\Noah\Desktop\visual novel"

// g++ anime.cpp -I"C:\SFML\include" -L"C:\SFML\lib" -lsfml-graphics -lsfml-window -lsfml-system -o anime.exe

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
// C++ KEYWORD HELPER
// =============================================================

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
namespace Syntax
{
    inline const sf::Color bg(32, 32, 31);
    inline const sf::Color keyword(190, 116, 227);
    inline const sf::Color orange(230, 164, 92);
    inline const sf::Color green(160, 200, 80);
    inline const sf::Color comment(102, 112, 133);
    inline const sf::Color text(208, 210, 216);
}

bool isBuiltin(const string &t)
{
    static const vector<string> b = {
        "cout", "cin", "endl", "std", "vector", "printf", "scanf", "main"};

    for (const auto &x : b)
        if (t == x)
            return true;

    return false;
}
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
// RHYTHM GAME
// =============================================================

namespace Theme
{
    inline const sf::Color bgDeep(10, 10, 26);
    inline const sf::Color bgPanel(16, 16, 40);
    inline const sf::Color steel(74, 90, 130);
    inline const sf::Color steelLight(130, 150, 200);
    inline const sf::Color gold(240, 200, 110);
    inline const sf::Color amber(200, 140, 80);
    inline const sf::Color violet(190, 110, 235);
    inline const sf::Color teal(90, 235, 235);
    inline const sf::Color rust(160, 90, 80);
    inline const sf::Color text(220, 225, 245);
    inline const sf::Color textDim(110, 120, 160);
}

struct RhythmNote
{
    int lane;
    float z;
    bool active = true;
};
struct EnemyBullet
{
    sf::Vector2f pos;
    sf::Vector2f vel;
    float radius = 6.f;
    bool active = true;
};

vector<EnemyBullet> enemyBullets;

float playerHitRadius = 5.f; // small hitbox, bullet-hell style
float hitCooldown = 0.f;     // invincibility frames after a hit
float enemySpawnTimer = 1.f;

struct BattleHud
{
    int playerHP = 100;
    int bossHP = 100;
    int combo = 0;
    int score = 0;
    int playerLane = 1;
    int bullets = 5;
    float spread = 20.f;
};

class RhythmTrack
{
public:
    static constexpr unsigned CW = 233; // low-res canvas
    static constexpr unsigned CH = 207;
    static constexpr float PX = 3.f; // upscale -> 699 x 621 on screen

    sf::Vector2f origin{18.f, 60.f}; // top-left of the track panel on screen

    RhythmTrack() : canvas(sf::Vector2u{CW, CH}) {}

    void update(float dt, int playerLane)
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

    void flash(int lane)
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

    void miss() { hurtFlash = 1.f; }

    sf::Vector2f playerScreenPos() const
    {
        return origin + pS(laneCenter(playerLaneF), 1.17f) * PX;
    }

    void draw(sf::RenderWindow &window,
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

private:
    // ----------------------------------------------------------------
    // data
    // ----------------------------------------------------------------
    sf::RenderTexture canvas;
    float time = 0.f;
    float laneGlow[4] = {0.f, 0.f, 0.f, 0.f};
    float playerLaneF = 1.f;
    float hurtFlash = 0.f;

    struct Spark
    {
        sf::Vector2f pos;
        sf::Vector2f vel;
        float life = 0.f;
        float maxLife = 1.f;
        sf::Color color;
    };

    vector<Spark> sparks;

    static constexpr float fW = 233.f;
    static constexpr float fH = 207.f;

    static constexpr float zHit = 150.f;
    static constexpr float zFar = 850.f;

    static constexpr float vanishX = 118.f; // portal
    static constexpr float vanishY = 36.f;
    static constexpr float hitY = 159.f;   // hit line
    static constexpr float halfHit = 92.f; // half track width at hit line

    // lane palette: teal | violet | rust | gold
    static inline const sf::Color laneBright[4] = {
        sf::Color(100, 240, 240), sf::Color(195, 115, 240),
        sf::Color(240, 125, 95), sf::Color(250, 210, 115)};

    static inline const sf::Color laneMid[4] = {
        sf::Color(30, 140, 150), sf::Color(110, 60, 170),
        sf::Color(160, 70, 60), sf::Color(200, 150, 70)};

    static inline const sf::Color laneDark[4] = {
        sf::Color(8, 46, 56), sf::Color(40, 24, 78),
        sf::Color(70, 28, 40), sf::Color(96, 64, 36)};

    // ----------------------------------------------------------------
    // helpers
    // ----------------------------------------------------------------
    static sf::Color col(sf::Color c, float a)
    {
        c.a = static_cast<std::uint8_t>(clamp(a, 0.f, 255.f));
        return c;
    }

    static sf::Color mix(sf::Color a, sf::Color b, float t)
    {
        auto m = [&](std::uint8_t x, std::uint8_t y)
        {
            return static_cast<std::uint8_t>(x + (y - x) * t);
        };
        return sf::Color(m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a));
    }

    static float laneCenter(float lane) { return -0.75f + 0.5f * lane; }

    // lanePos in [-1, 1], s = perspective scale (1 = hit line)
    static sf::Vector2f pS(float lanePos, float s)
    {
        return {vanishX + lanePos * halfHit * s,
                vanishY + (hitY - vanishY) * s};
    }

    static sf::Vector2f pZ(float lanePos, float z)
    {
        return pS(lanePos, zHit / max(z, 20.f));
    }

    void rect(float x, float y, float w, float h, sf::Color fill,
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

        canvas.draw(r);
    }

    void quad(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d,
              sf::Color ca, sf::Color cb, sf::Color cc, sf::Color cd)
    {
        sf::Vertex v[6] = {
            sf::Vertex{a, ca}, sf::Vertex{b, cb}, sf::Vertex{c, cc},
            sf::Vertex{a, ca}, sf::Vertex{c, cc}, sf::Vertex{d, cd}};

        canvas.draw(v, 6, sf::PrimitiveType::Triangles);
    }

    void quad(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d,
              sf::Color all)
    {
        quad(a, b, c, d, all, all, all, all);
    }

    void tri(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Color color)
    {
        sf::Vertex v[3] = {sf::Vertex{a, color}, sf::Vertex{b, color},
                           sf::Vertex{c, color}};
        canvas.draw(v, 3, sf::PrimitiveType::Triangles);
    }

    void line(sf::Vector2f a, sf::Vector2f b, sf::Color c)
    {
        const sf::Vector2f h(0.5f, 0.5f);
        sf::Vertex v[2] = {sf::Vertex{a + h, c}, sf::Vertex{b + h, c}};
        canvas.draw(v, 2, sf::PrimitiveType::Lines);
    }

    void circle(sf::Vector2f c, float r, sf::Color fill,
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

        canvas.draw(s);
    }

    void spiral(sf::Vector2f c, float rMin, float rMax, int arms, float twist,
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

    void frame(float x, float y, float w, float h, sf::Color fill,
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

    void bar(float x, float y, float w, float h, float frac, sf::Color fill,
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

    void magicCircle(sf::Vector2f c, float r, sf::Color a, sf::Color b,
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
        circle(c, 1.5f, sf::Color(255, 255, 255), sf::Color::Transparent, 0.f,
               6);
    }

    // ----------------------------------------------------------------
    // scene
    // ----------------------------------------------------------------
    void drawBackdrop()
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

    void drawPortal()
    {
        sf::Vector2f p{vanishX, vanishY};
        float pulse = 0.5f + 0.5f * std::sin(time * 3.f);

        circle(p, 17.f + pulse * 2.f, col(Theme::violet, 26.f),
               sf::Color::Transparent, 0.f, 32);
        circle(p, 13.f, col(Theme::violet, 50.f), sf::Color::Transparent, 0.f,
               32);
        circle(p, 10.5f, sf::Color(6, 4, 16), sf::Color(200, 120, 235), 2.f,
               32);
        circle(p, 8.f, sf::Color::Transparent, sf::Color(110, 60, 160, 200),
               1.f, 32);

        for (int k = 0; k < 6; k++)
        {
            float ang = time * 1.2f + k * 1.047f;
            rect(p.x + std::cos(ang) * 12.f, p.y + std::sin(ang) * 12.f, 1.f,
                 1.f, sf::Color(255, 220, 255));
        }
    }

    void drawTrack()
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

    void drawNotes(const vector<RhythmNote> &notes)
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

    void drawHitZone()
    {
        const float s0 = 0.93f;
        const float s1 = 1.07f;

        sf::Color base(12, 10, 30, 240);

        quad(pS(-1.04f, s1), pS(1.04f, s1), pS(1.04f, s0), pS(-1.04f, s0),
             base);

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

    void drawCrackle()
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

    void drawSparks()
    {
        for (const auto &s : sparks)
        {
            float k = s.life / s.maxLife;
            float size = k > 0.5f ? 2.f : 1.f;

            rect(s.pos.x, s.pos.y, size, size, col(s.color, 255.f * k));
        }
    }

    void drawPlayer()
    {
        sf::Vector2f p = pS(laneCenter(playerLaneF), 1.17f);
        int lane = clamp(static_cast<int>(std::lround(playerLaneF)), 0, 3);

        spiral(p, 2.f, 13.f, 3, 0.35f, time * 4.f, 0.45f, laneBright[lane]);

        circle(p, 8.f, col(laneBright[lane], 35.f));
        circle(p, 4.5f, laneMid[lane], laneBright[lane], 1.f, 16);
        circle(p, 2.5f, sf::Color(230, 250, 255), sf::Color::Transparent, 0.f,
               12);
    }

    // ----------------------------------------------------------------
    // HUD (drawn in canvas space, matches the reference layout)
    // ----------------------------------------------------------------
    void drawHud(const BattleHud &h)
    {
        // player portrait + HP
        frame(5.f, 4.f, 20.f, 20.f, sf::Color(14, 14, 34), Theme::steelLight);
        circle({15.f, 14.f}, 5.f, sf::Color(30, 120, 140), Theme::teal, 1.f,
               14);
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

        // boss portrait + HP (fills from the right, like the reference)
        float pulse = 0.5f + 0.5f * std::sin(time * 2.5f);

        frame(209.f, 4.f, 19.f, 20.f, sf::Color(14, 10, 28), Theme::rust);
        circle({218.5f, 14.f}, 5.f, sf::Color(8, 4, 18),
               col(Theme::violet, 160.f + 90.f * pulse), 2.f, 16);

        bar(144.f, 8.f, 65.f, 6.f, h.bossHP / 100.f, sf::Color(170, 90, 215),
            sf::Color(230, 170, 255), true, Theme::rust);

        // attack preview panel (shows bullets / spread you typed)
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

        // combo meter (bottom centre)
        bar(62.f, 186.f, 112.f, 9.f, min(h.combo, 50) / 50.f,
            sf::Color(210, 160, 70), sf::Color(255, 225, 140), false,
            Theme::steel);
    }

    void text(sf::RenderWindow &w, const sf::Font &f, const string &s,
              sf::Vector2f cpos, unsigned size, sf::Color c, int align = 0)
    {
        // align: 0 left, 1 centre, 2 right  (vertically centred on cpos.y)
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

    void drawOverlayText(sf::RenderWindow &w, const BattleHud &h,
                         const sf::Font &f)
    {
        text(w, f, "P1  " + to_string(h.playerHP), {28.f, 11.f}, 11,
             Theme::text);
        text(w, f, "SHIRO", {147.f, 11.f}, 11, Theme::text);
        text(w, f, "SCORE " + to_string(h.score), {208.f, 18.5f}, 11,
             Theme::gold, 2);
        text(w, f, "ATK PREVIEW", {27.f, 66.f}, 9, Theme::gold, 1);
        text(w, f, "COMBO x" + to_string(h.combo), {118.f, 190.5f}, 11,
             sf::Color(255, 255, 255), 1);

        const char *keys[4] = {"A", "S", "D", "F"};

        for (int i = 0; i < 4; i++)
            text(w, f, keys[i], {12.f + i * 9.f, 123.f}, 12, laneBright[i], 1);
    }
};

string getClipboardText()
{
    if (!OpenClipboard(nullptr))
        return "";

    HANDLE hData =
        GetClipboardData(CF_UNICODETEXT);

    if (!hData)
    {
        CloseClipboard();
        return "";
    }

    wchar_t *text =
        static_cast<wchar_t *>(
            GlobalLock(hData));

    if (!text)
    {
        CloseClipboard();
        return "";
    }

    int size =
        WideCharToMultiByte(
            CP_UTF8,
            0,
            text,
            -1,
            nullptr,
            0,
            nullptr,
            nullptr);

    string result;

    if (size > 1)
    {
        result.resize(size - 1);

        WideCharToMultiByte(
            CP_UTF8,
            0,
            text,
            -1,
            result.data(),
            size,
            nullptr,
            nullptr);
    }

    GlobalUnlock(hData);
    CloseClipboard();

    return result;
}

// =============================================================
// SHIRO TERMINAL
// =============================================================

struct AttackParams
{
    int bullets = 5;
    float speed = 8.f;
    float spread = 20.f;

    bool operator==(const AttackParams &o) const
    {
        return bullets == o.bullets && speed == o.speed && spread == o.spread;
    }
};

class ShiroTerminal
{
public:
    ShiroTerminal(const sf::Font &monoFont, const sf::Font &displayFont)
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

    bool focused() const { return focused_; }
    void blur() { focused_ = false; }
    const AttackParams &pending() const { return pending_; }
    const AttackParams &applied() const { return applied_; }

    void update(float dt, sf::Vector2f mouse, bool busy)
    {
        blink += dt;
        mouse_ = mouse;
        busy_ = busy;
        buildGlow = max(0.f, buildGlow - dt * 3.f);
        denyFlash = max(0.f, denyFlash - dt * 2.5f);
    }

    // returns true when the player pressed BUILD with valid code
    bool handleEvent(const sf::Event &e)
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

    void draw(sf::RenderWindow &w) const
    {
        const bool changed = !(pending_ == applied_);

        const Diag *firstErr = nullptr;
        const Diag *firstWarn = nullptr;
        for (const auto &d : diags)
        {
            if (d.error && !firstErr)
                firstErr = &d;
            if (!d.error && !firstWarn)
                firstWarn = &d;
        }

        // ---------------- panel + header ----------------
        box(w, 730, 70, 532, 590, sf::Color(2, 6, 13, 248),
            focused_ ? pink : alpha(pink, 110), 2.f);

        text(w, "ROOT // SHIRO_TERMINAL", 748, 84, 12, pink);
        text(w, string("attack.cpp") + (changed ? " *" : ""), 925, 86, 10,
             changed ? gold : dim);
        text(w, focused_ ? "EDITING - TIME SLOWED" : "CLICK CODE TO EDIT",
             1246, 85, 10, focused_ ? gold : dim, 2);
        box(w, 742, 105, 504, 1, alpha(pink, 100));

        // ---------------- code box ----------------
        box(w, 746, 118, 500, 300, sf::Color(1, 4, 10),
            focused_ ? alpha(cyan, 200) : sf::Color(20, 85, 110, 130), 1.f);
        box(w, 746, 118, 42, 300, sf::Color(3, 9, 17));

        if (focused_ && row >= scrollRow && row < scrollRow + visibleRows)
            box(w, 788, textTop + (row - scrollRow) * lineH - 1, 458, lineH,
                alpha(cyan, 16));

        int last = min(static_cast<int>(lines.size()), scrollRow + visibleRows);

        for (int r = scrollRow; r < last; r++)
        {
            float y = textTop + (r - scrollRow) * lineH;
            bool cur = focused_ && r == row;

            text(w, to_string(r + 1), 780, y + 3, 10,
                 cur ? cyan : sf::Color(45, 85, 110), 2);

            for (const auto &d : diags)
            {
                if (d.line != r)
                    continue;

                sf::Color c = d.error ? red : amber;
                box(w, 748, y + 3, 3, 14, c);

                if (d.error)
                    box(w, textLeft, y + lineH - 3,
                        max(1.f, len(r) * cw), 1, alpha(red, 200));
            }

            drawCode(w, lines[r], textLeft, y);
        }

        if (focused_ && row >= scrollRow && row < scrollRow + visibleRows &&
            std::fmod(blink, 1.f) < 0.6f)
        {
            box(w, textLeft + col * cw,
                textTop + (row - scrollRow) * lineH + 1, 2, 16, cyan);
        }

        if (static_cast<int>(lines.size()) > visibleRows)
        {
            float total = static_cast<float>(lines.size());
            float th = 292.f * visibleRows / total;
            float ty = 122.f + 292.f * scrollRow / total;
            box(w, 1240, 122, 3, 292, sf::Color(10, 25, 35));
            box(w, 1240, ty, 3, th, alpha(cyan, 140));
        }

        // ---------------- diagnostics strip ----------------
        sf::Color stripBorder = denyFlash > 0.f ? alpha(red, 255 * denyFlash)
                                                : sf::Color(0, 190, 220, 70);
        box(w, 746, 424, 500, 28, sf::Color(3, 8, 15), stripBorder, 1.f);

        if (firstErr)
            text(w, "ERR   line " + to_string(firstErr->line + 1) + ": " + firstErr->msg,
                 758, 432, 10, red);
        else if (firstWarn)
            text(w, "WARN  line " + to_string(firstWarn->line + 1) + ": " + firstWarn->msg,
                 758, 432, 10, amber);
        else
            text(w, "OK    compiles clean", 758, 432, 10, teal);

        // ---------------- build button ----------------
        bool hover = buildRect.contains(mouse_);
        string label, sub;
        sf::Color fill, border, labelCol, subCol;

        if (busy_)
        {
            label = "CASTING...";
            sub = "SHIRO IS CHARGING THE SPELL";
            fill = sf::Color(30, 22, 8);
            border = amber;
            labelCol = amber;
            subCol = sf::Color(150, 110, 60);
        }
        else if (hasError)
        {
            label = "FIX ERRORS";
            sub = "BUILD DISABLED - SEE LINE " + to_string(firstErr ? firstErr->line + 1 : 0);
            fill = sf::Color(24, 8, 14);
            border = sf::Color(120, 40, 60);
            labelCol = sf::Color(170, 90, 100);
            subCol = sf::Color(110, 60, 70);
        }
        else
        {
            label = "BUILD // EXECUTE";
            sub = to_string(pending_.bullets) + " BULLETS  |  SPREAD " +
                  to_string(static_cast<int>(pending_.spread)) + " DEG  |  CTRL+ENTER";
            fill = hover ? sf::Color(44, 10, 32) : sf::Color(6, 10, 20);
            border = hover ? sf::Color(255, 90, 170) : pink;
            labelCol = sf::Color(245, 245, 255);
            subCol = sf::Color(110, 155, 175);
        }

        box(w, 746, 460, 500, 66, fill, border, 2.f);
        text(w, label, 996, 472, 20, labelCol, 1, true);
        text(w, sub, 996, 504, 9, subCol, 1);

        if (buildGlow > 0.f)
            box(w, 746, 460, 500, 66, alpha(sf::Color::White, buildGlow * 120.f));

        // ---------------- telemetry ----------------
        box(w, 746, 545, 500, 88, sf::Color(3, 8, 15, 230), sf::Color(0, 190, 220, 80), 1.f);
        text(w, "TELEMETRY", 760, 553, 10, cyan);
        text(w, "BAR = BUILT   PINK TICK = TYPED, NOT BUILT", 1236, 555, 8, dim, 2);

        auto stat = [&](float x, const string &name, const string &value,
                        const string &pendValue, bool differs,
                        float fa, float fp, sf::Color c)
        {
            text(w, name, x, 574, 9, dim);
            text(w, value, x, 586, 16, c);

            if (differs)
                text(w, "> " + pendValue, x + 72, 592, 10, pink);

            box(w, x, 614, 140, 5, sf::Color(10, 22, 32));
            box(w, x, 614, 140.f * clamp(fa, 0.f, 1.f), 5, alpha(c, 200));

            if (differs)
                box(w, x + 140.f * clamp(fp, 0.f, 1.f) - 1.f, 611, 2, 11, pink);
        };

        stat(760, "BULLETS  1-50",
             to_string(applied_.bullets), to_string(pending_.bullets),
             applied_.bullets != pending_.bullets,
             (applied_.bullets - 1) / 49.f, (pending_.bullets - 1) / 49.f, cyan);

        stat(920, "SPEED  0.1-30",
             fmt(applied_.speed), fmt(pending_.speed),
             applied_.speed != pending_.speed,
             (applied_.speed - 0.1f) / 29.9f, (pending_.speed - 0.1f) / 29.9f, teal);

        stat(1080, "SPREAD  0-360",
             to_string(static_cast<int>(applied_.spread)),
             to_string(static_cast<int>(pending_.spread)),
             applied_.spread != pending_.spread,
             applied_.spread / 360.f, pending_.spread / 360.f, gold);

        text(w, "ESC release   |   CTRL+ENTER / F5 build   |   click code to edit",
             748, 641, 9, dim);
    }

private:
    struct Diag
    {
        int line;
        string msg;
        bool error;
    };

    const sf::Font *mono;
    const sf::Font *display;

    vector<string> lines;
    int row = 0;
    int col = 0;
    int scrollRow = 0;
    float cw = 8.4f;

    float blink = 0.f;
    float buildGlow = 0.f;
    float denyFlash = 0.f;

    bool focused_ = false;
    bool busy_ = false;
    bool hasError = false;

    sf::Vector2f mouse_{0.f, 0.f};
    AttackParams pending_;
    AttackParams applied_;
    vector<Diag> diags;

    sf::FloatRect codeRect{{746.f, 118.f}, {500.f, 300.f}};
    sf::FloatRect buildRect{{746.f, 460.f}, {500.f, 66.f}};

    static constexpr int maxCols = 50;
    static constexpr int maxRows = 40;
    static constexpr int visibleRows = 14;
    static constexpr float lineH = 20.f;
    static constexpr float textLeft = 796.f;
    static constexpr float textTop = 126.f;

    static inline const sf::Color cyan{0, 229, 255};
    static inline const sf::Color pink{255, 0, 127};
    static inline const sf::Color gold{240, 200, 110};
    static inline const sf::Color teal{0, 235, 190};
    static inline const sf::Color amber{240, 170, 70};
    static inline const sf::Color red{255, 90, 110};
    static inline const sf::Color dim{110, 145, 175};

    // ---------------- small helpers ----------------
    static sf::Color alpha(sf::Color c, float a)
    {
        c.a = static_cast<std::uint8_t>(clamp(a, 0.f, 255.f));
        return c;
    }

    static string fmt(float v)
    {
        char b[24];
        snprintf(b, sizeof b, "%.1f", v);
        return b;
    }

    static string trim(const string &s)
    {
        size_t a = s.find_first_not_of(" \t");
        if (a == string::npos)
            return "";
        size_t b = s.find_last_not_of(" \t");
        return s.substr(a, b - a + 1);
    }

    int len(int r) const { return static_cast<int>(lines[r].size()); }

    void box(sf::RenderWindow &w, float x, float y, float wd, float h,
             sf::Color fill, sf::Color outline = sf::Color::Transparent,
             float thick = 0.f) const
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

    // align: 0 left, 1 centre, 2 right
    void text(sf::RenderWindow &w, const string &s, float x, float y,
              unsigned size, sf::Color c, int align = 0,
              bool useDisplay = false) const
    {
        sf::Text t(useDisplay ? *display : *mono, s, size);
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

    void drawCode(sf::RenderWindow &w, const string &s, float x, float y) const
    {
        size_t i = 0;

        while (i < s.size())
        {
            if (s[i] == ' ')
            {
                i++;
                continue;
            }

            size_t start = i;
            sf::Color c(215, 225, 235);

            if (s[i] == '/' && i + 1 < s.size() && s[i + 1] == '/')
            {
                i = s.size();
                c = Syntax::comment;
            }
            else if (isalpha(static_cast<unsigned char>(s[i])) || s[i] == '_')
            {
                while (i < s.size() &&
                       (isalnum(static_cast<unsigned char>(s[i])) || s[i] == '_'))
                    i++;

                string tok = s.substr(start, i - start);

                if (isKeyword(tok))
                    c = Syntax::keyword;
                else if (tok == "bullets" || tok == "speed" || tok == "spread")
                    c = cyan;
            }
            else if (isdigit(static_cast<unsigned char>(s[i])))
            {
                while (i < s.size() &&
                       (isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.'))
                    i++;

                if (i < s.size() && (s[i] == 'f' || s[i] == 'F'))
                    i++;

                c = Syntax::orange;
            }
            else
            {
                i++;
                c = sf::Color(120, 150, 175);
            }

            text(w, s.substr(start, i - start), x + start * cw, y + 1, 14, c);
        }
    }

    // ---------------- editing ----------------
    void edited()
    {
        parse();
        moved();
    }

    void moved()
    {
        blink = 0.f;
        ensureVisible();
    }

    void ensureVisible()
    {
        if (row < scrollRow)
            scrollRow = row;
        if (row >= scrollRow + visibleRows)
            scrollRow = row - visibleRows + 1;
        clampScroll();
    }

    void clampScroll()
    {
        int maxScroll = max(0, static_cast<int>(lines.size()) - visibleRows);
        scrollRow = clamp(scrollRow, 0, maxScroll);
    }

    void insertChar(char c)
    {
        if (len(row) >= maxCols)
            return;

        lines[row].insert(col, 1, c);
        col++;
        edited();
    }

    void newline()
    {
        if (static_cast<int>(lines.size()) >= maxRows)
            return;

        size_t indentPos = lines[row].find_first_not_of(' ');
        int indent = (indentPos == string::npos) ? len(row) : static_cast<int>(indentPos);
        indent = min(indent, col);

        string tail = lines[row].substr(col);
        lines[row].erase(col);
        lines.insert(lines.begin() + row + 1, string(indent, ' ') + tail);

        row++;
        col = indent;
        edited();
    }

    void backspace()
    {
        if (col > 0)
        {
            lines[row].erase(col - 1, 1);
            col--;
        }
        else if (row > 0)
        {
            int prevLen = len(row - 1);
            if (prevLen + len(row) > maxCols)
                return;

            lines[row - 1] += lines[row];
            lines.erase(lines.begin() + row);
            row--;
            col = prevLen;
        }
        edited();
    }

    void del()
    {
        if (col < len(row))
        {
            lines[row].erase(col, 1);
        }
        else if (row + 1 < static_cast<int>(lines.size()))
        {
            if (len(row) + len(row + 1) > maxCols)
                return;

            lines[row] += lines[row + 1];
            lines.erase(lines.begin() + row + 1);
        }
        edited();
    }

    void paste()
    {
        string s = getClipboardText();
        s.erase(remove(s.begin(), s.end(), '\r'), s.end());

        for (char c : s)
        {
            if (c == '\n')
                newline();
            else if (c == '\t')
            {
                for (int i = 0; i < 4; i++)
                    insertChar(' ');
            }
            else if (c >= 32 && c < 127)
                insertChar(c);
        }
    }

    void placeCursor(sf::Vector2f p)
    {
        int r = scrollRow + static_cast<int>((p.y - textTop) / lineH);
        r = clamp(r, 0, static_cast<int>(lines.size()) - 1);

        int c = static_cast<int>(std::lround((p.x - textLeft) / cw));
        c = clamp(c, 0, len(r));

        row = r;
        col = c;
        blink = 0.f;
    }

    bool tryBuild()
    {
        if (busy_)
            return false;

        if (hasError)
        {
            denyFlash = 1.f;
            return false;
        }

        applied_ = pending_;
        focused_ = false;
        buildGlow = 1.f;
        return true;
    }

    // ---------------- parsing ----------------
    void addDiag(int line, const string &msg, bool error)
    {
        diags.push_back({line, msg, error});
        if (error)
            hasError = true;
    }

    void parse()
    {
        diags.clear();
        hasError = false;

        AttackParams p = pending_; // lines with errors keep the last good value

        for (int i = 0; i < static_cast<int>(lines.size()); i++)
        {
            string s = lines[i];

            size_t c = s.find("//");
            if (c != string::npos)
                s.erase(c);

            s = trim(s);
            if (s.empty())
                continue;

            if (s.back() != ';')
            {
                addDiag(i, "missing ';'", true);
                continue;
            }
            s.pop_back();

            size_t eq = s.find('=');
            if (eq == string::npos)
            {
                addDiag(i, "expected 'name = value;'", true);
                continue;
            }

            string left = trim(s.substr(0, eq));
            string right = trim(s.substr(eq + 1));

            size_t sp = left.find_last_of(" \t");
            string name = (sp == string::npos) ? left : left.substr(sp + 1);

            if (!right.empty() && (right.back() == 'f' || right.back() == 'F'))
                right.pop_back();

            char *endp = nullptr;
            float v = strtof(right.c_str(), &endp);

            if (right.empty() || *endp != '\0' || !std::isfinite(v))
            {
                addDiag(i, "'" + right + "' is not a number", true);
                continue;
            }

            if (name == "bullets")
            {
                int b = static_cast<int>(lround(clamp(v, -1000.f, 1000.f)));
                if (b < 1 || b > 50)
                {
                    addDiag(i, "bullets limited to 1-50", false);
                    b = clamp(b, 1, 50);
                }
                p.bullets = b;
            }
            else if (name == "speed")
            {
                if (v < 0.1f || v > 30.f)
                {
                    addDiag(i, "speed limited to 0.1-30", false);
                    v = clamp(v, 0.1f, 30.f);
                }
                p.speed = v;
            }
            else if (name == "spread")
            {
                if (v < 0.f || v > 360.f)
                {
                    addDiag(i, "spread limited to 0-360", false);
                    v = clamp(v, 0.f, 360.f);
                }
                p.spread = v;
            }
            else
            {
                addDiag(i, "unknown variable '" + name + "'", true);
            }
        }

        pending_ = p;
    }
};

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
        BossFight
    };

    Overworld()
    {
        tiles.assign(H, string(W, '.'));

        for (int x = 0; x < W; x++)
            tiles[0][x] = tiles[H - 1][x] = '#';
        for (int y = 0; y < H; y++)
            tiles[y][0] = tiles[y][W - 1] = '#';

        for (int y = 5; y < 11; y++)
            for (int x = 12; x < 21; x++)
                tiles[y][x] = 'g';

        for (int y = 3; y < 8; y++)
            tiles[y][7] = '#';

        Npc shiro;
        shiro.pos = {TILE * 22.5f, TILE * 3.5f};
        shiro.name = "SHIRO";
        shiro.color = sf::Color(150, 235, 215);
        shiro.boss = true;
        shiro.lines = {"You want to get past me?",
                       "Then prove you can write real code."};
        shiro.afterLines = {"...Fine. You win. Go on ahead."};
        npcs.push_back(shiro);

        Npc sora;
        sora.pos = {TILE * 4.5f, TILE * 3.5f};
        sora.name = "SORA";
        sora.color = sf::Color(240, 150, 110);
        sora.lines = {"Tall grass is full of bugs.",
                      "Walk in it if you want to fight."};
        sora.afterLines = sora.lines;
        npcs.push_back(sora);

        respawn();
    }

    void respawn()
    {
        pos = {TILE * 3.f, TILE * 8.f};
        stepAccum = 0.f;
        talking = false;
    }

    // E / Enter / Space: start talking, advance, or finish a conversation
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

        // one axis at a time so you slide along walls
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

        if (moved > 0.f && tileAtPixel(p) == 'g')
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

    // transition: 0..1 while the battle intro plays, negative = none
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
                else if ((x + y) % 2 == 0)
                    c = sf::Color(44, 50, 76);

                box(w, x * TILE, y * TILE, TILE, TILE, c);
            }
        }

        // draw whoever is lower on screen last, so they appear in front
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

        if (!beaten)
            label(w, font, "Find Shiro (top right) to fight the boss", 20.f, 64.f, 12,
                  sf::Color(240, 200, 110));
        else
            label(w, font, "Shiro defeated!", 20.f, 64.f, 12, sf::Color(0, 235, 190));

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

        // battle transition: flickering veil + bars closing in
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
        sf::Vector2f pos;
        string name;
        sf::Color color{200, 120, 220};
        bool boss = false;
        vector<string> lines;
        vector<string> afterLines;
    };

    static constexpr float TILE = 48.f;
    static constexpr int W = 28;
    static constexpr int H = 14;

    vector<string> tiles; // '#' wall   '.' floor   'g' tall grass
    vector<Npc> npcs;

    sf::Vector2f pos{TILE * 3.f, TILE * 8.f};
    float stepAccum = 0.f;
    float walkTime = 0.f;

    bool talking = false;
    int talkNpc = 0;
    size_t talkIndex = 0;
    vector<string> talkLines;

    char tileAt(int tx, int ty) const
    {
        if (tx < 0 || ty < 0 || tx >= W || ty >= H)
            return '#';
        return tiles[ty][tx];
    }

    char tileAtPixel(sf::Vector2f p) const
    {
        return tileAt(static_cast<int>(std::floor(p.x / TILE)),
                      static_cast<int>(std::floor(p.y / TILE)));
    }

    // p = the player's feet
    bool blocked(sf::Vector2f p) const
    {
        const float halfW = 12.f;

        for (float dx : {-halfW, halfW})
            for (float dy : {-8.f, 0.f})
                if (tileAtPixel({p.x + dx, p.y + dy}) == '#')
                    return true;

        for (const auto &n : npcs)
        {
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

// =============================================================
// PLEDGE SCREEN (rules + stakes shown before a boss fight)
// =============================================================

class PledgeScreen
{
public:
    enum class Result
    {
        None,
        Accepted,
        Declined
    };

    bool active() const { return active_; }

    void open(const string &opponent, const string &wager,
              const vector<string> &rules)
    {
        opponent_ = opponent;
        wager_ = wager;
        rules_ = rules;
        t_ = 0.f;
        active_ = true;
    }

    void update(float dt)
    {
        if (active_)
            t_ += dt;
    }

    Result handleEvent(const sf::Event &e)
    {
        if (!active_)
            return Result::None;

        const auto *k = e.getIf<sf::Event::KeyPressed>();
        if (!k)
            return Result::None;

        if (k->code == sf::Keyboard::Key::Escape)
        {
            active_ = false;
            return Result::Declined;
        }

        bool confirm = k->code == sf::Keyboard::Key::E ||
                       k->code == sf::Keyboard::Key::Enter ||
                       k->code == sf::Keyboard::Key::Space;

        // only accept once every rule has appeared
        if (confirm && t_ > revealEnd())
        {
            active_ = false;
            return Result::Accepted;
        }

        return Result::None;
    }

    void draw(sf::RenderWindow &w, const sf::Font &font) const
    {
        if (!active_)
            return;

        w.setView(w.getDefaultView());

        box(w, 0.f, 0.f, 1280.f, 720.f, sf::Color(0, 0, 0, 200));
        box(w, 240.f, 100.f, 800.f, 520.f, sf::Color(10, 10, 25, 245),
            sf::Color(190, 110, 235), 2.f);

        label(w, font, "TERMS OF THE MATCH", 640.f, 130.f, 30,
              sf::Color(240, 200, 110), true);
        label(w, font, "Opponent: " + opponent_, 640.f, 180.f, 20,
              sf::Color(220, 225, 245), true);

        for (int i = 0; i < static_cast<int>(rules_.size()); i++)
        {
            float a = clamp((t_ - (0.5f + i * 0.6f)) / 0.4f, 0.f, 1.f);
            if (a <= 0.f)
                continue;

            label(w, font, to_string(i + 1) + ".  " + rules_[i], 290.f,
                  240.f + i * 50.f, 20,
                  sf::Color(220, 225, 245, static_cast<std::uint8_t>(255 * a)));
        }

        float wa = clamp((t_ - (0.5f + rules_.size() * 0.6f)) / 0.4f, 0.f, 1.f);
        if (wa > 0.f)
            label(w, font, "STAKES:  " + wager_, 290.f, 250.f + rules_.size() * 50.f,
                  20, sf::Color(240, 125, 95, static_cast<std::uint8_t>(255 * wa)));

        if (t_ > revealEnd() && fmod(t_, 1.f) < 0.7f)
            label(w, font, "[E] Accept        [Esc] Walk away", 640.f, 570.f, 18,
                  sf::Color(90, 235, 235), true);
    }

private:
    bool active_ = false;
    float t_ = 0.f;
    string opponent_;
    string wager_;
    vector<string> rules_;

    float revealEnd() const { return 0.5f + (rules_.size() + 1) * 0.6f; }

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
                      float x, float y, unsigned size, sf::Color c,
                      bool centre = false)
    {
        sf::Text t(f, s, size);
        t.setFillColor(c);
        if (centre)
        {
            sf::FloatRect b = t.getLocalBounds();
            t.setOrigin({b.position.x + b.size.x / 2.f, 0.f});
        }
        t.setPosition({x, y});
        w.draw(t);
    }
};
struct BuildResult
{
    bool ok;
    string output;
};

BuildResult compileCode(const string &code)
{
    {
        ofstream out("test.cpp");
        if (!out.is_open())
            return {false, "Could not write test.cpp"};
        out << code;
    }

    ::remove("test.exe"); // so an old exe can't pass for a new build

    FILE *pipe = _popen("g++ test.cpp -o test.exe 2>&1", "r");
    if (!pipe)
        return {false, "Could not start g++ (is it on PATH?)"};

    string output;
    char buf[256];
    while (fgets(buf, sizeof buf, pipe))
        output += buf;

    int rc = _pclose(pipe);

    if (rc != 0)
        return {false, output.empty() ? "Compile failed." : output};

    return {true, output.empty() ? "Build succeeded - 0 errors." : output};
}
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
    //                            body (pale pink)       edge (hot pink)  accent (white)   text (deep rose)
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
size_t lineStartOf(const string &s, size_t pos)
{
    if (pos == 0)
        return 0;

    size_t p = s.rfind('\n', pos - 1);
    return p == string::npos ? 0 : p + 1;
}

size_t lineEndOf(const string &s, size_t pos)
{
    size_t p = s.find('\n', pos);
    return p == string::npos ? s.size() : p;
}

int lineIndexOf(const string &s, size_t pos)
{
    return static_cast<int>(count(s.begin(), s.begin() + min(pos, s.size()), '\n'));
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
        cout << "FAILED TO LOAD fuckemup.png\n";
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
    const float charW = editorFont.getGlyph(U'M', 14, false).advance;

    // =========================================================
    // OVERWORLD / RPG STATE
    // =========================================================

    Overworld overworld;
    PledgeScreen pledge; // rules + stakes shown before the boss fight

    bool overworldActive = false; // true once you leave the dialogue
    bool prevBattle = false;      // lets us detect "battle just ended"
    bool beatShiro = false;       // story flag
    bool lastWasBoss = false;
    bool introBoss = false; // which fight the transition leads to
    float introTimer = 0.f; // battle transition countdown
    const float introLength = 0.8f;

    // resets all battle state and starts a fight
    auto startBattle = [&](bool boss)
    {
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
    // EDITOR
    // =========================================================

    bool editorOpen = false;

    float scrollOffset = 0.f;

    sf::RectangleShape editorBackground;

    editorBackground.setSize({1200.f, 620.f});
    editorBackground.setPosition({30.f, 20.f});
    editorBackground.setFillColor(sf::Color(32, 32, 31, 250));
    editorBackground.setOutlineColor(sf::Color(0, 200, 255, 0));
    editorBackground.setOutlineThickness(3.f);

    sf::RectangleShape editorTitleBar;

    editorTitleBar.setSize({1200.f, 28.f});
    editorTitleBar.setPosition({30.f, 20.f});
    editorTitleBar.setFillColor(sf::Color::Blue);

    sf::Text editorWindowTitle(editorFont);

    editorWindowTitle.setString("Disboard IDE - [anime.cpp]");
    editorWindowTitle.setCharacterSize(13);
    editorWindowTitle.setPosition({30.f, 25.f});
    editorWindowTitle.setFillColor(sf::Color::White);

    sf::RectangleShape closeBtn;

    closeBtn.setSize({16.f, 16.f});
    closeBtn.setPosition({1198.f, 26.f});
    closeBtn.setFillColor(sf::Color(220, 50, 50));

    sf::VertexArray scanlines(sf::PrimitiveType::Lines);

    for (float y = 50.f; y < 640.f; y += 4.f)
    {
        sf::Vertex v1({30.f, y}, sf::Color(0, 255, 255, 12));
        sf::Vertex v2({1230.f, y}, sf::Color(0, 255, 255, 12));

        scanlines.append(v1);
        scanlines.append(v2);
    }

    sf::RectangleShape cursor;

    cursor.setSize({2.f, 16.f});
    cursor.setFillColor(sf::Color(0, 220, 255));

    string code = "";

    size_t cursorPosition = 0;
    sf::Clock cursorClock;
    size_t lastCursor = static_cast<size_t>(-1);

    const int editorMaxColumns = 130;

    // =========================================================
    // LOAD COMMENTS FROM ANIME.CPP
    // =========================================================

    ifstream file("anime.cpp");

    if (file.is_open())
    {
        string line;

        int commentCount = 0;

        while (getline(file, line))
        {
            size_t first = line.find_first_not_of(" \t");

            if (first != string::npos &&
                line[first] == '/' &&
                first + 1 < line.size() &&
                line[first + 1] == '/')
            {
                string output = line.substr(first);

                code += output + "\n";

                commentCount++;

                if (commentCount >= 3)
                {
                    code += "\n";
                }
            }
        }

        file.close();
    }

    cursorPosition = 0;

    // =========================================================
    // EDITOR BUTTON
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
    // BUILD BUTTON
    // =========================================================

    sf::RectangleShape buildButton;

    buildButton.setSize({150.f, 40.f});
    buildButton.setPosition({545.f, 490.f});
    buildButton.setFillColor(sf::Color(0, 110, 180));
    buildButton.setOutlineColor(sf::Color(0, 0, 255));
    buildButton.setOutlineThickness(1.f);

    sf::Text buildButtonText(editorFont);

    buildButtonText.setString("BUILD");
    buildButtonText.setCharacterSize(18);
    buildButtonText.setPosition({590.f, 498.f});
    string buildOutput = "Press BUILD (or F5) to compile.";
    bool buildOk = true;

    auto runBuild = [&]()
    {
        BuildResult r = compileCode(code);
        buildOk = r.ok;
        buildOutput = r.output;

        if (r.ok)
        {
            // your test code uses cin, so give it its own console window
            system("start \"\" cmd /k test.exe");
        }
    };

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
            // SHIRO TERMINAL (returns true when BUILD pressed)
            // =================================================

            if (shiroBattle && terminal.handleEvent(*event))
            {
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

            if (pledge.active())
            {
                if (pledge.handleEvent(*event) == PledgeScreen::Result::Accepted)
                {
                    introBoss = true;
                    introTimer = introLength;
                }
            }
            else if (
                overworldActive &&
                !shiroBattle &&
                introTimer <= 0.f)
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
                    // CLOSE EDITOR
                    // =================================================

                    if (editorOpen &&
                        closeBtn.getGlobalBounds().contains({mouseX, mouseY}))
                    {
                        editorOpen = false;

                        continue;
                    }

                    // =================================================
                    // EDITOR BUTTON
                    // =================================================

                    if (soraSelected &&
                        soraSelect1 &&
                        !editorOpen &&
                        editorButton.getGlobalBounds().contains({mouseX, mouseY}))
                    {
                        editorOpen = true;

                        scrollOffset = 0.f;

                        cursorPosition = 0;

                        continue;
                    }

                    // =================================================
                    // BUILD NORMAL EDITOR
                    // =================================================

                    if (editorOpen &&
                        buildButton.getGlobalBounds().contains({mouseX, mouseY}))
                    {
                        ofstream outFile("test.cpp");

                        if (outFile.is_open())
                        {
                            outFile << code;

                            outFile.close();

                            cout << "CODE SAVED\n";
                        }
                        else
                        {
                            cout << "FAILED TO SAVE\n";
                        }
                    }

                    // =================================================
                    // CLICK EDITOR
                    // =================================================

                    if (editorOpen &&
                        mouseX >= 45.f &&
                        mouseX <= 1210.f &&
                        mouseY >= 55.f &&
                        mouseY <= 480.f)
                    {
                        const float codeLeft = 45.f;

                        const float lineHeight = 17.f;

                        float clickY = mouseY - 55.f + scrollOffset;

                        int targetLine = static_cast<int>(clickY / lineHeight);

                        if (targetLine < 0)
                            targetLine = 0;

                        size_t lineStart = 0;

                        int currentLine = 0;

                        size_t targetPosition = code.size();

                        while (lineStart < code.size())
                        {
                            size_t lineEnd = code.find('\n', lineStart);

                            if (lineEnd == string::npos)
                            {
                                lineEnd = code.size();
                            }

                            if (currentLine == targetLine)
                            {
                                float drawX = codeLeft;

                                targetPosition = lineStart;

                                float minDistance = abs(mouseX - drawX);

                                for (size_t i = lineStart; i < lineEnd; i++)
                                {
                                    sf::Text charText(
                                        editorFont,
                                        string(1, code[i]),
                                        14);

                                    float charWidth =
                                        charText.getLocalBounds().size.x;

                                    float charCenter = drawX + charWidth / 2.f;

                                    float dist = abs(mouseX - charCenter);

                                    if (dist < minDistance)
                                    {
                                        minDistance = dist;

                                        targetPosition =
                                            (mouseX >= charCenter) ? i + 1 : i;
                                    }

                                    drawX += charWidth;
                                }

                                if (mouseX >= drawX)
                                {
                                    targetPosition = lineEnd;
                                }

                                break;
                            }

                            currentLine++;

                            if (lineEnd == code.size())
                            {
                                targetPosition = code.size();

                                break;
                            }

                            lineStart = lineEnd + 1;

                            targetPosition = lineStart;
                        }

                        cursorPosition = targetPosition;
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

            if (shiroBattle && !editorOpen && !terminal.focused())
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

            // =========================================================
            // EDITOR TYPING
            // =========================================================

            if (editorOpen)
            {
                // =====================================================
                // TEXT INPUT
                // =====================================================

                if (const auto *textEvent =
                        event->getIf<sf::Event::TextEntered>())
                {
                    if (textEvent->unicode >= 32 &&
                        textEvent->unicode < 127)
                    {
                        size_t lineStart =
                            code.rfind(
                                '\n',
                                cursorPosition == 0 ? 0 : cursorPosition - 1);

                        if (lineStart == string::npos)
                            lineStart = 0;
                        else
                            lineStart++;

                        size_t column = cursorPosition - lineStart;

                        if (column >= static_cast<size_t>(editorMaxColumns))
                        {
                            code.insert(cursorPosition, 1, '\n');

                            cursorPosition++;
                        }
                        code.insert(
                            cursorPosition,
                            1,
                            static_cast<char>(textEvent->unicode));

                        cursorPosition++;
                    }
                }

                // =====================================================
                // KEYBOARD
                // =====================================================

                if (const auto *keyEvent =
                        event->getIf<sf::Event::KeyPressed>())
                {
                    bool ctrlHeld =
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);

                    // =================================================
                    // CTRL + V
                    // =================================================

                    if (ctrlHeld &&
                        keyEvent->code == sf::Keyboard::Key::V)
                    {
                        string pasted = getClipboardText();

                        if (!pasted.empty())
                        {
                            // Windows clipboard newlines
                            // -> normal LF
                            pasted.erase(
                                remove(pasted.begin(), pasted.end(), '\r'),
                                pasted.end());

                            // Tabs -> 4 spaces
                            size_t tabPos = 0;

                            while ((tabPos = pasted.find('\t', tabPos)) !=
                                   string::npos)
                            {
                                pasted.replace(tabPos, 1, "    ");

                                tabPos += 4;
                            }

                            // Find indentation of current line
                            size_t currentLineStart =
                                code.rfind(
                                    '\n',
                                    cursorPosition == 0 ? 0 : cursorPosition - 1);

                            if (currentLineStart == string::npos)
                            {
                                currentLineStart = 0;
                            }
                            else
                            {
                                currentLineStart++;
                            }

                            size_t indentEnd = currentLineStart;

                            while (indentEnd < code.size() &&
                                   (code[indentEnd] == ' ' ||
                                    code[indentEnd] == '\t'))
                            {
                                indentEnd++;
                            }

                            string currentIndent =
                                code.substr(
                                    currentLineStart,
                                    indentEnd - currentLineStart);

                            // Add current indentation to
                            // subsequent pasted lines
                            string formattedPaste;

                            for (size_t i = 0; i < pasted.size(); i++)
                            {
                                formattedPaste += pasted[i];

                                if (pasted[i] == '\n' &&
                                    i + 1 < pasted.size())
                                {
                                    formattedPaste += currentIndent;
                                }
                            }

                            code.insert(cursorPosition, formattedPaste);

                            cursorPosition += formattedPaste.size();
                        }
                    }

                    // =================================================
                    // TAB
                    // =================================================

                    else if (keyEvent->code == sf::Keyboard::Key::Tab)
                    {
                        code.insert(cursorPosition, "    ");

                        cursorPosition += 4;
                    }

                    // =================================================
                    // BACKSPACE
                    // =================================================

                    else if (
                        keyEvent->code == sf::Keyboard::Key::Backspace &&
                        cursorPosition > 0)
                    {
                        code.erase(cursorPosition - 1, 1);

                        cursorPosition--;
                    }

                    // =================================================
                    // ENTER + AUTO INDENT
                    // =================================================

                    else if (keyEvent->code == sf::Keyboard::Key::Enter)
                    {
                        size_t lineStart =
                            code.rfind(
                                '\n',
                                cursorPosition == 0 ? 0 : cursorPosition - 1);

                        if (lineStart == string::npos)
                        {
                            lineStart = 0;
                        }
                        else
                        {
                            lineStart++;
                        }

                        size_t indentEnd = lineStart;

                        while (indentEnd < code.size() &&
                               code[indentEnd] == ' ')
                        {
                            indentEnd++;
                        }

                        string indent =
                            code.substr(lineStart, indentEnd - lineStart);

                        string newline = "\n" + indent;

                        code.insert(cursorPosition, newline);

                        cursorPosition += newline.size();
                    }

                    // =================================================
                    // LEFT
                    // =================================================

                    else if (keyEvent->code == sf::Keyboard::Key::Left)
                    {
                        if (cursorPosition > 0)
                            cursorPosition--;
                    }

                    // =================================================
                    // RIGHT
                    // =================================================

                    else if (keyEvent->code == sf::Keyboard::Key::Right)
                    {
                        if (cursorPosition < code.size())
                        {
                            cursorPosition++;
                        }
                    }

                    // =================================================
                    // UP
                    // =================================================

                    else if (keyEvent->code == sf::Keyboard::Key::Up)
                    {
                        size_t lineStart =
                            code.rfind(
                                '\n',
                                cursorPosition == 0 ? 0 : cursorPosition - 1);

                        if (lineStart == string::npos)
                        {
                            lineStart = 0;
                        }
                        else
                        {
                            lineStart++;
                        }

                        if (lineStart > 0)
                        {
                            size_t previousLineEnd = lineStart - 1;

                            size_t previousLineStart =
                                code.rfind(
                                    '\n',
                                    previousLineEnd == 0 ? 0 : previousLineEnd - 1);

                            if (previousLineStart == string::npos)
                            {
                                previousLineStart = 0;
                            }
                            else
                            {
                                previousLineStart++;
                            }

                            size_t column = cursorPosition - lineStart;

                            size_t previousLineLength =
                                previousLineEnd - previousLineStart;

                            if (column > previousLineLength)
                            {
                                column = previousLineLength;
                            }

                            cursorPosition = previousLineStart + column;
                        }
                    }

                    // =================================================
                    // DOWN
                    // =================================================

                    else if (keyEvent->code == sf::Keyboard::Key::Down)
                    {
                        size_t lineStart =
                            code.rfind(
                                '\n',
                                cursorPosition == 0 ? 0 : cursorPosition - 1);

                        if (lineStart == string::npos)
                        {
                            lineStart = 0;
                        }
                        else
                        {
                            lineStart++;
                        }

                        size_t column = cursorPosition - lineStart;

                        size_t nextLineStart = code.find('\n', cursorPosition);

                        if (nextLineStart != string::npos)
                        {
                            nextLineStart++;

                            size_t nextLineEnd = code.find('\n', nextLineStart);

                            if (nextLineEnd == string::npos)
                            {
                                nextLineEnd = code.size();
                            }

                            size_t nextLineLength = nextLineEnd - nextLineStart;

                            if (column > nextLineLength)
                            {
                                column = nextLineLength;
                            }

                            cursorPosition = nextLineStart + column;
                        }
                    }
                }

                // =========================================================
                // EDITOR SCROLL
                // =========================================================

                if (const auto *scrollEvent =
                        event->getIf<sf::Event::MouseWheelScrolled>())
                {
                    scrollOffset -= scrollEvent->delta * 55.f;

                    size_t lineCount =
                        count(code.begin(), code.end(), '\n') + 1;

                    float maxScroll =
                        max(0.f,
                            static_cast<float>(lineCount) * 17.f -
                                (480.f - 55.f));

                    if (scrollOffset < 0.f)
                        scrollOffset = 0.f;

                    if (scrollOffset > maxScroll)
                        scrollOffset = maxScroll;
                }
            }
            {

                if (const auto *keyEvent =
                        event->getIf<sf::Event::KeyPressed>())
                {
                    // LEFT
                    if (keyEvent->code == sf::Keyboard::Key::Left)
                    {
                        if (cursorPosition > 0)
                            cursorPosition--;
                    }

                    // RIGHT
                    if (keyEvent->code == sf::Keyboard::Key::Right)
                    {
                        if (cursorPosition < code.size())
                        {
                            cursorPosition++;
                        }
                    }

                    // UP
                    if (keyEvent->code == sf::Keyboard::Key::Up)
                    {
                        size_t lineStart =
                            code.rfind(
                                '\n',
                                cursorPosition == 0 ? 0 : cursorPosition - 1);

                        if (lineStart == string::npos)
                        {
                            lineStart = 0;
                        }
                        else
                        {
                            lineStart++;
                        }

                        if (lineStart > 0)
                        {
                            size_t previousLineEnd = lineStart - 1;

                            size_t previousLineStart =
                                code.rfind(
                                    '\n',
                                    previousLineEnd == 0 ? 0 : previousLineEnd - 1);

                            if (previousLineStart == string::npos)
                            {
                                previousLineStart = 0;
                            }
                            else
                            {
                                previousLineStart++;
                            }

                            size_t column = cursorPosition - lineStart;

                            size_t previousLineLength =
                                previousLineEnd - previousLineStart;

                            if (column > previousLineLength)
                            {
                                column = previousLineLength;
                            }

                            cursorPosition = previousLineStart + column;
                        }
                    }

                    // DOWN
                    if (keyEvent->code == sf::Keyboard::Key::Down)
                    {
                        size_t lineStart =
                            code.rfind(
                                '\n',
                                cursorPosition == 0 ? 0 : cursorPosition - 1);

                        if (lineStart == string::npos)
                        {
                            lineStart = 0;
                        }
                        else
                        {
                            lineStart++;
                        }

                        size_t column = cursorPosition - lineStart;

                        size_t nextLineStart = code.find('\n', cursorPosition);

                        if (nextLineStart != string::npos)
                        {
                            nextLineStart++;

                            size_t nextLineEnd = code.find('\n', nextLineStart);

                            if (nextLineEnd == string::npos)
                            {
                                nextLineEnd = code.size();
                            }

                            size_t nextLineLength = nextLineEnd - nextLineStart;

                            if (column > nextLineLength)
                            {
                                column = nextLineLength;
                            }

                            cursorPosition = nextLineStart + column;
                        }
                    }
                }
                // =========================================================
                // EDITOR SCROLL
                // =========================================================

                if (const auto *scrollEvent =
                        event->getIf<sf::Event::MouseWheelScrolled>())
                {
                    scrollOffset -= scrollEvent->delta * 90.f;

                    if (scrollOffset < 0.f)
                        scrollOffset = 0.f;
                }
            }
        }

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

        if (overworldActive && !shiroBattle && !pledge.active())
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

                if (req != Overworld::Request::None)
                {
                    introBoss = (req == Overworld::Request::BossFight);
                    introTimer = introLength;
                }
            }
        }

        // battle just ended -> back to the overworld
        if (prevBattle && !shiroBattle && overworldActive)
        {
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

                txt(to_string(score), {130.f, 582.f}, 20, neonPink, true);

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

                terminal.draw(window);
            }

            // =====================================================
            // OVERWORLD
            // =====================================================

            else if (overworldActive)
            {
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

                    window.draw(dialogueTextS1);
                }

                // =================================================
                // EDITOR BUTTON
                // =================================================

                if (soraSelected &&
                    soraSelect1 &&
                    !editorOpen)
                {
                    bool hov = editorButton.getGlobalBounds().contains(
                        sf::Vector2f(sf::Mouse::getPosition(window)));

                    drawNGNLButton(window, editorButton.getGlobalBounds(),
                                   "Cmon I'm not Joking", font, Themes::sora, hov, ui);
                }

                // =================================================
                // CODE EDITOR
                // =================================================

                if (editorOpen)
                {
                    window.draw(editorBackground);

                    window.draw(editorTitleBar);

                    window.draw(editorWindowTitle);

                    window.draw(closeBtn);

                    const float codeLeft = 45.f;

                    const float codeRight = 1210.f;

                    const float codeTop = 55.f;

                    const float codeBottom = 480.f;

                    const float lineHeight = 17.f;

                    float drawX = codeLeft;

                    float drawY = codeTop - scrollOffset;

                    // =================================================
                    // FAST TEXT DRAWING
                    // =================================================

                    auto drawRun =
                        [&](const string &textString,
                            sf::Color color)
                    {
                        if (textString.empty())
                            return;

                        sf::Text text(editorFont);

                        text.setCharacterSize(14);

                        text.setString(textString);

                        text.setFillColor(color);

                        float width = text.getLocalBounds().size.x;

                        if (drawX + width > codeRight)
                        {
                            drawX = codeLeft;
                            drawY += lineHeight;
                        }

                        if (drawY >= codeTop &&
                            drawY + lineHeight <= codeBottom)
                        {
                            text.setPosition({drawX, drawY});

                            window.draw(text);
                        }

                        drawX += width;
                    };

                    size_t i = 0;

                    while (i < code.size())
                    {
                        if (drawY >= codeBottom)
                        {
                            break;
                        }

                        // =================================================
                        // NEW LINE
                        // =================================================

                        if (code[i] == '\n')
                        {
                            drawX = codeLeft;

                            drawY += lineHeight;

                            i++;

                            continue;
                        }

                        // =================================================
                        // COMMENT
                        // =================================================

                        if (code[i] == '/' &&
                            i + 1 < code.size() &&
                            code[i + 1] == '/')
                        {
                            size_t end = code.find('\n', i);

                            if (end == string::npos)
                            {
                                end = code.size();
                            }

                            drawRun(
                                code.substr(i, end - i),
                                Syntax::comment);

                            i = end;

                            continue;
                        }

                        // =================================================
                        // STRING
                        // =================================================

                        if (code[i] == '"')
                        {
                            size_t end = code.find('"', i + 1);

                            if (end == string::npos)
                            {
                                end = code.size() - 1;
                            }

                            drawRun(
                                code.substr(i, end - i + 1),
                                Syntax::green);

                            i = end + 1;

                            continue;
                        }

                        // =================================================
                        // IDENTIFIER
                        // =================================================

                        if (isalpha(static_cast<unsigned char>(code[i])) ||
                            code[i] == '_')
                        {
                            size_t start = i;

                            while (i < code.size() &&
                                   (isalnum(static_cast<unsigned char>(code[i])) ||
                                    code[i] == '_'))
                            {
                                i++;
                            }

                            string token = code.substr(start, i - start);

                            sf::Color color =
                                isKeyword(token)   ? Syntax::keyword
                                : isBuiltin(token) ? Syntax::orange
                                                   : Syntax::text;
                            drawRun(token, color);

                            continue;
                        }

                        // =================================================
                        // NUMBER
                        // =================================================

                        if (isdigit(static_cast<unsigned char>(code[i])))
                        {
                            size_t start = i;

                            while (i < code.size() &&
                                   (isdigit(static_cast<unsigned char>(code[i])) ||
                                    code[i] == '.'))
                            {
                                i++;
                            }

                            drawRun(
                                code.substr(start, i - start),
                                Syntax::orange);

                            continue;
                        }

                        // =================================================
                        // NORMAL CHARACTER
                        // =================================================

                        drawRun(
                            string(1, code[i]),
                            Syntax::text);

                        i++;
                    }
                    // =====================================================
                    // CURSOR
                    // =====================================================

                    float cursorX = codeLeft;
                    float cursorY = codeTop - scrollOffset;

                    size_t lineStart =
                        code.rfind(
                            '\n',
                            cursorPosition == 0 ? 0 : cursorPosition - 1);

                    if (lineStart == string::npos)
                    {
                        lineStart = 0;
                    }
                    else
                    {
                        lineStart++;
                    }

                    size_t column = cursorPosition - lineStart;

                    size_t lineNumber = 0;

                    for (size_t i = 0; i < lineStart && i < code.size(); i++)
                    {
                        if (code[i] == '\n')
                            lineNumber++;
                    }

                    cursorX += static_cast<float>(column) * 8.4f;

                    cursorY =
                        codeTop +
                        static_cast<float>(lineNumber) * lineHeight -
                        scrollOffset;

                    if (cursorY >= codeTop &&
                        cursorY < codeBottom)
                    {
                        cursor.setPosition({cursorX, cursorY});

                        window.draw(cursor);
                    }
                }
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