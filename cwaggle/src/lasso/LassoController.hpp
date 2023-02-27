#pragma once

#include "CWaggle.h"
#include "EntityControllers.hpp"
#include "TrackedSensor.hpp"
#include "DigitalFilters.h"
#include "SensorTools.hpp"
#include "Config.hpp"
#include <math.h>
#include <random>
#include <algorithm>
#include <queue>
#include <deque>

using namespace std;

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

// Define a fixed-length queue in which to store past robot positions.  We use this to engage
// an escape behaviour when the robot appears to be stuck.
// https://stackoverflow.com/questions/56334492/c-create-fixed-size-queue 
template <typename T, int MaxLen, typename Container=std::deque<T>>
class FixedLengthQueue : public std::queue<T, Container> {
public:
    void push(const T& value) {
        if (this->size() == MaxLen) {
           this->c.pop_front();
        }
        std::queue<T, Container>::push(value);
    }
};

enum class State {NORMAL, SATISFIED, AT_BORDER, STOPPED};

ostream& operator<< (ostream& out, State& state)
{
    switch(state) {
        case State::NORMAL:     out << "NORMAL"; break;
        case State::SATISFIED:  out << "SATISFIED"; break;
        case State::AT_BORDER:  out << "AT_BORDER"; break;
        case State::STOPPED:    out << "STOPPED"; break;
    }
    return out;
}

CColor toColor(State& state)
{
    switch(state) {
        case State::NORMAL:     return CColor(127, 127, 127, 127); break;
        case State::SATISFIED:  return CColor(127, 255, 127, 127); break;
        case State::AT_BORDER:  return CColor(127, 127, 255, 127); break;
        case State::STOPPED:    return CColor(227, 227, 227, 127); break;
    }
}


class LassoController : public EntityController
{
    std::shared_ptr<World> m_world;
    Entity m_robot;
    default_random_engine &m_rng;
    Config m_config;

    TrackedSensor m_trackedSensor;

    CControllerVis &m_visComponent;
    CVectorIndicator m_indicator;

    FixedLengthQueue<Vec2, 50> m_positionQueue;
    uniform_real_distribution<double> m_escapeNoiseDistV;
    uniform_real_distribution<double> m_escapeNoiseDistW;

    uniform_real_distribution<double> m_blindResetDist;
    uniform_real_distribution<double> m_blindTauDist;

public:
    // m_tau represents the isoline the robot is trying to follow.  It is retained as
    // the only piece of state information in between calls to getAction.
    double m_tau = 0.5;

    double m_medianTau = 0.5;

    double m_filteredTau = 0;

    // If m_config.controllerState is true then we will retain this addtional state
    // variable which tracks completion of the construction task, movement of all robots to the border,
    // then stopping behind the first robot that detects the starting line of the second scalar field.
    State m_state = State::NORMAL;
//private:

    int m_laps = 0;
    double m_lastStartBar = 0;

    double m_sampleTime;
    HighPassFilter3 m_highPassFilter;

    vector<double> m_tauDuringLoop;

    int m_escapeCountdown = 0;

    //
    // These quantites are data members just for convenience.  They are reset upon every call
    // to getAction.
    //

    // Forward and angular speeds.
    double m_v, m_w;

    Vec2 m_robotPos, m_targetPos;
    bool m_targetValid, m_slow, m_stop;

    SensorReading m_reading;

public:
    std::map<std::string, double> outputParams;

    LassoController(Entity robot, std::shared_ptr<World> world, default_random_engine &rng, Config &config)
        : m_world(world)
        , m_robot(robot)
        , m_rng(rng)
        , m_config(config)
        , m_trackedSensor(robot, rng, config)
        , m_visComponent(m_robot.addComponent<CControllerVis>())
        , m_indicator(3.14, 20, 255, 0, 0, 255)
        , m_escapeNoiseDistV(-0.5, 0.1)
        , m_escapeNoiseDistW(-0.5, 0.5)
        , m_blindResetDist(0, 1.0)
        , m_blindTauDist(m_world->getGrid(0).getMinimumAbove(0), m_world->getGrid(0).getMaximumBelow(1))
        , m_sampleTime(0.01)
        , m_highPassFilter(m_sampleTime, 2.0 * M_PI * m_config.filterConstant)
    {
        m_robotPos = m_robot.getComponent<CTransform>().p;
        m_positionQueue.push(m_robotPos);
    }

