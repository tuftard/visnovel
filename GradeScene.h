#pragma once

#include <string>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include "BattleStats.h"

class GradeScene
{
public:
    bool active() const { return active_; }

    void open(const BattleStats &s, int score, char grade);
    void update(float dt);

    // returns true when the player leaves the scene
    bool handleEvent(const sf::Event &e);

    void draw(sf::RenderWindow &w, const sf::Font &display, const sf::Font &mono) const;

private:
    static constexpr float textStart = 1.2f;
    static constexpr float cps = 40.f;

    bool active_ = false;
    float t_ = 0.f;
    BattleStats stats_;
    int score_ = 0;
    int unique_ = 0;
    char grade_ = 'A';
    std::string comment_;

    size_t shown() const;
};