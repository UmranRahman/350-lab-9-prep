#include <iostream>
#include <vector>

#include "camera.h"
#include <SFML/Graphics.hpp>

constexpr size_t WINDOW_WIDTH = 1280;
constexpr size_t WINDOW_HEIGHT = 720;
constexpr float LINE_WIDTH = 4;
constexpr float PLAYER_WIDTH = 50;
constexpr float PLAYER_SPEED = 20;
constexpr float GRAVITY = 4;
constexpr float JUMP_SPEED = -50;

constexpr float FLOOR_HEIGHT = 140;
constexpr float WORLD_TEX_WIDTH = 576;
constexpr float WORLD_TEX_HEIGHT = 160;
constexpr float WORLD_WIDTH = WORLD_TEX_WIDTH * (WINDOW_HEIGHT / WORLD_TEX_HEIGHT);
constexpr float WORLD_HEIGHT = WINDOW_HEIGHT;
constexpr float SOFT_LIMIT_PERCENT = 1.f / 3;
constexpr float HARD_LIMIT_PERCENT = 1.f / 5;

constexpr float SOFT_CAMERA_ALPHA = 0.1f;

struct Player {
    Player()
        : position(WINDOW_WIDTH / 2, WINDOW_HEIGHT - FLOOR_HEIGHT),
          velocity{},
          shape({PLAYER_WIDTH, PLAYER_WIDTH}),
          isJumping{false} {
        shape.setFillColor(sf::Color::Green);
        shape.setOrigin({PLAYER_WIDTH / 2, PLAYER_WIDTH});
        shape.setPosition({WINDOW_WIDTH / 2, WINDOW_HEIGHT - FLOOR_HEIGHT});
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const {
        target.draw(shape, states);
    }

    // should have a deltaTime here if we didn't have fixed fps.
    void update() {
        position += velocity;
        position.x = std::clamp(position.x, PLAYER_WIDTH * 0.5f, WORLD_WIDTH - PLAYER_WIDTH * 0.5f);
        if (position.y >= WINDOW_HEIGHT - FLOOR_HEIGHT) {
            position.y = WINDOW_HEIGHT - FLOOR_HEIGHT;
        }

        shape.setPosition(position);
    }

    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::RectangleShape shape;
    bool isJumping;
};

int main() {
    sf::RenderWindow window(sf::VideoMode{{WINDOW_WIDTH, WINDOW_HEIGHT}}, "Cameras");
    window.setFramerateLimit(60);

    // Load background
    sf::Texture backgroundTexture;
    if (!backgroundTexture.loadFromFile("assets/grunge-tileset.png")) {
        return 1;
    }
    sf::Sprite backgroundSprite(backgroundTexture);
    float backgroundScaleFactor = WORLD_HEIGHT / WORLD_TEX_HEIGHT;
    backgroundSprite.setScale({backgroundScaleFactor, backgroundScaleFactor});

    sf::RectangleShape leftSoftLine({4, 720});
    leftSoftLine.setFillColor(sf::Color(0, 255, 0, 128));
    leftSoftLine.setOrigin({LINE_WIDTH / 2, 0});
    leftSoftLine.setPosition({WINDOW_WIDTH / 3.f, 0});

    sf::RectangleShape rightSoftLine({4, 720});
    rightSoftLine.setFillColor(sf::Color(0, 255, 0, 128));
    rightSoftLine.setOrigin({LINE_WIDTH / 2, 0});
    rightSoftLine.setPosition({WINDOW_WIDTH * 2.f / 3.f, 0});

    sf::RectangleShape leftHardLine({4, 720});
    leftHardLine.setFillColor(sf::Color(255, 0, 0, 128));
    leftHardLine.setOrigin({LINE_WIDTH / 2, 0});
    leftHardLine.setPosition({WINDOW_WIDTH * HARD_LIMIT_PERCENT, 0});

    sf::RectangleShape rightHardLine({4, 720});
    rightHardLine.setFillColor(sf::Color(255, 0, 0, 128));
    rightHardLine.setOrigin({LINE_WIDTH / 2, 0});
    rightHardLine.setPosition({WINDOW_WIDTH * (1.f - HARD_LIMIT_PERCENT), 0});

    Player player;

    sf::View worldView = window.getView();
    sf::View guiView = window.getView();
    Camera camera(sf::Vector2f(window.getSize()), {WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f});

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                return 0;
            } else if (auto keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scan::Q) {
                    return 0;
                }
            }
        }

        float ax = 0;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D)) {
            ax += PLAYER_SPEED;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A)) {
            ax -= PLAYER_SPEED;
        }
        player.velocity.x = ax;
        if ((player.position.y >= WINDOW_HEIGHT - FLOOR_HEIGHT)) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Space)) {
                player.velocity.y += JUMP_SPEED;
            }
        } else {
            player.velocity.y += GRAVITY;
        }
        player.update();
        // camera

        // TODO: Remove this line and replace it with code that determines if the player's (center)
        // is between the two green lines (open space), in the soft exclusion zones (between the
        // green and red lines), or in the hard exclusion zones (beyond either of the red lines).
        //
        // If the player is in the open zone, the camera does not need to move.
        //
        // If the player is in the soft exclusion zones, the camera should softly translate over
        // multiple frames (via an exponential moving average) until the player's center is aligned
        // with the violated soft boundary. Use SOFT_CAMERA_ALPHA for the alpha value.
        //
        // If the player is in the hard exclusion zones, the camera should hard-adjust so that
        // the player's center is always kept at most *at* the red lines. This means that the
        // player center must NEVER pass the red lines.
        //
        // Use SOFT_LIMIT_PERCENT and HARD_LIMIT_PERCENT to parameterize the green and red
        // line positions.

        /**
         * Camera tracking (x-axis only). Zones are defined relative to the WINDOW:
         *   dead zone : between the green lines      -> camera doesn't move
         *   soft zone : between green and red lines  -> camera eases toward the player
         *   hard zone : beyond the red lines         -> camera snaps so the player is ON the red line
         *
         * Each frame we convert the player's world position to a window percent
         * (worldToWindowPercent), so the zone lines are just SOFT_LIMIT_PERCENT and
         * HARD_LIMIT_PERCENT. In the soft zone we move the camera by
         * SOFT_CAMERA_ALPHA * overshoot (an exponential moving average: close a fixed
         * fraction of the remaining gap each frame, so it eases out). Then we re-measure
         * and apply the full remaining overshoot past the red lines, which guarantees
         * the player's center never crosses them. Soft runs first, hard runs last.
         */
        /**
         * AI disclosure: I used Claude (Anthropic) as a tutor while doing this lab.
         * It walked me through the idea and structure of camera.h and the camera
         * logic in main.cpp step by step, and helped me debug my CMake/WSL build
         * setup. I typed and tested the code myself, ran it to confirm the camera
         * behavior, and removed my temporary test function afterward.
         */
        /**
         * Reflection:
         * - Coordinate systems: a camera is an anchor (the world point under the
         *   window center) plus a scale. Conversions measure the offset from the
         *   anchor, rescale it, and add the other anchor. Points need the anchor;
         *   deltas only need the rescale.
         * - Normalizing helps: working in window percent made the zone lines
         *   resolution-independent and turned the logic into simple comparisons.
         * - EMA easing: s += alpha * (target - s) needs no extra state and gives
         *   smooth, damped motion because the step shrinks as the gap shrinks.
         * - Order of operations: soft nudge first, then re-measure, then hard clamp,
         *   so the hard limit always has the final say.
         * - Process: testing with a scale other than 1 catches bugs that would
         *   otherwise pass silently, and I learned to configure CMake before building.
         */
        float px = camera.worldToWindowPercent(player.position).x;  // player's screen x, 0..1
        float leftSoft = SOFT_LIMIT_PERCENT;
        float rightSoft = 1.f - SOFT_LIMIT_PERCENT;

        float overshoot = 0.f;  // how far past the violated green line, as a window percent
        if (px < leftSoft) {
            overshoot = px - leftSoft;       // negative: player is left of the line
        } else if (px > rightSoft) {
            overshoot = px - rightSoft;      // positive: player is right of the line
        }
        camera.translateWindowPercent({SOFT_CAMERA_ALPHA * overshoot, 0.f});

        // Hard limit: the player's center must never pass the red lines.
        float leftHard = HARD_LIMIT_PERCENT;
        float rightHard = 1.f - HARD_LIMIT_PERCENT;

        px = camera.worldToWindowPercent(player.position).x;  // re-measure after the soft nudge
        if (px < leftHard) {
            camera.translateWindowPercent({px - leftHard, 0.f});   // negative: camera moves left
        } else if (px > rightHard) {
            camera.translateWindowPercent({px - rightHard, 0.f});  // positive: camera moves right
        }

        worldView.setCenter(camera.getCameraCenter());
        window.clear(sf::Color::Black);
        // world rendering
        window.setView(worldView);
        window.draw(backgroundSprite);
        player.draw(window, sf::RenderStates{});
        // gui rendering
        window.setView(guiView);
        window.draw(leftSoftLine);
        window.draw(rightSoftLine);
        window.draw(leftHardLine);
        window.draw(rightHardLine);
        // display to screen
        window.display();
    }
    return 0;
}
