#pragma once

#include <vector>
#include <cassert>
#include <memory>
#include <algorithm>

#include "Vec2.hpp"
#include "Timer.hpp"
#include "World.hpp"
#include "Components.hpp"

#define SLOWED_ROBOT_COUNT 100

struct CollisionData
{
    CTransform * t1;
    CTransform * t2;
    CCircleBody * b1;
    CCircleBody * b2;
};

typedef std::vector<Entity> EntityVec;

class Simulator
{
    std::shared_ptr<World> m_world;

    // physics configuration
    double m_timeStep = 1.0;  // time step per update call
    double m_overlapThreshold = 0.1;  // allow overlap of this amount without resolution
    double m_deceleration = 0.4;  // deceleration multiplier, replace with friction
    double m_stoppingSpeed = 0.001; // stop an object if moving less than this speed

    // time keeping
    double m_computeTime = 0;    // the CPU time of the last frame of collisions
    double m_computeTimeMax = 0;    // the max CPU time of collisions since init

    std::vector<CollisionData>  m_collisions;
    std::vector<CTransform>     m_fakeTransforms;
    std::vector<CCircleBody>    m_fakeBodies;

    std::vector<Entity>         m_collisionEntities;

    void movement()
    {
        // update entity's velocity from its heading and angle
        for (auto & entity : m_world->getEntities("robot"))
        { 
            if (!entity.hasComponent<CSteer>()) { continue; }

            auto & transform = entity.getComponent<CTransform>();
            auto & steer     = entity.getComponent<CSteer>();

            // update the entity velocity based on angle and speed
            if (steer.slowedCount > 0) {
                // AV: If the robot is slowed then it cannot reach its commanded velocity.
                transform.v.x = 0.1 * steer.speed * cos(steer.angle);
                transform.v.y = 0.1 * steer.speed * sin(steer.angle);
            } else {
                transform.v.x = steer.speed * cos(steer.angle);
                transform.v.y = steer.speed * sin(steer.angle);
            }
        }

        // apply acceleration, velocity to all circles
        for (auto e : m_world->getEntities())
        {
            auto & t = e.getComponent<CTransform>();

            if (t.v.length() < m_stoppingSpeed) { t.v = Vec2(0, 0); }
            t.a = t.v * -m_deceleration;
            t.p += t.v * m_timeStep;
            t.v += t.a * m_timeStep;
            t.moved = fabs(t.v.x) > 0 || fabs(t.v.y) > 0;
        }
    }

