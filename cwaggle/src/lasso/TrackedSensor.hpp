#pragma once

#include <float.h>

#include "World.hpp"
#include "Entity.hpp"
#include "Angles.h"
#include "Intersect.h"

enum class SensorOp { GET_MIN_DTG, GET_MAX_DTG };

/**
 * A TrackedSensor senses objects according to their position on the two scalar fields:
 * distance-to-goal (DTG) and distance-along-track (DAT).
 */
class TrackedSensor
{
    Entity m_robot;
    default_random_engine m_rng;
    uniform_real_distribution<double> m_sensorNoiseDist;
    size_t m_toGoalGridIndex = 0;
    size_t m_alongTrackGridIndex = 1;
    ValueGrid *m_toGoalGrid = nullptr;
    ValueGrid *m_alongTrackGrid = nullptr;

public:

    TrackedSensor(Entity robot, default_random_engine &rng, Config &config)
        : m_robot(robot), m_rng(rng), m_sensorNoiseDist(-config.sensorNoise, config.sensorNoise)
    {
    }

    void getDTGExtent(std::shared_ptr<World> world, Entity robot, double &minDTG, double &maxDTG, double &maxDTGlessThanOne)
    {
        if (m_toGoalGrid == nullptr) {
            m_toGoalGrid = &world->getGrid(m_toGoalGridIndex);
            m_alongTrackGrid = &world->getGrid(m_alongTrackGridIndex);
        }

        bool vis = robot.getComponent<CControllerVis>().selected;
        vector<Vec2> samples = samplePerimeter(world, robot, false, vis);

        // Keep track of the minimum and maximum values found so far.
        minDTG = DBL_MAX;
        maxDTG = -DBL_MAX;
        maxDTGlessThanOne = -DBL_MAX;
        for (Vec2 pos : samples) {
            size_t gX = (size_t) round(pos.x);
            size_t gY = (size_t) round(pos.y);
            double v = m_toGoalGrid->get(gX, gY);
            if (v > maxDTG) maxDTG = v;
            if (v < minDTG) minDTG = v;
            if (v < 1 && v > maxDTGlessThanOne) maxDTGlessThanOne = v;
        }
    }

    /**
     * Get the most extreme (largest or smallest, depending on getMax) DTG value for the given type of object
     * which is within the given difference in DAT values of the robot.
     */
    inline virtual double getExtreme(std::shared_ptr<World> world, Entity robot, string objectType, SensorOp op, 
                                        double minDTG, double maxDTG, double senseRadius, bool &valid,
                                        double minRelativeDAT = -1, double maxRelativeDAT = 1)
    {
        if (m_toGoalGrid == nullptr) {
            m_toGoalGrid = &world->getGrid(m_toGoalGridIndex);
            m_alongTrackGrid = &world->getGrid(m_alongTrackGridIndex);
        }

        bool vis = robot.getComponent<CControllerVis>().selected;

        Vec2 robotPos = robot.getComponent<CTransform>().p;
        double robotAngle = robot.getComponent<CSteer>().angle;

        // Keep track of the minimum and maximum values found so far.
        double result;
        if (op == SensorOp::GET_MAX_DTG)
            result = 0;
        else if (op == SensorOp::GET_MIN_DTG)
            result = 1;
        else
            cerr << "TrackedSensor: Unsupported Op" << endl;

        valid = false;
        for (auto e : world->getEntities(objectType)) {
            if (robot.id() == e.id()) { continue; }

vector<Vec2> samples = samplePerimeter(world, e, objectType == "robot", vis);
//            vector<Vec2> samples = samplePerimeter(world, e, false, vis);

            for (Vec2 pos : samples) {
                if (!checkVisibility(world, robotPos, robotAngle, pos, minDTG, maxDTG, senseRadius))
                    continue;

                size_t gX = (size_t) round(pos.x);
                size_t gY = (size_t) round(pos.y);
                double v = m_toGoalGrid->get(gX, gY);

                // So we perceive this point... 
                // ...but is it the highest/lowest?
                if (op == SensorOp::GET_MAX_DTG && v > result) {
                    result = v;
                    valid = true;
                }
                if (op == SensorOp::GET_MIN_DTG && v < result) {
                    result = v;
                    valid = true;
                }
            }
        }

        // Vis-only
        if (vis) {
            int visGridIndex = 2;
            if (objectType == "robot")
                visGridIndex = 3;
            auto & visGrid = world->getGrid(visGridIndex);

/*
            double visRegionIntensity = 0.25;
            int w = visGrid.width();
            int h = visGrid.height();
            for (int j=0; j<h; ++j)
                for (int i=0; i<w; ++i)
                    if (checkVisibility(world, robotPos, Vec2(i, j), minDTG, maxDTG, senseRadius))
                        visGrid.set(i, j, visRegionIntensity);
*/

            if (op == SensorOp::GET_MAX_DTG) {
                visGrid.addContour(result, *m_toGoalGrid, 1.0);
            } else if (op == SensorOp::GET_MIN_DTG) {
                visGrid.addContour(result, *m_toGoalGrid, 0.5);
            } else {
                cerr << "TrackedSensor: Unsupported Op" << endl;
            }
        }

        return result;
    }

