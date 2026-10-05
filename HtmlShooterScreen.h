#pragma once

#include <string>
#include <vector>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

// =============================================================
// HTML WORLD SHOOTER  (Izuna boss, attacks written as HTML tags)
// =============================================================

class HtmlShooterScreen
{
public:
    bool active() const { return active_; }

    // true once, after the player closes the screen having won
    bool takeWin();

    void open(const sf::Font &mono);
    void update(float dt);
    void handleEvent(const sf::Event &e);
    void draw(sf::RenderWindow &w, const sf::Font &display, const sf::Font &mono) const;

private:
    struct Params
    {
        float shipSpeed = 6.f;
        int count = 3;
        float spread = 24.f;
        float shotSpeed = 10.f;
        float rateMs = 250.f;
    };

    struct Diag
    {
        int line;
        std::string msg;
        bool error;
    };

    struct Bullet
    {
        sf::Vector2f pos;
        sf::Vector2f vel;
    };

    static constexpr float fx = 20.f, fy = 70.f, fw = 690.f, fh = 620.f;
    static constexpr int bossMax = 150;
    static constexpr int maxCols = 52;
    static constexpr int maxRows = 8;
    static constexpr float lineH = 20.f;
    static constexpr float textLeft = 796.f;
    static constexpr float textTop = 126.f;

    sf::FloatRect codeRect{{746.f, 118.f}, {500.f, 180.f}};
    sf::FloatRect buildRect{{746.f, 345.f}, {500.f, 60.f}};

    bool active_ = false;
    bool ended_ = false;
    int outcome_ = 0; // 0 none, 1 won, 2 lost/quit

    float t_ = 0.f, blink = 0.f, buildGlow = 0.f, denyFlash = 0.f;
    float cw = 8.4f;
    sf::Vector2f mouse{0.f, 0.f};

    sf::Vector2f playerPos, bossPos;
    int playerHP = 100, bossHP = bossMax;
    float invuln = 0.f, hurt = 0.f, fireTimer = 0.f;
    float ringTimer = 1.f, aimTimer = 1.f, ringSpin = 0.f;
    std::vector<Bullet> mine, theirs;

    std::vector<std::string> lines;
    int row = 0, col = 0;
    bool focused = false;
    bool hasError = false;
    Params pending, applied;
    std::vector<Diag> diags;

    int len(int r) const { return static_cast<int>(lines[r].size()); }
    static std::string trim(const std::string &s);

    void build();
    void addDiag(int line, const std::string &msg, bool error);
    void setClamped(float &target, float v, float lo, float hi, int line, const std::string &key);
    void parse();

    static void box(sf::RenderWindow &w, float x, float y, float wd, float h,
                    sf::Color fill, sf::Color outline = sf::Color::Transparent,
                    float thick = 0.f);

    // align: 0 left, 1 centre, 2 right
    static void put(sf::RenderWindow &w, const sf::Font &f, const std::string &s,
                    float x, float y, unsigned size, sf::Color c, int align = 0);

    void drawLine(sf::RenderWindow &w, const sf::Font &f, const std::string &s,
                  float x, float y) const;
};