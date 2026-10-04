// cd "C:\Users\Noah\Desktop\visual novel"
//
// g++ anime.cpp -I"C:\SFML\include" -L"C:\SFML\lib" -lsfml-graphics -lsfml-window -lsfml-system -o anime.exe

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <SFML/Graphics.hpp>

using namespace std;

// ============================================================
// HELPERS
// ============================================================

bool isKeyword(const string& token)
{
    static const vector<string> keywords =
    {
        "int", "float", "bool", "void",
        "if", "else", "while", "for",
        "return", "true", "false",
        "const", "auto"
    };

    for (const auto& k : keywords)
    {
        if (token == k)
            return true;
    }

    return false;
}

// ============================================================
// GAME STATES
// ============================================================

enum class GameState
{
    Title,
    CharacterSelect,
    ShiroIntro,
    Battle,
    Victory,
    Defeat
};

// ============================================================
// RHYTHM NOTE
// ============================================================

struct Note
{
    int lane;
    float z;
    bool active = true;
};

// ============================================================
// BULLET
// ============================================================

struct Bullet
{
    sf::CircleShape shape;
    sf::Vector2f velocity;
};

// ============================================================
// RHYTHM TRACK
// ============================================================

class RhythmTrack
{
private:

    float perspective = 400.f;

    sf::Vector2f center =
    {
        365.f,
        305.f
    };

public:

    sf::Vector2f project(
        float x,
        float y,
        float z
    )
    {
        z = max(z, 0.1f);

        float scale =
            perspective / z;

        return
        {
            center.x + x * scale,
            center.y + y * scale
        };
    }