    EntityAction getAction()
    {
        m_robotPos = m_robot.getComponent<CTransform>().p;
        SensorTools::ReadSensorArray(m_robot, m_world, m_reading);

        if (!m_config.controllerState) {

            // The algorithm without state (i.e. with no stopping criterion).
            computeTau();

            m_v = 0;
            m_w = 0;
            computeSpeeds();
            slowOrStop();
            escapeIfStuck();

        } else {
            // The algorithm with state (i.e. with a stopping criterion).
        
            // First determine if we've just hit the start bar.  If so, we apply some filtering
            // on the raw tau values that were encountered during this past "lap" and change
            // state from NORMAL to SATISFIED if the task seems to be completed (indicated by
            // a positive zero-crossing of the high-pass filter response).
            double startBar = m_world->getGrid(1).get(round(m_robotPos.x), round(m_robotPos.y));
            bool hitStartBar = startBar < 0.1 && m_lastStartBar > 0.9;
            if (hitStartBar) {
                m_laps++;

                std::sort(m_tauDuringLoop.begin(), m_tauDuringLoop.end());
                if (m_tauDuringLoop.size() > 0)
                    m_medianTau = m_tauDuringLoop[m_tauDuringLoop.size() / 2];

                double m_lastFilteredTau = m_highPassFilter.getOutput();
                m_filteredTau = m_highPassFilter.update(m_medianTau);
                /* DISABLE THIS SECTION TO DISABLE STATE CHANGES (e.g. FOR SHOWING THE FILTERING) */
                if (m_state == State::NORMAL && m_lastFilteredTau < 0 && m_filteredTau > 0) {
                    m_state = State::SATISFIED;
                    m_tau = 1.0;
                }
                /**/
                m_tauDuringLoop.clear();
            }
            m_lastStartBar = startBar;

            if (m_state == State::NORMAL)
                computeTau();

            m_tauDuringLoop.push_back(m_tau);

            m_v = 0;
            m_w = 0;
            if (m_state != State::STOPPED)
                computeSpeeds();

            if (m_state == State::SATISFIED && !m_targetValid)
                m_state = State::AT_BORDER;

            slowOrStop();
            if (m_state == State::AT_BORDER && hitStartBar)
                m_state = State::STOPPED;

            if (m_state == State::NORMAL || m_state == State::SATISFIED)
                escapeIfStuck();
        }

        if (m_escapeCountdown > 0)
            --m_escapeCountdown;

        //
        // Debug / visualization
        //

        m_robot.addComponent<CColor>(toColor(m_state));
        if (m_robot.getComponent<CControllerVis>().selected) {
            auto &visGrid = m_world->getGrid(5);
            visGrid.addContour(m_tau, m_world->getGrid(0), 1.0);

            std::stringstream ss;
            ss << "state: \t" << m_state << endl
               << "laps: \t" << m_laps << endl
               << "slow: \t" << m_slow << endl
               << "stop: \t" << m_stop << endl
               << "tau: \t" << m_tau << endl
               << "medianTau: \t" << m_medianTau << endl
               << "filteredTau: \t" << m_filteredTau << endl
               << "v, w: \t" << m_v << ", " << m_w << endl;
            m_visComponent.msg = ss.str();
        }
        setIndicator();

        // It is through passing 'outputParams' that we control (and debug) the live robots
        // via CWaggleBridge.py and cwaggle_bridge.so.  It is intentional that we pass
        // v and w (forward and angular velocity) prior to their being scaled in the return
        // line below.  This is because of the need to trade off between forward and angular
        // speed in a different way on the actual robots.
        outputParams["v"] = m_v;
        outputParams["w"] = m_w;
        outputParams["tau"] = m_tau;
        //outputParams["scenario"] = scenario;
        outputParams["targetX"] = m_targetPos.x;
        outputParams["targetY"] = m_targetPos.y;

        m_previousAction = EntityAction(m_v * m_config.maxForwardSpeed, m_w * m_config.maxAngularSpeed);
        return m_previousAction;
    }