    inline virtual Vec2 getTargetPointFromCircle(std::shared_ptr<World> world, Entity robot, double targetDTG, bool &targetValid)
    {
        if (m_toGoalGrid == nullptr) {
            m_toGoalGrid = &world->getGrid(m_toGoalGridIndex);
            m_alongTrackGrid = &world->getGrid(m_alongTrackGridIndex);
        }

        Vec2 robotPos = robot.getComponent<CTransform>().p;
        double robotAngle = robot.getComponent<CSteer>().angle;

        // Generate sample positions in a circle around the robot's current position.
        auto & pb = m_robot.getComponent<CPlowBody>();
        auto & steer = m_robot.getComponent<CSteer>();
        double radius = 24;
        int nSamples = 8;
//        double fieldOfView = 2 * M_PI;
        double fieldOfView = M_PI/2.0;
        vector<Vec2> samples;
        for (int i=0; i<nSamples; ++i) {
            double angle = i * fieldOfView / nSamples - 0.5 * fieldOfView + robotAngle;
            Vec2 s{robotPos.x + radius * cos(angle), robotPos.y + radius * sin(angle)};
            samples.push_back(s);
        }

        targetValid = false;
        Vec2 target;
        double smallestAbsDiff = DBL_MAX;
        double lastV = 0;
        for (Vec2 sample : samples) {
            size_t gx = (size_t) round(sample.x);
            size_t gy = (size_t) round(sample.y);
            double v = m_toGoalGrid->get(gx, gy);

            if (v < lastV && lastV < 1) {
                double absDiff = abs(targetDTG - v);

                if (absDiff < smallestAbsDiff) {
                    smallestAbsDiff = absDiff;
                    target.x = sample.x;
                    target.y = sample.y;
                    targetValid = true;
                }
            }

            lastV = v;
        }

        bool vis = robot.getComponent<CControllerVis>().selected;
        if (vis) {
            auto & visGridWhite = world->getGrid(5);
            auto & visGridGreen = world->getGrid(3);

            for (Vec2 S : samples) {
                int r = 1;
                for (int j=-r; j<=r; ++j)
                    for (int i=-r; i<=r; ++i)
                        visGridGreen.set(S.x + i, S.y + j, 0.5);
            }

            int r = 3;
            for (int j=-r; j<=r; ++j)
                for (int i=-r; i<=r; ++i)
                    visGridWhite.set(target.x + i, target.y + j, 1.0);

            //std::cerr << "target: " << target.x << ", " << target.y << std::endl;
        }

        return target;
    }

