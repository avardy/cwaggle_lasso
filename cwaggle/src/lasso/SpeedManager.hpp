#pragma once

#include "KeyboardCallback.hpp"
#include "Config.hpp"
#include <iostream>
using std::cout;
using std::endl;

/**
 * Manages the initialization and control of the renderSteps and simTimeStep
 * variables and their adjustment through the GUI.
 */
class SpeedManager : public KeyboardCallback {
private:
    Config &m_config;
    int m_renderSteps;
    double m_simTimeStep;
    int m_stepCount;

public:
    SpeedManager(Config &config)
        : m_config(config)
        , m_renderSteps(m_config.renderSteps)
        , m_simTimeStep(m_config.simTimeStep)
        , m_stepCount(0)
    {
    }

    void keyHandler(sf::Keyboard::Key key)
    {
        switch (key) {
        case sf::Keyboard::Up:
            increaseSpeed();
            break;
        case sf::Keyboard::Down:
            decreaseSpeed();
            break;
        case sf::Keyboard::Space:
            togglePause();
            break;
        case sf::Keyboard::X:
            m_stepCount = m_config.maxTimeSteps;
            break;

        default:
            break;
        }
    }

    int getStepCount() const { return m_stepCount; }

    void incrementStepCount() {
        m_stepCount++;
    }

    int getRenderSteps() const { return m_renderSteps; }

    double getSimTimeStep() const { return m_simTimeStep; }

    void decreaseSpeed()
    {
        if (m_renderSteps > 1) {
            m_renderSteps--;
        } else if (m_renderSteps == 1 && m_simTimeStep > 0.1) {
            m_simTimeStep -= 0.1;
        }
        cout << "m_renderSteps: " << m_renderSteps << endl;
        cout << "m_simTimeStep: " << m_simTimeStep << endl;
    }

    void increaseSpeed()
    {
        if (m_renderSteps == 1 && m_simTimeStep < 1) {
            m_simTimeStep += 0.1;
        } else {
            m_renderSteps++;
        }
        cout << "m_renderSteps: " << m_renderSteps << endl;
        cout << "m_simTimeStep: " << m_simTimeStep << endl;
    }

    void togglePause()
    {
        if (m_simTimeStep > 0) {
            m_simTimeStep = 0;
            m_renderSteps = 1;
        } else {
            m_simTimeStep = 1;
            m_renderSteps = 1;
        }
        cout << "m_renderSteps: " << m_renderSteps << endl;
        cout << "m_simTimeStep: " << m_simTimeStep << endl;
    }
};