    int getStateAsInt() {
        return static_cast<std::underlying_type<State>::type>(m_state);
    }

private:

    void computeTau()
    {
        if (m_config.controllerBlindness == 1) {
            // BLIND CONTROLLER (VARIANT 1): If tau has not been set before, or with a small probability,
            // set tau randomly.
            if (m_tau == 0.5 || m_blindResetDist(m_rng) < 0.001) {
                m_tau = m_blindTauDist(m_rng);
            }
        } else if (m_config.controllerBlindness == 2) {
            // BLIND CONTROLLER (VARIANT 2): If tau has not been set before, or with a small probability,
            // set tau to a randomly selected scalar field in the range (0, 1).
            if (m_tau == 0.5 || m_blindResetDist(m_rng) < 0.001) {
                bool foundValid = false;
                uniform_int_distribution<int> Xrng(0, m_world->width());
                uniform_int_distribution<int> Yrng(0, m_world->height());
                while (!foundValid) {
                    m_tau = m_world->getGrid(0).get(Xrng(m_rng), Yrng(m_rng));
                    foundValid = m_tau > 0 && m_tau < 1;
                }
            }
        } else {
            // NORMAL CASE: Controller is not blinded in any way.
            bool puckValid = false;
            double puckValue = m_trackedSensor.getExtreme(m_world, m_robot, "red_puck", SensorOp::GET_MAX_DTG,
                                                    0, 1, m_config.puckSensingDistance, puckValid);

            if (puckValid)
                m_tau = puckValue;
        }

//m_tau *= 0.9995;
        //m_tauConflict = m_tau < lowerRobotValue || m_tau > upperRobotValue;
    }

    void computeSpeeds() 
    {
        m_targetValid = false;
        Vec2 target = m_trackedSensor.getTargetPointFromCircle(m_world, m_robot, m_tau, m_targetValid);

        if (m_targetValid) {
            double robotAngle = m_robot.getComponent<CSteer>().angle;
            double dx = target.x - m_robotPos.x;
            double dy = target.y - m_robotPos.y;
            double alpha = Angles::getSmallestSignedAngularDifference(atan2(dy, dx), robotAngle);

            // v = 1;
            // w = alpha / M_PI;
            m_v = pow(cos(alpha), 3);
            m_w = pow(sin(alpha), 3);
        } else {
            // We are close to the border, just turn right.
            m_v = 0;
            m_w = 0.25;
        }
    }

    void slowOrStop()
    {
        m_stop = m_reading.robotAheadClose;
        m_slow = m_reading.robotAheadFar;

        // Applying the choice made above (if any).
        if (m_stop) {
            m_v = 0;
        } else if (m_slow) {
            m_v *= 0.25;
        }
    }

    void escapeIfStuck() 
    {
        Vec2 oldPos = m_positionQueue.front();
        if (m_robotPos.dist(oldPos) < 1) {
            // We seem to be stuck.  Try and escape!
            m_escapeCountdown = m_config.escapeDuration;
        }

        if (m_escapeCountdown > 0) {
            m_v = m_escapeNoiseDistV(m_rng);
            m_w = m_escapeNoiseDistW(m_rng);
        }

        m_positionQueue.push(m_robotPos);
    }

    void setIndicator()
    {
        m_indicator.angle = 0;

        m_indicator.length = 20 * m_v;

        if (m_w >= 0) {
            m_indicator.r = 255 * (1 - m_w);
            m_indicator.g = 0;
            m_indicator.b = 0;
        } else if (m_w < 0) {
            m_indicator.r = 0;
            m_indicator.g = 0;
            m_indicator.b = 255 * (1 + m_w);
        }
        m_indicator.a = 255;
        m_robot.addComponent<CVectorIndicator>(m_indicator);
    }

};