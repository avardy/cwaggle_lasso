#pragma once

#include <SFML/Graphics.hpp>

/**
 * Not exactly a callback, but an object that might respond to key events.
 * This is an abstract class.
 */
class KeyboardCallback {
public:
    virtual void keyHandler(sf::Keyboard::Key key) = 0;
};