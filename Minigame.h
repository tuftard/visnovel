#pragma once
#include <SFML/Graphics.hpp>

// Anything the main loop can launch from the city (aim game, future games...).
class Minigame
{
public:
    virtual ~Minigame() = default;

    virtual void handleEvent(const sf::Event &e) = 0;
    virtual void update(float dt) = 0;
    virtual void draw(sf::RenderWindow &w, const sf::Font &font) = 0;
    virtual bool finished() const = 0;

    // score / reward the main loop can read right before it deletes the game
    virtual int result() const { return 0; }
};