    void draw(
        sf::RenderWindow& window,
        const vector<Note>& notes
    )
    {
        const float zNear = 150.f;
        const float zFar = 850.f;

        const float yNear = 120.f;
        const float yFar = -80.f;

        const float widthNear = 620.f;
        const float widthFar = 80.f;

        // ----------------------------------------------------
        // TRACK SURFACE
        // ----------------------------------------------------

        sf::VertexArray track(
            sf::PrimitiveType::Triangles
        );

        sf::Color laneColors[4] =
        {
            sf::Color(0, 180, 255, 45),
            sf::Color(120, 40, 255, 45),
            sf::Color(255, 30, 100, 45),
            sf::Color(255, 180, 0, 45)
        };

        for (int lane = 0; lane < 4; lane++)
        {
            float nearLane =
                widthNear / 4.f;

            float farLane =
                widthFar / 4.f;

            float nearLeft =
                -widthNear / 2.f +
                nearLane * lane;

            float nearRight =
                nearLeft + nearLane;

            float farLeft =
                -widthFar / 2.f +
                farLane * lane;

            float farRight =
                farLeft + farLane;

            sf::Vector2f p1 =
                project(
                    nearLeft,
                    yNear,
                    zNear
                );

            sf::Vector2f p2 =
                project(
                    nearRight,
                    yNear,
                    zNear
                );

            sf::Vector2f p3 =
                project(
                    farRight,
                    yFar,
                    zFar
                );

            sf::Vector2f p4 =
                project(
                    farLeft,
                    yFar,
                    zFar
                );

            track.append(
                sf::Vertex(
                    p1,
                    laneColors[lane]
                )
            );

            track.append(
                sf::Vertex(
                    p2,
                    laneColors[lane]
                )
            );

            track.append(
                sf::Vertex(
                    p3,
                    laneColors[lane]
                )
            );

            track.append(
                sf::Vertex(
                    p1,
                    laneColors[lane]
                )
            );

            track.append(
                sf::Vertex(
                    p3,
                    laneColors[lane]
                )
            );

            track.append(
                sf::Vertex(
                    p4,
                    laneColors[lane]
                )
            );
        }

        window.draw(track);

        // ----------------------------------------------------
        // THE ACTUAL PERSPECTIVE LINES
        // ----------------------------------------------------

        for (int i = 0; i <= 4; i++)
        {
            float nearX =
                -widthNear / 2.f +
                widthNear / 4.f * i;

            float farX =
                -widthFar / 2.f +
                widthFar / 4.f * i;

            sf::Vector2f nearPoint =
                project(
                    nearX,
                    yNear,
                    zNear
                );

            sf::Vector2f farPoint =
                project(
                    farX,
                    yFar,
                    zFar
                );

            sf::Vertex line[] =
            {
                sf::Vertex(
                    nearPoint,
                    sf::Color(
                        0,
                        220,
                        255,
                        190
                    )
                ),

                sf::Vertex(
                    farPoint,
                    sf::Color(
                        0,
                        220,
                        255,
                        80
                    )
                )
            };

            window.draw(
                line,
                2,
                sf::PrimitiveType::Lines
            );
        }

        // ----------------------------------------------------
        // NOTES
        // ----------------------------------------------------

        for (const auto& note : notes)
        {
            if (!note.active)
                continue;

            if (
                note.z < zNear ||
                note.z > zFar
            )
                continue;

            float t =
                (note.z - zNear) /
                (zFar - zNear);

            float width =
                widthNear +
                (widthFar - widthNear) * t;

            float y =
                yNear +
                (yFar - yNear) * t;

            float laneWidth =
                width / 4.f;

            float x =
                -width / 2.f +
                laneWidth * note.lane;

            sf::Vector2f left =
                project(
                    x + 4.f,
                    y,
                    note.z
                );

            sf::Vector2f right =
                project(
                    x + laneWidth - 4.f,
                    y,
                    note.z
                );

            float noteHeight =
                max(
                    5.f,
                    22.f *
                    (perspective / note.z)
                );

            sf::VertexArray noteShape(
                sf::PrimitiveType::Triangles
            );

            sf::Color color =
                sf::Color(
                    0,
                    230,
                    255,
                    255
                );

            sf::Color white =
                sf::Color(
                    255,
                    255,
                    255,
                    255
                );

            sf::Vector2f topLeft =
            {
                left.x,
                left.y - noteHeight
            };

            sf::Vector2f topRight =
            {
                right.x,
                right.y - noteHeight
            };

            noteShape.append(
                sf::Vertex(
                    topLeft,
                    white
                )
            );

            noteShape.append(
                sf::Vertex(
                    topRight,
                    white
                )
            );

            noteShape.append(
                sf::Vertex(
                    right,
                    color
                )
            );

            noteShape.append(
                sf::Vertex(
                    topLeft,
                    white
                )
            );

            noteShape.append(
                sf::Vertex(
                    right,
                    color
                )
            );

            noteShape.append(
                sf::Vertex(
                    left,
                    color
                )
            );

            window.draw(noteShape);
        }

        // ----------------------------------------------------
        // HIT LINE
        // ----------------------------------------------------

        sf::Vector2f hitLeft =
            project(
                -widthNear / 2.f,
                yNear,
                zNear
            );

        sf::Vector2f hitRight =
            project(
                widthNear / 2.f,
                yNear,
                zNear
            );

        sf::Vertex hitLine[] =
        {
            sf::Vertex(
                hitLeft,
                sf::Color(
                    255,
                    255,
                    255,
                    220
                )
            ),

            sf::Vertex(
                hitRight,
                sf::Color(
                    255,
                    0,
                    150,
                    220
                )
            )
        };

        window.draw(
            hitLine,
            2,
            sf::PrimitiveType::Lines
        );
    }
};

// ============================================================
// MAIN
// ============================================================

