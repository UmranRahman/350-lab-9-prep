#ifndef CAMERA_H
#define CAMERA_H

#include <cassert> //in case if assert goes wrong
#include "vector_util.h"
#include <SFML/System.hpp>

class Camera {
public:
    Camera(const sf::Vector2f& windowSize, const sf::Vector2f& viewCenterWorld,
           float worldUnitsPerPixel = 1.f)
        : mWindowSize{windowSize},
          mViewCenterWorld{viewCenterWorld},
          mWorldUnitsPerPixel{worldUnitsPerPixel} {}

    // TODO: all of these:

    sf::Vector2f windowToWorld(const sf::Vector2f& pWindow) { 
        sf::Vector2f offsetPixels = pWindow -  mWindowSize/2.f; // from window center
        return mViewCenterWorld + offsetPixels*mWorldUnitsPerPixel; 
    };

    sf::Vector2f worldToWindow(const sf::Vector2f& pWorld) {
        sf::Vector2f offsetWorld = pWorld - mViewCenterWorld; // from view center
        return mWindowSize * 0.5f + offsetWorld / mWorldUnitsPerPixel;
    };

    sf::Vector2f windowPercentToWorld(const sf::Vector2f& pWindowPercent) {
        return windowToWorld(pWindowPercent * mWindowSize); // percent -> pixels -> world
    };

    sf::Vector2f worldToWindowPercent(const sf::Vector2f& pWorld) {
        return worldToWindow(pWorld) / mWindowSize; // world -> pixels -> percent
    };

    void translateWorld(const sf::Vector2f& deltaWorld) {
        mViewCenterWorld += deltaWorld;
    };

    void translateWindow(const sf::Vector2f& deltaScreen) {
        translateWorld(deltaScreen * mWorldUnitsPerPixel); //pixel -> world then translate
    };

    void translateWindowPercent(const sf::Vector2f& deltaScreenPercent) {
        translateWindow(deltaScreenPercent * mWindowSize); //percent -> pixel -> world using translateWindow then translate
    };

    void setCamera(const sf::Vector2f& pWorld, const sf::Vector2f& pWindow) {
        // Make window point (pWindow) map to world point (pWorld), keeping the scale.
        // Currently pWindow maps to windowToWorld(pWindow); shift by the difference.
        translateWorld(pWorld - windowToWorld(pWindow));
    };

    void setCameraPercent(const sf::Vector2f& pWorld, const sf::Vector2f& pWindowPercent) {
        setCamera(pWorld, pWindowPercent * mWindowSize);
    };

    void setCameraCenter(const sf::Vector2f& pWorld) {
        mViewCenterWorld = pWorld;
    };

    sf::Vector2f getCameraCenter() const {
        return mViewCenterWorld;
    };

private:
    sf::Vector2f mWindowSize;       // size of the window in screen pixels (i.e. window coords)
    sf::Vector2f mViewCenterWorld;  // world coords point corresponding to the center of our window.
    float mWorldUnitsPerPixel;      // distance in world coords covered by one pixel
};

#endif  // CAMERA_H
