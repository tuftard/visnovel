#pragma once

#include <string>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include "BattleStats.h"

class ResultsScreen
{
public:
    bool active() const { return active_; }

    void open(const BattleStats &s, int score, const std::string &subtitle);
    void update(float dt);
    bool handleEvent(const sf::Event &e);
    void draw(sf::RenderWindow &w, const sf::Font &font) const;

private:
    bool active_ = false;
    float t_ = 0.f;
    BattleStats stats_;
    int score_ = 0;
    int unique_ = 0;
    std::string favourite_ = "none";
    std::string sub_;
};