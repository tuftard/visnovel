#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

// =============================================================
// ASCII CITY (walkable Web District, ported from the HTML version)
// =============================================================
class AsciiCity
{
public:
    enum class Request
    {
        None,
        Exit,
        Shooter
    };

    AsciiCity();

    void enter();
    Request handleEvent(const sf::Event &e);
    void update(float dt);
    void draw(sf::RenderWindow &w, const sf::Font &font) const;

private:
    static constexpr int W = 104;
    static constexpr int H = 76;
    static constexpr int N = W * H;
    static constexpr float CW = 11.f; // cell size in pixels (tune to your font)
    static constexpr float CH = 20.f;
    static constexpr float BASE = 15.f; // baseline offset inside a cell
    static constexpr unsigned SIZE = 18;

    static inline const sf::Color NEON[6] = {
        {0, 229, 255}, {255, 45, 149}, {255, 210, 63}, {182, 107, 255}, {57, 255, 136}, {255, 122, 61}};

    struct Npc
    {
        int x, y;
        std::string name;
        sf::Color color;
        std::vector<std::string> lines;
        bool fixed = false;
        bool shooter = false;
        float timer = 0.f;
    };

    std::vector<char> tile;
    std::vector<sf::Color> col, win;
    std::vector<unsigned char> kind; // 0 ground, 1 wall, 2 window, 3 sign, 4 door
    std::vector<unsigned char> solidMap;
    std::vector<Npc> npcs;

    float px = 52.5f, py = 34.5f;
    float t = 0.f;
    bool talking = false;
    int talkNpc = 0;
    size_t talkIndex = 0;
    std::string toastText;
    float toastT = 0.f;
    std::uint32_t seed = 2077;

    float rnd();
    bool solidAt(int x, int y) const;
    bool freeTile(int x, int y) const { return !solidAt(x, y); }
    bool hit(float x, float y) const;
    int nearestNpc(float range) const;

    static void glyph(sf::VertexArray &va, const sf::Font &f, char c,
                      float x, float y, sf::Color color);
    static void label(sf::RenderWindow &w, const sf::Font &f, const std::string &s,
                      float x, float y, unsigned size, sf::Color c);

    void generate();
};