int main()
{
    // ========================================================
    // WINDOW
    // ========================================================

    sf::RenderWindow window(
        sf::VideoMode({1280, 720}),
        "The Only Way Academy"
    );

    window.setFramerateLimit(60);

    // ========================================================
    // FONTS
    // ========================================================

    sf::Font font;
    sf::Font editorFont;
    sf::Font shiroFont;

    font.openFromFile(
        "Hyper Oxide.ttf"
    );

    editorFont.openFromFile(
        "FiraCode-Vf.ttf"
    );

    shiroFont.openFromFile(
        "Nitro Break.otf"
    );

    // ========================================================
    // TEXTURES
    // ========================================================

    sf::Texture backgroundTexture;
    sf::Texture shiroTexture;
    sf::Texture soraTexture;

    backgroundTexture.loadFromFile(
        "fuckemup.png"
    );

    shiroTexture.loadFromFile(
        "shiro.png"
    );

    soraTexture.loadFromFile(
        "sora.png"
    );

    backgroundTexture.setSmooth(false);
    shiroTexture.setSmooth(false);
    soraTexture.setSmooth(false);

    sf::Sprite background(
        backgroundTexture
    );

    background.setScale(
        {
            1280.f /
                backgroundTexture.getSize().x,

            720.f /
                backgroundTexture.getSize().y
        }
    );

    sf::Sprite shiro(
        shiroTexture
    );

    sf::Sprite sora(
        soraTexture
    );

    shiro.setOrigin(
        {
            shiro.getLocalBounds().size.x / 2.f,
            shiro.getLocalBounds().size.y / 2.f
        }
    );

    sora.setOrigin(
        {
            sora.getLocalBounds().size.x / 2.f,
            sora.getLocalBounds().size.y / 2.f
        }
    );

    // ========================================================
    // GAME STATE
    // ========================================================

    GameState state =
        GameState::Title;

    bool editorOpen = false;

    // ========================================================
    // TITLE
    // ========================================================

    sf::Text title(
        font,
        "THE ONLY WAY",
        52
    );

    title.setPosition(
        {
            400.f,
            105.f
        }
    );

    title.setFillColor(
        sf::Color::Black
    );

    sf::RectangleShape playButton(
        {
            250.f,
            70.f
        }
    );

    playButton.setPosition(
        {
            515.f,
            330.f
        }
    );

    playButton.setFillColor(
        sf::Color(
            10,
            10,
            25,
            230
        )
    );

    playButton.setOutlineColor(
        sf::Color::White
    );

    playButton.setOutlineThickness(2.f);

    sf::Text playText(
        font,
        "PLAY GAME",
        30
    );

    playText.setPosition(
        {
            575.f,
            348.f
        }
    );

    // ========================================================
    // BATTLE DATA
    // ========================================================

    RhythmTrack track;

    vector<Note> notes =
    {
        {0, 800.f, true},
        {1, 700.f, true},
        {2, 600.f, true},
        {3, 500.f, true},
        {0, 900.f, true},
        {2, 1000.f, true}
    };

    sf::Clock gameClock;

    float noteSpeed =
        300.f;

    int playerHP =
        100;

    int shiroHP =
        100;

    int combo =
        0;

    int score =
        0;

    float attackCooldown =
        0.f;

    // ========================================================
    // CODE ATTACK
    // ========================================================

    string attackCode =
        "// ATTACK\n"
        "int bullets = 5;\n"
        "float speed = 8.0f;\n"
        "float spread = 20.0f;\n";

    int attackBullets = 5;
    float attackSpeed = 8.f;
    float attackSpread = 20.f;

    // ========================================================
    // BULLETS
    // ========================================================

    vector<Bullet> bullets;

    // ========================================================
    // EDITOR
    // ========================================================

    sf::RectangleShape editor(
        {
            1200.f,
            620.f
        }
    );

    editor.setPosition(
        {
            30.f,
            20.f
        }
    );

    editor.setFillColor(
        sf::Color(
            10,
            12,
            18,
            250
        )
    );

    sf::RectangleShape editorBar(
        {
            1200.f,
            30.f
        }
    );

    editorBar.setPosition(
        {
            30.f,
            20.f
        }
    );

    editorBar.setFillColor(
        sf::Color(
            20,
            80,
            120
        )
    );

    sf::Text editorTitle(
        editorFont,
        "Disboard IDE // attack.cpp",
        14
    );

    editorTitle.setPosition(
        {
            42.f,
            26.f
        }
    );

    sf::RectangleShape buildButton(
        {
            160.f,
            45.f
        }
    );

    buildButton.setPosition(
        {
            550.f,
            560.f
        }
    );

    buildButton.setFillColor(
        sf::Color(
            0,
            120,
            190
        )
    );

    sf::Text buildText(
        editorFont,
        "BUILD",
        20
    );

    buildText.setPosition(
        {
            600.f,
            570.f
        }
    );

    // ========================================================
    // MAIN LOOP
    // ========================================================

    while (window.isOpen())
    {
        while (auto event =
            window.pollEvent())
        {
            if (
                event->is<
                    sf::Event::Closed
                >()
            )
            {
                window.close();
            }

            // =================================================
            // MOUSE
            // =================================================

            if (
                const auto* mouse =
                    event->getIf<
                        sf::Event::MouseButtonPressed
                    >()
            )
            {
                if (
                    mouse->button ==
                    sf::Mouse::Button::Left
                )
                {
                    sf::Vector2i pos =
                        sf::Mouse::getPosition(
                            window
                        );

                    sf::Vector2f m =
                    {
                        static_cast<float>(
                            pos.x
                        ),

                        static_cast<float>(
                            pos.y
                        )
                    };

                    // TITLE
                    if (
                        state ==
                        GameState::Title
                    )
                    {
                        if (
                            playButton
                                .getGlobalBounds()
                                .contains(m)
                        )
                        {
                            state =
                                GameState::CharacterSelect;
                        }
                    }

                    // CHARACTER SELECT
                    else if (
                        state ==
                        GameState::CharacterSelect
                    )
                    {
                        sf::FloatRect shiroBox(
                            {100.f, 200.f},
                            {400.f, 350.f}
                        );

                        if (
                            shiroBox.contains(m)
                        )
                        {
                            state =
                                GameState::ShiroIntro;
                        }
                    }

                    // SHIRO INTRO
                    else if (
                        state ==
                        GameState::ShiroIntro
                    )
                    {
                        state =
                            GameState::Battle;
                    }

                    // BATTLE
                    else if (
                        state ==
                        GameState::Battle
                    )
                    {
                        // OPEN EDITOR
                        sf::FloatRect editorBox(
                            {730.f, 590.f},
                            {500.f, 45.f}
                        );

                        if (
                            !editorOpen &&
                            editorBox.contains(m)
                        )
                        {
                            editorOpen = true;
                        }

                        // BUILD
                        if (
                            editorOpen &&
                            buildButton
                                .getGlobalBounds()
                                .contains(m)
                        )
                        {
                            size_t pos;

                            pos =
                                attackCode.find(
                                    "bullets = "
                                );

                            if (
                                pos !=
                                string::npos
                            )
                            {
                                attackBullets =
                                    stoi(
                                        attackCode.substr(
                                            pos + 10
                                        )
                                    );
                            }

                            pos =
                                attackCode.find(
                                    "speed = "
                                );

                            if (
                                pos !=
                                string::npos
                            )
                            {
                                attackSpeed =
                                    stof(
                                        attackCode.substr(
                                            pos + 8
                                        )
                                    );
                            }

                            pos =
                                attackCode.find(
                                    "spread = "
                                );

                            if (
                                pos !=
                                string::npos
                            )
                            {
                                attackSpread =
                                    stof(
                                        attackCode.substr(
                                            pos + 9
                                        )
                                    );
                            }

                            attackBullets =
                                clamp(
                                    attackBullets,
                                    1,
                                    20
                                );

                            attackSpeed =
                                clamp(
                                    attackSpeed,
                                    1.f,
                                    25.f
                                );

                            attackSpread =
                                clamp(
                                    attackSpread,
                                    1.f,
                                    360.f
                                );

                            editorOpen =
                                false;

                            cout
                                << "ATTACK BUILT\n";
                        }
                    }

                    // VICTORY / DEFEAT
                    else if (
                        state ==
                            GameState::Victory ||
                        state ==
                            GameState::Defeat
                    )
                    {
                        state =
                            GameState::Title;

                        playerHP = 100;
                        shiroHP = 100;
                        combo = 0;
                        score = 0;

                        for (
                            size_t i = 0;
                            i < notes.size();
                            i++
                        )
                        {
                            notes[i].active = true;
                            notes[i].z =
                                500.f +
                                i * 100.f;
                        }
                    }
                }
            }

            // =================================================
            // KEYBOARD
            // =================================================

            if (
                const auto* key =
                    event->getIf<
                        sf::Event::KeyPressed
                    >()
            )
            {
                // ESC CLOSES EDITOR
                if (
                    editorOpen &&
                    key->code ==
                        sf::Keyboard::Key::Escape
                )
                {
                    editorOpen = false;
                }

                // RHYTHM INPUT
                if (
                    state ==
                    GameState::Battle &&
                    !editorOpen
                )
                {
                    int lane = -1;

                    if (
                        key->code ==
                        sf::Keyboard::Key::A
                    )
                        lane = 0;

                    if (
                        key->code ==
                        sf::Keyboard::Key::S
                    )
                        lane = 1;

                    if (
                        key->code ==
                        sf::Keyboard::Key::D
                    )
                        lane = 2;

                    if (
                        key->code ==
                        sf::Keyboard::Key::F
                    )
                        lane = 3;

                    if (lane != -1)
                    {
                        bool hit = false;

                        for (
                            auto& note :
                            notes
                        )
                        {
                            if (
                                note.active &&
                                note.lane == lane &&
                                note.z >= 135.f &&
                                note.z <= 210.f
                            )
                            {
                                note.active =
                                    false;

                                hit = true;

                                combo++;

                                score +=
                                    100 *
                                    combo;

                                shiroHP -=
                                    5 +
                                    combo / 10;

                                if (
                                    shiroHP <= 0
                                )
                                {
                                    shiroHP = 0;

                                    state =
                                        GameState::Victory;
                                }

                                break;
                            }
                        }

                        if (!hit)
                        {
                            combo = 0;
                            playerHP -= 3;

                            if (
                                playerHP <= 0
                            )
                            {
                                playerHP = 0;

                                state =
                                    GameState::Defeat;
                            }
                        }
                    }
                }
            }

            // =================================================
            // EDITOR TEXT
            // =================================================

            if (
                editorOpen
            )
            {
                if (
                    const auto* text =
                        event->getIf<
                            sf::Event::TextEntered
                        >()
                )
                {
                    if (
                        text->unicode >= 32 &&
                        text->unicode < 127
                    )
                    {
                        attackCode +=
                            static_cast<char>(
                                text->unicode
                            );
                    }

                    if (
                        text->unicode == 13
                    )
                    {
                        attackCode +=
                            '\n';
                    }

                    if (
                        text->unicode == 8 &&
                        !attackCode.empty()
                    )
                    {
                        attackCode.pop_back();
                    }
                }
            }
        }

        // ====================================================
        // UPDATE
        // ====================================================

        float dt =
            gameClock.restart()
                .asSeconds();

        if (
            state ==
            GameState::Battle
        )
        {
            attackCooldown -= dt;

            // MOVE NOTES
            for (
                auto& note :
                notes
            )
            {
                if (!note.active)
                    continue;

                note.z -=
                    noteSpeed *
                    dt;

                if (
                    note.z < 110.f
                )
                {
                    note.z =
                        850.f +
                        static_cast<float>(
                            rand() % 500
                        );

                    note.lane =
                        rand() % 4;

                    combo = 0;
                    playerHP -= 2;

                    if (
                        playerHP <= 0
                    )
                    {
                        playerHP = 0;

                        state =
                            GameState::Defeat;
                    }
                }
            }

            // PLAYER ATTACK
            if (
                attackCooldown <= 0.f &&
                combo >= 5
            )
            {
                float start =
                    -attackSpread / 2.f;

                float step =
                    attackBullets > 1
                    ?
                    attackSpread /
                    (
                        attackBullets - 1
                    )
                    :
                    0.f;

                for (
                    int i = 0;
                    i < attackBullets;
                    i++
                )
                {
                    Bullet bullet;

                    bullet.shape =
                        sf::CircleShape(
                            5.f
                        );

                    bullet.shape.setOrigin(
                        {
                            5.f,
                            5.f
                        }
                    );

                    bullet.shape.setPosition(
                        {
                            100.f,
                            400.f
                        }
                    );

                    bullet.shape.setFillColor(
                        sf::Color(
                            0,
                            230,
                            255
                        )
                    );

                    float angle =
                        (
                            start +
                            step * i
                        ) *
                        3.14159265f /
                        180.f;

                    bullet.velocity =
                    {
                        cos(angle) *
                            attackSpeed *
                            20.f,

                        sin(angle) *
                            attackSpeed *
                            20.f
                    };

                    bullets.push_back(
                        bullet
                    );
                }

                attackCooldown =
                    0.8f;
            }

            // MOVE BULLETS
            for (
                auto& bullet :
                bullets
            )
            {
                bullet.shape.move(
                    bullet.velocity *
                    dt
                );
            }

            // REMOVE OLD BULLETS
            bullets.erase(
                remove_if(
                    bullets.begin(),
                    bullets.end(),
                    [](const Bullet& b)
                    {
                        sf::Vector2f p =
                            b.shape.getPosition();

                        return
                            p.x < -50.f ||
                            p.x > 700.f ||
                            p.y < -50.f ||
                            p.y > 600.f;
                    }
                ),
                bullets.end()
            );
        }

        // ====================================================
        // DRAW
        // ====================================================

        window.clear();

        // ====================================================
        // TITLE
        // ====================================================

        if (
            state ==
            GameState::Title
        )
        {
            window.draw(background);

            window.draw(title);

            window.draw(playButton);

            window.draw(playText);
        }

        // ====================================================
        // CHARACTER SELECT
        // ====================================================

        else if (
            state ==
            GameState::CharacterSelect
        )
        {
            window.draw(background);

            sf::Text selectText(
                font,
                "CHOOSE YOUR ROUTE",
                32
            );

            selectText.setPosition(
                {
                    450.f,
                    100.f
                }
            );

            window.draw(selectText);

            shiro.setScale(
                {
                    0.35f,
                    0.35f
                }
            );

            shiro.setPosition(
                {
                    300.f,
                    400.f
                }
            );

            sora.setScale(
                {
                    0.35f,
                    0.35f
                }
            );

            sora.setPosition(
                {
                    900.f,
                    400.f
                }
            );

            window.draw(shiro);
            window.draw(sora);

            sf::Text shiroText(
                shiroFont,
                "SHIRO",
                28
            );

            shiroText.setPosition(
                {
                    250.f,
                    550.f
                }
            );

            window.draw(shiroText);

            sf::Text soraText(
                shiroFont,
                "SORA",
                28
            );

            soraText.setPosition(
                {
                    850.f,
                    550.f
                }
            );

            window.draw(soraText);
        }

        // ====================================================
        // SHIRO INTRO
        // ====================================================

        else if (
            state ==
            GameState::ShiroIntro
        )
        {
            window.draw(background);

            shiro.setScale(
                {
                    0.45f,
                    0.45f
                }
            );

            shiro.setPosition(
                {
                    650.f,
                    300.f
                }
            );

            window.draw(shiro);

            sf::RectangleShape box(
                {
                    800.f,
                    150.f
                }
            );

            box.setPosition(
                {
                    240.f,
                    500.f
                }
            );

            box.setFillColor(
                sf::Color(
                    10,
                    10,
                    25,
                    235
                )
            );

            box.setOutlineColor(
                sf::Color(
                    0,
                    220,
                    255
                )
            );

            box.setOutlineThickness(
                2.f
            );

            window.draw(box);

            sf::Text dialogue(
                shiroFont,
                "You wanna fight?\nThen show me your code.",
                28
            );

            dialogue.setPosition(
                {
                    275.f,
                    525.f
                }
            );

            window.draw(dialogue);
        }

        // ====================================================
        // BATTLE
        // ====================================================

        else if (
            state ==
            GameState::Battle
        )
        {
            // BACKGROUND
            sf::RectangleShape bg(
                {
                    1280.f,
                    720.f
                }
            );

            bg.setFillColor(
                sf::Color(
                    1,
                    3,
                    8
                )
            );

            window.draw(bg);

            // HEADER
            sf::Text header(
                editorFont,
                "SHIRO // BATTLE.OS",
                20
            );

            header.setPosition(
                {
                    25.f,
                    15.f
                }
            );

            header.setFillColor(
                sf::Color(
                    255,
                    0,
                    120
                )
            );

            window.draw(header);

            // LEFT COMBAT PANEL
            sf::RectangleShape combat(
                {
                    690.f,
                    475.f
                }
            );

            combat.setPosition(
                {
                    18.f,
                    70.f
                }
            );

            combat.setFillColor(
                sf::Color(
                    3,
                    7,
                    15
                )
            );

            combat.setOutlineColor(
                sf::Color(
                    0,
                    220,
                    255
                )
            );

            combat.setOutlineThickness(
                1.f
            );

            window.draw(combat);

            // HP
            sf::Text playerHPText(
                editorFont,
                "PLAYER " +
                    to_string(playerHP),
                13
            );

            playerHPText.setPosition(
                {
                    35.f,
                    82.f
                }
            );

            window.draw(
                playerHPText
            );

            sf::Text bossHPText(
                editorFont,
                "SHIRO " +
                    to_string(shiroHP),
                13
            );

            bossHPText.setPosition(
                {
                    560.f,
                    82.f
                }
            );

            window.draw(
                bossHPText
            );

            // HP BARS
            sf::RectangleShape hp1(
                {
                    250.f,
                    10.f
                }
            );

            hp1.setPosition(
                {
                    35.f,
                    105.f
                }
            );

            hp1.setFillColor(
                sf::Color(
                    0,
                    220,
                    255
                )
            );

            hp1.setScale(
                {
                    playerHP / 100.f,
                    1.f
                }
            );

            window.draw(hp1);

            sf::RectangleShape hp2(
                {
                    180.f,
                    10.f
                }
            );

            hp2.setPosition(
                {
                    500.f,
                    105.f
                }
            );

            hp2.setFillColor(
                sf::Color(
                    255,
                    0,
                    120
                )
            );

            hp2.setScale(
                {
                    shiroHP / 100.f,
                    1.f
                }
            );

            window.draw(hp2);

            // TRACK
            track.draw(
                window,
                notes
            );

            // PLAYER
            sf::CircleShape player(
                18.f
            );

            player.setOrigin(
                {
                    18.f,
                    18.f
                }
            );

            player.setPosition(
                {
                    100.f,
                    400.f
                }
            );

            player.setFillColor(
                sf::Color(
                    0,
                    220,
                    255
                )
            );

            window.draw(player);

            // SHIRO
            shiro.setScale(
                {
                    0.25f,
                    0.25f
                }
            );

            shiro.setPosition(
                {
                    600.f,
                    360.f
                }
            );

            window.draw(shiro);

            // BULLETS
            for (
                auto& bullet :
                bullets
            )
            {
                window.draw(
                    bullet.shape
                );
            }

            // COMBO
            sf::Text comboText(
                font,
                "COMBO " +
                    to_string(combo),
                22
            );

            comboText.setPosition(
                {
                    280.f,
                    590.f
                }
            );

            comboText.setFillColor(
                sf::Color(
                    0,
                    230,
                    255
                )
            );

            window.draw(
                comboText
            );

            // SCORE
            sf::Text scoreText(
                editorFont,
                "SCORE " +
                    to_string(score),
                13
            );

            scoreText.setPosition(
                {
                    35.f,
                    620.f
                }
            );

            window.draw(
                scoreText
            );

            // KEY GUIDE
            sf::Text keys(
                editorFont,
                "A       S       D       F",
                18
            );

            keys.setPosition(
                {
                    245.f,
                    650.f
                }
            );

            keys.setFillColor(
                sf::Color::White
            );

            window.draw(keys);

            // =================================================
            // RIGHT TERMINAL
            // =================================================

            sf::RectangleShape terminal(
                {
                    532.f,
                    590.f
                }
            );

            terminal.setPosition(
                {
                    730.f,
                    70.f
                }
            );

            terminal.setFillColor(
                sf::Color(
                    2,
                    6,
                    13
                )
            );

            terminal.setOutlineColor(
                sf::Color(
                    255,
                    0,
                    130
                )
            );

            terminal.setOutlineThickness(
                2.f
            );

            window.draw(terminal);

            sf::Text terminalTitle(
                editorFont,
                "ROOT // SHIRO_TERMINAL",
                13
            );

            terminalTitle.setPosition(
                {
                    750.f,
                    85.f
                }
            );

            terminalTitle.setFillColor(
                sf::Color(
                    255,
                    0,
                    130
                )
            );

            window.draw(
                terminalTitle
            );

            // CODE DISPLAY
            sf::Text codeDisplay(
                editorFont,
                attackCode,
                15
            );

            codeDisplay.setPosition(
                {
                    755.f,
                    125.f
                }
            );

            codeDisplay.setFillColor(
                sf::Color(
                    210,
                    220,
                    230
                )
            );

            window.draw(
                codeDisplay
            );

            // BUILD AREA
            sf::RectangleShape buildArea(
                {
                    500.f,
                    60.f
                }
            );

            buildArea.setPosition(
                {
                    746.f,
                    460.f
                }
            );

            buildArea.setFillColor(
                sf::Color(
                    6,
                    10,
                    20
                )
            );

            buildArea.setOutlineColor(
                sf::Color(
                    255,
                    0,
                    130
                )
            );

            buildArea.setOutlineThickness(
                2.f
            );

            window.draw(
                buildArea
            );

            sf::Text buildLabel(
                font,
                "BUILD ATTACK",
                22
            );

            buildLabel.setPosition(
                {
                    900.f,
                    475.f
                }
            );

            window.draw(
                buildLabel
            );

            // EDITOR BUTTON
            sf::RectangleShape editButton(
                {
                    500.f,
                    45.f
                }
            );

            editButton.setPosition(
                {
                    746.f,
                    590.f
                }
            );

            editButton.setFillColor(
                sf::Color(
                    0,
                    90,
                    140
                )
            );

            window.draw(
                editButton
            );

            sf::Text editLabel(
                editorFont,
                "OPEN ATTACK EDITOR",
                15
            );

            editLabel.setPosition(
                {
                    900.f,
                    603.f
                }
            );

            window.draw(
                editLabel
            );

            // =================================================
            // EDITOR
            // =================================================

            if (editorOpen)
            {
                window.draw(editor);
                window.draw(editorBar);
                window.draw(editorTitle);

                sf::Text code(
                    editorFont,
                    attackCode,
                    18
                );

                code.setPosition(
                    {
                        55.f,
                        70.f
                    }
                );

                code.setFillColor(
                    sf::Color(
                        220,
                        220,
                        220
                    )
                );

                window.draw(code);

                window.draw(
                    buildButton
                );

                window.draw(
                    buildText
                );

                sf::Text close(
                    editorFont,
                    "ESC = CLOSE",
                    13
                );

                close.setPosition(
                    {
                        1040.f,
                        26.f
                    }
                );

                window.draw(close);
            }
        }

        // ====================================================
        // VICTORY
        // ====================================================

        else if (
            state ==
            GameState::Victory
        )
        {
            sf::RectangleShape bg(
                {
                    1280.f,
                    720.f
                }
            );

            bg.setFillColor(
                sf::Color(
                    2,
                    10,
                    15
                )
            );

            window.draw(bg);

            sf::Text victory(
                font,
                "SHIRO DEFEATED",
                50
            );

            victory.setPosition(
                {
                    410.f,
                    250.f
                }
            );

            victory.setFillColor(
                sf::Color(
                    0,
                    230,
                    255
                )
            );

            window.draw(victory);

            sf::Text scoreResult(
                editorFont,
                "FINAL SCORE: " +
                    to_string(score) +
                    "\n\nCLICK TO RETURN",
                20
            );

            scoreResult.setPosition(
                {
                    500.f,
                    340.f
                }
            );

            window.draw(
                scoreResult
            );
        }

        // ====================================================
        // DEFEAT
        // ====================================================

        else if (
            state ==
            GameState::Defeat
        )
        {
            sf::RectangleShape bg(
                {
                    1280.f,
                    720.f
                }
            );

            bg.setFillColor(
                sf::Color(
                    15,
                    2,
                    8
                )
            );

            window.draw(bg);

            sf::Text defeat(
                font,
                "SYSTEM FAILURE",
                50
            );

            defeat.setPosition(
                {
                    430.f,
                    250.f
                }
            );

            defeat.setFillColor(
                sf::Color(
                    255,
                    0,
                    100
                )
            );

            window.draw(defeat);

            sf::Text retry(
                editorFont,
                "CLICK TO RETRY",
                20
            );

            retry.setPosition(
                {
                    520.f,
                    350.f
                }
            );

            window.draw(retry);
        }

        window.display();
    }

    return 0;
}