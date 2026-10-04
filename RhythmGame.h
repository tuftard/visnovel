#pragma once
#include <vector>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/Vertex.hpp>

struct RhythmNote
{
    int lane;
    float z;
    bool active = true;
};

class RhythmTrack
{
private:
    float perspective = 400.f;
    sf::Vector2f center = {365.f, 305.f};

public:
    sf::Vector2f project(float x, float y, float z);
    void draw(sf::RenderWindow& window, const std::vector<RhythmNote>& notes);
};
