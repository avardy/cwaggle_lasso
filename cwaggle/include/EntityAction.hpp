#pragma once

#include "Entity.hpp"
#include "Components.hpp"

#include <cassert>

/**
 * This class serves the role of representing an entity's "intended" action, but it actually
 * plays a big role in simulating the physics.  Initially, it was essentially a pair of
 * linear speed and angular speed.  Now, direct control over angular speed is removed, and the
 * input (args to the constructor) are linear speed and angular acceleration.  This acceleration
 * is integrated to compute the speed, which is further integrated to the angle.
 * 
 * Note: We could play the same game and require linear acceleration as an input, instead of
 * having direct control over speed.  This would be more realistic, but did not seem necessary
 * as the qualitative behaviour of the Kodama robots were replicated just with the angular change.
 */

class EntityAction
{
public:
    double m_angularAcceleration = 0;
    double m_speed = 0;

    EntityAction() {}
    EntityAction(double speed, double angle)
        : m_angularAcceleration(angle), m_speed(speed) {}

    inline const auto speed() const { return m_speed; }
    inline const auto angularAcceleration() const { return m_angularAcceleration; }

    virtual void doAction(Entity e, double timeStep)
    {
        if (!e.hasComponent<CSteer>())
        {
            e.addComponent<CSteer>();
        }
        auto &steer = e.getComponent<CSteer>();

        if (steer.frozen)
        {
            steer.speed = 0;
            return;
        }

        // Integerate from angular acceleration to angular speed.  A slowed body
        // cannot accelerate quickly.
        if (steer.slowedCount > 0)
            steer.angularSpeed += 0.1 * m_angularAcceleration * timeStep;
        else
            steer.angularSpeed += m_angularAcceleration * timeStep;

        // Incorporate a drag component.
        steer.angularSpeed *= 0.9;

        // Integrate from angular speed to angular position (i.e. angle).
        steer.angle += steer.angularSpeed * timeStep;

        steer.speed = m_speed;
    }
};