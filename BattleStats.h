#pragma once

#include <vector>
#include "ShiroTerminal.h" // AttackParams

struct BattleStats
{
    int notesHit = 0;
    int hitsTaken = 0;
    int longestCombo = 0;
    std::vector<AttackParams> builds;

    bool fullCombo() const { return hitsTaken == 0 && notesHit > 0; }
};