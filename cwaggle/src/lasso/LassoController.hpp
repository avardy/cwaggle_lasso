#pragma once

/*

Forked from LassoController.hpp to exclude experimental features such as 
task completion detection and the various forms of "blindness". This version
adds getSGFstate().

*/


#include "CWaggle.h"
#include "EntityControllers.hpp"
#include "TrackedSensor.hpp"
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

public:
    // m_tau represents the isoline the robot is trying to follow.  It is retained as
    // the only piece of state information in between calls to getAction.
    double m_tau = 0.5;

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
    {
        m_robotPos = m_robot.getComponent<CTransform>().p;
        m_positionQueue.push(m_robotPos);
    }

    EntityAction getAction()
    {
        m_robotPos = m_robot.getComponent<CTransform>().p;
        SensorTools::ReadSensorArray(m_robot, m_world, m_reading);

        // Compute tau, defining the isoline of the scalar field to follow.
        computeTau();

        // Try and follow the isoline, but slow/stop if another robot is ahead,
        // and try a random movement if we appear to be stuck.
        m_v = 0;
        m_w = 0;
        computeSpeeds();
        slowOrStop();
        escapeIfStuck();

        if (m_escapeCountdown > 0)
            --m_escapeCountdown;

        //
        // Debug / visualization
        //

        int sgf = getSGFstate();
        if (sgf == 0)
            m_robot.addComponent<CColor>(CColor(100, 100, 255, 127));
        else if (sgf == 1)
            m_robot.addComponent<CColor>(CColor(100, 255, 100, 127));
        else if (sgf == 2)
            m_robot.addComponent<CColor>(CColor(255, 200, 0, 127));

        if (m_robot.getComponent<CControllerVis>().selected) {
            auto &visGrid = m_world->getGrid(5);
            visGrid.addContour(m_tau, m_world->getGrid(0), 1.0);

            std::stringstream ss;
            ss << "slow: \t" << m_slow << endl
               << "stop: \t" << m_stop << endl
               << "tau: \t" << m_tau << endl
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

    // Return 0, 1, or 2 to indicate the robot's state according to the SGF
    // model of Hamann and Reina:
    // 
    // Heiko Hamann and Andreagiovanni Reina. Scalability in computing and
    // robotics. IEEE Transactions on Computers, 71(6):1453–1465, 2021.
    //
    // 0: We interpret the robot as being in the "solo" state, meaning
    // that the robot is operating without interference from other robots.
    //
    // 1: We interpret the robot as being in the "grupo" state. It is in
    // contact with other robots, which are likely interfering with it.
    //
    // 2: We interpret the robot as being in the "fermo" state. The robot is
    // currently stuck and cannot meaningfully contribute to the task.
    int getSGFstate() {
        // To interpret as "fermo" we rely on the simulator itself, which sets
        // "slowedCount" to a non-zero value if the robot has hit another robot
        // or the border.

        auto & steer = m_robot.getComponent<CSteer>();
        if (m_reading.robotAheadClose) //steer.slowedCount > 0)
            return 2;
        else if (m_reading.robotAheadFar)
            return 1;
        else
            return 0;
    }

private:

    void computeTau()
    {
        bool puckValid = false;
        double puckValue = m_trackedSensor.getExtreme(m_world, m_robot, "red_puck", SensorOp::GET_MAX_DTG,
                                                0, 1, m_config.puckSensingDistance, puckValid);

        if (puckValid)
            m_tau = puckValue;
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