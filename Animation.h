#pragma once
#include <string>
#include <vector>
#include <SFML/Graphics/Texture.hpp>

bool loadPiskelFrames(const std::string& fileName, int layerIndex, std::vector<sf::Texture>& frames, int& frameWidth, int& frameHeight);
