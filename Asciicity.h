#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <utility>
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
        Shooter,
        AimGame
    };

    AsciiCity();

    void enter();
    Request handleEvent(const sf::Event &e);
    void update(float dt);
    void draw(sf::RenderWindow &w, const sf::Font &font) const;

    // map name of the machine NPC you just finished talking to ("" = default)
    const std::string &requestedMap() const { return pendingMap; }

private:
    static constexpr int W = 104;
    static constexpr int H = 76;
    static constexpr int N = W * H;
    static constexpr float CW = 11.f; // cell size in pixels (tune to your font)
    static constexpr float CH = 20.f;
    static constexpr float BASE = 15.f; // baseline offset inside a cell
    static constexpr unsigned SIZE = 18;

    // where you appear when you walk in (next to Izuna, on the plaza)
    static constexpr float SPAWN_X = 52.5f;
    static constexpr float SPAWN_Y = 34.5f;
    static constexpr float PLAYER_R = 0.3f; // half-size of the player's hitbox, in tiles

    static inline const sf::Color NEON[6] = {
        {0, 229, 255}, {255, 45, 149}, {255, 210, 63}, {182, 107, 255}, {57, 255, 136}, {255, 122, 61}};
    struct Npc
    {
        int x, y;
        std::string name;
        sf::Color color;
        std::vector<std::string> lines;
        bool fixed = false;
        float timer = 0.f;
        bool shooter = false;
        bool aimGame = false;
        std::string mapName; // map this NPC's minigame uses ("" = default)
    };

    // a building entrance in the city
    struct Door
    {
        int x, y;
        std::string name;
        std::uint32_t seed; // makes each interior layout repeatable
    };

    std::vector<char> tile;
    std::vector<sf::Color> col, win;
    std::vector<unsigned char> kind; // 0 ground, 1 wall, 2 window, 3 sign, 4 door
    std::vector<unsigned char> solidMap;
    std::vector<Npc> npcs;

    // --- interiors ---
    std::vector<Door> doors;
    bool inside = false;
    std::string placeName = "NEO-KOWLOON // WEB DISTRICT";
    sf::Vector2f savedPos{0.f, 0.f};

    // the city, parked while you are indoors
    std::vector<char> cTile;
    std::vector<sf::Color> cCol, cWin;
    std::vector<unsigned char> cKind, cSolid;
    std::vector<Npc> cNpcs;

    float px = SPAWN_X, py = SPAWN_Y;
    float t = 0.f;
    bool talking = false;
    bool onDoor = false; // true while standing on a door tile (fires once per step-on)
    int talkNpc = 0;
    size_t talkIndex = 0;
    std::string toastText;
    float toastT = 0.f;
    std::uint32_t seed = 2077;
    std::string pendingMap;

    float rnd();
    bool solidAt(int x, int y) const;
    bool freeTile(int x, int y) const { return !solidAt(x, y); }
    bool hit(float x, float y) const;
    bool overlapsPlayer(int tx, int ty) const; // would an NPC on this tile touch the player?
    void clearSpawn();                         // move any NPC off the spawn point
    int nearestNpc(float range) const;

    const Door *doorAt(int x, int y) const;
    void enterBuilding(const Door &d);
    void leaveBuilding();
    void generateInterior(const Door &d);

    static void glyph(sf::VertexArray &va, const sf::Font &f, char c,
                      float x, float y, sf::Color color);
    static void label(sf::RenderWindow &w, const sf::Font &f, const std::string &s,
                      float x, float y, unsigned size, sf::Color c);

    void generate();
};