    void collisions()
    {
        Timer timer;
        timer.start();
        m_collisions.clear();
        m_fakeBodies.clear();
        m_fakeTransforms.clear();

        // we can skip collision checking for any circle that hasn't moved
        // static resolution doesn't alter speed, so movement not recorded
        // so if a circle collided last frame, consider it to have moved
        for (auto e : m_world->getEntities())
        {
            if (e.getComponent<CCircleBody>().collided) { e.getComponent<CTransform>().moved = true; }
            e.getComponent<CCircleBody>().collided = false;
        }

        // AV: No robot is slowed unless it hits another robot or the border.
        for (auto & entity : m_world->getEntities("robot"))
        { 
            if (!entity.hasComponent<CSteer>()) { continue; }
            auto & steer     = entity.getComponent<CSteer>();
            if (steer.slowedCount > 0)
                steer.slowedCount--;
        }

        auto & transforms   = EntityMemoryPool::Instance().getData<CTransform>();
        auto & bodies       = EntityMemoryPool::Instance().getData<CCircleBody>();
        auto tIt            = transforms.begin();
        auto bIt            = bodies.begin();

        for (auto e1 : m_collisionEntities)
        {
            //auto & t1 = e1.getComponent<CTransform>();
            //auto & b1 = e1.getComponent<CBody>();
            auto & t1 = *(tIt + e1.id());
            auto & b1 = *(bIt + e1.id());

            // step 1: check collisions of all circles against all lines
            for (auto & e : m_world->getEntities("line"))
            {
                auto & edge = e.getComponent<CLineBody>();

                bool collided = handleCollisionWithLineBody(b1, t1, edge, false);
                if (collided && e1.hasComponent<CSteer>()) {
                    // If this circlebody belongs to a robot, then slow it
                    auto & steer1 = e1.getComponent<CSteer>();
                    steer1.slowedCount = SLOWED_ROBOT_COUNT;
                }
            }

            // AV: step 1.5: check collisions of all circles against all robots with plows
            for (auto & e : m_world->getEntities("robot"))
            {
                if (!e.hasComponent<CPlowBody>()) { continue; }

                // Do not check with collisions between a robot's CircleBody and its own plow.
                if (e1.id() == e.id()) { continue; }

                auto & t = e.getComponent<CTransform>();
                auto & cb = e.getComponent<CCircleBody>();
                auto & pb = e.getComponent<CPlowBody>();
                auto & steer = e.getComponent<CSteer>();

                // We create (but do not store) a CLineBody object used to check
                // for collision with the current circle (b1).
                double xStart = t.p.x + pb.startLength * cos(steer.angle + pb.angle);
                double yStart = t.p.y + pb.startLength * sin(steer.angle + pb.angle);
                double xProw = t.p.x + pb.length * cos(steer.angle + pb.angle);
                double yProw = t.p.y + pb.length * sin(steer.angle + pb.angle);
                CLineBody wedgeLineBody(Vec2(xStart, yStart), Vec2(xProw, yProw), pb.width/2.0);

                bool collided = handleCollisionWithLineBody(b1, t1, wedgeLineBody, true);

                // If this circlebody belongs to a robot, then slow both of them
                if (collided && e1.hasComponent<CPlowBody>()) {
                    auto & steer1 = e1.getComponent<CSteer>();
                    auto & steer = e.getComponent<CSteer>();
                    steer1.slowedCount = SLOWED_ROBOT_COUNT;
                    steer.slowedCount = SLOWED_ROBOT_COUNT;
                }
            }
            
            // if this circle hasn't moved, we don't need to check collisions for it
            if (!t1.moved) { continue; }

            // step 2: check collisions of all circles against all other circles
            for (auto e2 : m_collisionEntities)
            {
                //auto & t2 = e2.getComponent<CTransform>();
                //auto & b2 = e2.getComponent<CBody>();
                auto & t2 = *(tIt + e2.id());
                auto & b2 = *(bIt + e2.id());

                if (t1.p.distSq(t2.p) > (b1.r + b2.r)*(b1.r + b2.r)) { continue; }
                if (e1.id() == e2.id()) { continue; }

                // calculate the actual distance and overlap between circles
                double dist = t1.p.dist(t2.p);
                double overlap = (b1.r + b2.r) - dist;

                // circles overlap if the overlap is positive
                if (overlap > m_overlapThreshold)
                {
                    if (dist == 0) {
                        // AV: Circles are coincident.  If unchecked, this leads to division by zero below.  
                        // Arbitrarily perturb body 1 by plus-or-minus 1 in x and y. 
                        t1.p.x += 1 - (rand() % 3);
                        t1.p.y += 1 - (rand() % 3);
                        continue;
                    }

                    // record that a collision took place between these two objects
                    m_collisions.push_back({ &t1, &t2, &b1, &b2 });

                    // calculate the static collision resolution (direct position modifier)
                    // scale how much we push each circle back in the static collision by mass ratio
                    Vec2 delta1 = (t1.p - t2.p) / dist * overlap * (b2.m / (b1.m + b2.m));
                    Vec2 delta2 = (t1.p - t2.p) / dist * overlap * (b1.m / (b1.m + b2.m));

                    // apply the static collision resolution and record collision
                    t1.p += delta1; 
                    t2.p -= delta2;
                    b1.collided = true;
                    b2.collided = true;

                    // If both circlebodys belongs to robots, then slow both of them
                    if (e1.hasComponent<CPlowBody>() && e2.hasComponent<CPlowBody>()) {
                        auto & steer1 = e1.getComponent<CSteer>();
                        auto & steer2 = e2.getComponent<CSteer>();
                        steer1.slowedCount = SLOWED_ROBOT_COUNT;
                        steer2.slowedCount = SLOWED_ROBOT_COUNT;
                    }
                }
            }
            // wraparound behavior
            //if (c1.p.x < 0) { c1.p.x += m_world->width(); }
            //if (c1.p.y < 0) { c1.p.y += m_world->height(); }
            //if (c1.p.x >= m_world->width()) { c1.p.x -= m_world->width(); }
            //if (c1.p.y >= m_world->height()) { c1.p.y -= m_world->height(); }
            
            // check for collisions with the bounds of the world
            if (t1.p.x - b1.r < 0) { t1.p.x = b1.r; b1.collided = true; }
            if (t1.p.y - b1.r < 0) { t1.p.y = b1.r; b1.collided = true; }
            if (t1.p.x + b1.r > m_world->width()) { t1.p.x = m_world->width() - b1.r;  b1.collided = true; }
            if (t1.p.y + b1.r > m_world->height()) { t1.p.y = m_world->height() - b1.r; b1.collided = true; }

            // AV: check for collisions between plows and bounds of the world.
            /*
            if (!e1.hasComponent<CPlowBody>()) { continue; }
            auto & pb = e1.getComponent<CPlowBody>();
            auto & steer = e1.getComponent<CSteer>();
            double xProw = t1.p.x + pb.length * cos(steer.angle + pb.angle);
            double yProw = t1.p.y + pb.length * sin(steer.angle + pb.angle);
            bool plowBorderCollision = false;
            if (xProw < 0) {t1.p.x -= xProw; b1.collided = true; plowBorderCollision = true; }
            if (yProw < 0) {t1.p.y -= yProw; b1.collided = true; plowBorderCollision = true; }
            if (xProw > m_world->width()) {t1.p.x -= xProw - m_world->width(); b1.collided = true; plowBorderCollision = true; }
            if (yProw > m_world->height()) {t1.p.y -= yProw - m_world->height(); b1.collided = true; plowBorderCollision = true; }
            // Special slow down for plow/border collisions.
            if (plowBorderCollision) {
                steer.slowedCount = SLOWED_ROBOT_COUNT;
            }
            */
        }

        // step 3: calculate and apply dynamic collision resolution to any detected collisions
        for (auto & collision : m_collisions)
        {
            auto t1 = collision.t1;
            auto t2 = collision.t2;
            auto b1 = collision.b1;
            auto b2 = collision.b2;

            // normal between the circles
            double dist = t1->p.dist(t2->p);
            if (dist == 0)
                // AV: Avoid division by zero below.
                dist = 1;
            double nx = (t2->p.x - t1->p.x) / dist;
            double ny = (t2->p.y - t1->p.y) / dist;

            // thank you wikipedia
            // https://en.wikipedia.org/wiki/Elastic_collision
            double kx = (t1->v.x - t2->v.x);
            double ky = (t1->v.y - t2->v.y);
            double p = 2.0f * (nx*kx + ny * ky) / (b1->m + b2->m);

            t1->v.x -= p * b2->m * nx;
            t1->v.y -= p * b2->m * ny;
            t2->v.x += p * b1->m * nx;
            t2->v.y += p * b1->m * ny;
        }

        // record the time that this collision calculation took
        m_computeTime = timer.getElapsedTimeInMilliSec();
        m_computeTimeMax = m_computeTime > m_computeTimeMax ? m_computeTime : m_computeTimeMax;
    }

