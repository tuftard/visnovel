
#pragma once

#include "Common.h"

#include <string>
#include <vector>

using std::string;
using std::vector;


class ShiroTerminal
{
public:
    ShiroTerminal(const sf::Font &monoFont,
                  const sf::Font &displayFont);

    bool focused() const;
    void blur();
    void setRank(char c);

    const AttackParams &pending() const;
    const AttackParams &applied() const;

    void update(float dt, sf::Vector2f mouse, bool busy);
    bool handleEvent(const sf::Event &e);
    void draw(sf::RenderWindow &w) const;

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
    char rank_ = 'A';
    bool hasError = false;

    sf::Vector2f mouse_{0.f, 0.f};

    AttackParams pending_;
    AttackParams applied_;

    vector<Diag> diags;

    sf::FloatRect codeRect{
        {746.f, 118.f},
        {500.f, 300.f}};

    sf::FloatRect buildRect{
        {746.f, 460.f},
        {500.f, 66.f}};

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

    static sf::Color alpha(sf::Color c, float a);
    static string fmt(float v);
    static string trim(const string &s);

    int len(int r) const;

    void box(
        sf::RenderWindow &w,
        float x,
        float y,
        float wd,
        float h,
        sf::Color fill,
        sf::Color outline = sf::Color::Transparent,
        float thick = 0.f) const;

    void text(
        sf::RenderWindow &w,
        const string &s,
        float x,
        float y,
        unsigned size,
        sf::Color c,
        int align = 0,
        bool useDisplay = false) const;

    void drawCode(
        sf::RenderWindow &w,
        const string &s,
        float x,
        float y) const;

    void edited();
    void moved();
    void ensureVisible();
    void clampScroll();

    void insertChar(char c);
    void newline();
    void backspace();
    void del();
    void paste();

    void placeCursor(sf::Vector2f p);

    bool tryBuild();

    void addDiag(
        int line,
        const string &msg,
        bool error);

    void parse();
};