    inline virtual Vec2 getTargetPointFromLine(std::shared_ptr<World> world, Entity robot, double targetDTG, bool &borderSensed, bool &validTarget, bool &aheadGreaterThanCentre)
    {
        if (m_toGoalGrid == nullptr) {
            m_toGoalGrid = &world->getGrid(m_toGoalGridIndex);
            m_alongTrackGrid = &world->getGrid(m_alongTrackGridIndex);
        }

        Vec2 robotPos = robot.getComponent<CTransform>().p;
        double robotAngle = robot.getComponent<CSteer>().angle;

        // Generate sample positions in a line ahead of the robot's current position.
        int nSamples = 8;  // Arbitrarily set
        double ahead = 20; // Arbitrarily set
        double width = 30; // Arbitrarily set
        
        // Derived parameters used to define vectors A and B which mark either end of the
        // line to sample.
        double alpha = atan(width / (2.0*ahead));
        double L = hypot(ahead, width / 2.0);
        Vec2 A{L * cos(robotAngle - alpha),
               L * sin(robotAngle - alpha)};
        Vec2 B{L * cos(robotAngle + alpha),
               L * sin(robotAngle + alpha)};

        vector<Vec2> samples;
        for (int i=0; i<nSamples; ++i) {
            double t = i / (nSamples - 1.0);
            Vec2 S = robotPos + A + (B - A) * t;
            samples.push_back(S);
        }

        borderSensed = false;
        validTarget = false;

        Vec2 target;
        double smallestAbsDiff = DBL_MAX;
        double lastV = 0;
        for (Vec2 sample : samples) {
            size_t gx = (size_t) round(sample.x);
            size_t gy = (size_t) round(sample.y);
            double v = m_toGoalGrid->get(gx, gy);

            if (v == 1)
                borderSensed = true;

            if (v < lastV && lastV < 1) {
                double absDiff = abs(targetDTG - v);
                if (absDiff < smallestAbsDiff) {
                    smallestAbsDiff = absDiff;
                    target.x = sample.x;
                    target.y = sample.y;
                    validTarget = true;
                }
            }

            lastV = v;
        }

        // Compare a sample straight ahead with a sample at the centre of robot.
        size_t aheadX = (size_t) round(robotPos.x + ahead * cos(robotAngle));
        size_t aheadY = (size_t) round(robotPos.y + ahead * sin(robotAngle));
        double aheadV = m_toGoalGrid->get(aheadX, aheadY);

        size_t centreX = (size_t) round(robotPos.x);
        size_t centreY = (size_t) round(robotPos.y);
        double centreV = m_toGoalGrid->get(centreX, centreY);

        aheadGreaterThanCentre = aheadV > centreV;

        bool vis = robot.getComponent<CControllerVis>().selected;
        if (vis) {
            auto & visGridWhite = world->getGrid(5);
            auto & visGridGreen = world->getGrid(3);

            for (Vec2 S : samples) {
                int r = 1;
                for (int j=-r; j<=r; ++j)
                    for (int i=-r; i<=r; ++i)
                        visGridGreen.set(S.x + i, S.y + j, 0.5);
            }

            int r = 3;
            for (int j=-r; j<=r; ++j)
                for (int i=-r; i<=r; ++i)
                    visGridWhite.set(target.x + i, target.y + j, 1.0);

            //std::cerr << "target: " << target.x << ", " << target.y << std::endl;
        }

        return target;
    }

private:

    // Sample positions along the perimeter of the given body.
    inline vector<Vec2> samplePerimeter(std::shared_ptr<World> world, Entity e, bool expandObject, bool vis)
    {
        // Sample positions along the perimeter of this body.  If this object
        // has a plow, add the tip of the plow to the sampled positions.
        // with the tip of its plow.

        Vec2 p = e.getComponent<CTransform>().p;
        auto & cb = e.getComponent<CCircleBody>();
        double radius = cb.r;
        if (expandObject)
            radius *= 2;

        if (e.hasComponent<CPlowBody>() && e.hasComponent<CSteer>()) {
            auto & pb = e.getComponent<CPlowBody>();
            auto & steer = e.getComponent<CSteer>();
//radius = pb.length;
        }

        int nSamples = 16;
        vector<Vec2> samples;
        for (int i=0; i<nSamples; ++i) {
            double angle = i * 2 * M_PI / nSamples;
            Vec2 pos{p.x + radius * cos(angle), p.y + radius * sin(angle)};
            samples.push_back(pos);
        }
        if (e.hasComponent<CPlowBody>() && e.hasComponent<CSteer>()) {
            auto & pb = e.getComponent<CPlowBody>();
            auto & steer = e.getComponent<CSteer>();
            double length = pb.length;
            if (expandObject)
                length += cb.r;
            double xProw = p.x + length * cos(steer.angle + pb.angle);
            double yProw = p.y + length * sin(steer.angle + pb.angle);
            samples.push_back(Vec2{xProw, yProw});
        }

        for (Vec2 &sample : samples) {
            sample.x += m_sensorNoiseDist(m_rng);
            sample.y += m_sensorNoiseDist(m_rng);
        }

        if (vis) {
            auto & visGrid = world->getGrid(2);
            for (Vec2 &sample : samples)
                visGrid.set(round(sample.x), round(sample.y), 1.0);
        }

        return samples;
    }