    // Handles the collision between CCircleBody b1 at position/velocity t1 with the given CLineBody.
    // If treatAsCone is true then the CLineBody is treated as a cone with a "fat" and a "thin" end.
    bool handleCollisionWithLineBody(CCircleBody &b1, CTransform &t1, CLineBody &lineBody, 
                                     bool treatAsCone) {
        double lineX1 = lineBody.e.x - lineBody.s.x;
        double lineY1 = lineBody.e.y - lineBody.s.y;
        double lineX2 = t1.p.x - lineBody.s.x;
        double lineY2 = t1.p.y - lineBody.s.y;

        double edgeLength = lineX1 * lineX1 + lineY1 * lineY1;
        double dotProd = lineX1 * lineX2 + lineY1 * lineY2;
        double t = std::max(0.0, std::min(edgeLength, dotProd)) / edgeLength;

        // find the closest point on the line to the circle and the distance to it
        Vec2 closestPoint(lineBody.s.x + t * lineX1, lineBody.s.y + t * lineY1);
        double distance = closestPoint.dist(t1.p);

        if (distance == 0)
            // AV: Avoid division by zero below.
            distance = 1;

        // pretend the closest point on the line is a circle and check collision
        // calculate the overlap between the circle and that fake circle
        double overlap = b1.r - distance;
        if (treatAsCone) {
            // The effective radius of this lineBody varies depending on the point of contact 
            // from a max of r at the "fat" end to a minimum of 0.
            overlap += (1 - t) * lineBody.r;
        } else {
            overlap += lineBody.r;
        }

        // if the circle and the line overlap
        if (overlap > m_overlapThreshold)
        {
            // create a fake circlebody to handle physics
            m_fakeBodies.emplace_back(CCircleBody(b1.r));
            m_fakeTransforms.emplace_back(CTransform(closestPoint));
            m_fakeTransforms.back().v = t1.v * -1.0;

            // add a collision between the circle and the fake circle
            // this will later be resolved in the dynamic collision resolution
            m_collisions.push_back({ &t1, &m_fakeTransforms.back(), &b1, &m_fakeBodies.back() });

            // resolve the static collision by pushing circle away from line
            // lines assume infinite mass and do not get moved
            t1.p.x += overlap * (t1.p.x - m_fakeTransforms.back().p.x) / distance;
            t1.p.y += overlap * (t1.p.y - m_fakeTransforms.back().p.y) / distance;
            b1.collided = true;

            return true;
        }

        return false;
    }

