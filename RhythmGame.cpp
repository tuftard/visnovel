#include "RhythmGame.h"
#include <algorithm>

using namespace std;

sf::Vector2f RhythmTrack::project(
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

void RhythmTrack::draw(
    sf::RenderWindow& window,
    const vector<RhythmNote>& notes
)
{
    const float zNear = 150.f;
    const float zFar = 850.f;

    const float yNear = 120.f;
    const float yFar = -80.f;

    const float widthNear = 620.f;
    const float widthFar = 80.f;


    // =====================================================
    // TRACK
    // =====================================================

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


    // =====================================================
    // LANE LINES
    // =====================================================

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


    // =====================================================
    // NOTES
    // =====================================================

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
                (
                    perspective /
                    note.z
                )
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


    // =====================================================
    // HIT LINE
    // =====================================================

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
