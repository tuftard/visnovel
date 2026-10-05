#pragma once

#include <string>
#include <vector>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

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

    void open(const std::string &opponent, const std::string &wager,
              const std::vector<std::string> &rules);
    void update(float dt);
    Result handleEvent(const sf::Event &e);
    void draw(sf::RenderWindow &w, const sf::Font &font) const;

private:
    bool active_ = false;
    float t_ = 0.f;
    std::string opponent_;
    std::string wager_;
    std::vector<std::string> rules_;

    float revealEnd() const { return 0.5f + (rules_.size() + 1) * 0.6f; }
};