    void appendTo(std::vector<Entity> & src, std::vector<Entity> & dest)
    {
        dest.insert(dest.end(), src.begin(), src.end());
    }

public:

    Simulator(std::shared_ptr<World> world)
        : m_world(world)
    {
        m_collisions.reserve(MaxEntities);
        m_fakeBodies.reserve(MaxEntities);
        m_fakeTransforms.reserve(MaxEntities);
        m_collisionEntities.reserve(MaxEntities);
    }

    void update(double timeStep = 1.0)
    {
        m_timeStep = timeStep;

        // update the world so entities get managed
        m_world->update();

        // populate the vector of entities we care about colliding
        m_collisionEntities.clear();
        appendTo(m_world->getEntities("robot"), m_collisionEntities);
//        appendTo(m_world->getEntities("puck"), m_collisionEntities);
appendTo(m_world->getEntities("red_puck"), m_collisionEntities);
appendTo(m_world->getEntities("green_puck"), m_collisionEntities);

        // do the actual simulation
        movement();
        collisions();
    }

    // TODO: remove this, make sim world only on constructor
    void setWorld(const std::shared_ptr<World> world)
    {
        m_world = world;
        m_collisions.clear();
        m_fakeTransforms.clear();
        m_fakeBodies.clear();
        m_collisionEntities.clear();
    }

    std::vector<CollisionData> & getCollisions()
    {
        return m_collisions;
    }

    double getComputeTime() const
    {
        return m_computeTime;
    }

    double getComputeTimeMax() const
    {
        return m_computeTimeMax;
    }

    std::shared_ptr<World> getWorld()
    {
        return m_world;
    }
};