    inline bool checkVisibility(std::shared_ptr<World> world, Vec2 robotPos, double robotAngle, Vec2 pos,
                         double minDTG, double maxDTG, double senseRadius)
    {
        if (robotPos.dist(pos) > senseRadius)
            return false;

        double angle = Angles::constrainAngle(atan2(pos.y - robotPos.y, pos.x - robotPos.x) - robotAngle);
        // FOR A FORWARD-CENTRED FIELD OF VIEW
        //if (fabs(angle) > M_PI / 2.0)
        //    return false;
        // FOR A FORWARD-CENTRED FIELD OF VIEW, BUT WHICH ONLY INCLUDES THE FORWARD-LEFT QUADRANT
        //if (angle > 0 || angle < -M_PI / 2.0)
        //    return false;

        // Grid position of robot (rx, ry) and the position to test (x, y)
        size_t rx = (size_t) round(robotPos.x);
        size_t ry = (size_t) round(robotPos.y);
        size_t x = (size_t) round(pos.x);
        size_t y = (size_t) round(pos.y);

        double targetDTG = m_toGoalGrid->get(x, y);
        double robotDTG = m_toGoalGrid->get(rx, ry);
        if (robotDTG == 0 || targetDTG == 0 || targetDTG == 1)
            return false;

        if (targetDTG < minDTG || targetDTG > maxDTG)
            return false;

        // Is there a clear line of sight from the robot to this point.  Note that we do not
        // test against all lines, but only against visibility_line's.
        /*
        for (auto lineEntity : world->getEntities("visibility_line")) {
            auto & line = lineEntity.getComponent<CLineBody>();

            if (Intersect::segmentsIntersect(robotPos, pos, line.s, line.e)) {
                return false;
            }
        }
        */

        return true;
    }

    inline bool checkPosOld(std::shared_ptr<World> world, Vec2 robotPos, Vec2 pos,
                         double minDTG, double maxDTG, double maxSensingDistance,
                         double minRelativeDAT = -1, double maxRelativeDAT = 1)
    {
        // Grid position of robot (rx, ry) and the position to test (x, y)
        size_t rx = (size_t) round(robotPos.x);
        size_t ry = (size_t) round(robotPos.y);
        size_t x = (size_t) round(pos.x);
        size_t y = (size_t) round(pos.y);

        if (robotPos.dist(pos) > maxSensingDistance)
            return false;

        double robotDAT = m_alongTrackGrid->get(rx, ry);
        double targetDAT = m_alongTrackGrid->get(x, y);
        double targetDTG = m_toGoalGrid->get(x, y);
        double robotDTG = m_toGoalGrid->get(rx, ry);
        if (robotDTG == 0 || targetDTG == 0 || targetDTG == 1)
            return false;

        double deltaDAT = targetDAT - robotDAT;

        // Check if the shortest distance around a circle is deltaDAT, otherwise
        // correct for this wrapping.  We determine the direction of wrapping
        // and either add or subtract 1.  This is analagous to adding or subtracting
        // 2pi to an angle to bring it within a desired range.
        if (deltaDAT > 0.5)
            deltaDAT -= 1;
        else if (deltaDAT < -0.5)
            deltaDAT += 1;

        if (deltaDAT < minRelativeDAT || deltaDAT > maxRelativeDAT)
            return false;

        if (targetDTG < minDTG || targetDTG > maxDTG)
            return false;

        // Is there a clear line of sight from the robot to this point.  Note that we do not
        // test against all lines, but only against visibility_line's.
/*
        for (auto lineEntity : world->getEntities("visibility_line")) {
            auto & line = lineEntity.getComponent<CLineBody>();

            if (Intersect::segmentsIntersect(robotPos, pos, line.s, line.e)) {
                return false;
            }
        }
*/

        return true;
    }
};