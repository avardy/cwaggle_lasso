#pragma once

//#include "Components.hpp"
#include "EntityControllers.hpp"
#include "Simulator.hpp"
#include "ValueGrid.hpp"
#include "World.hpp"
#include "WorldUtils.hpp"
#include "Intersect.h"

#include "Config.hpp"

#include <sstream>

using namespace std;

namespace lasso_world {

/**
 * If the given position is within the given radius of any lines, return false.  Otherwise
 * we assume the position is okay to occupy and return true.
 */
bool checkPosition(std::shared_ptr<World> world, Vec2 p, double radius)
{
    for (auto lineEntity : world->getEntities("line")) {
        auto & line = lineEntity.getComponent<CLineBody>();
        if (Intersect::checkCircleSegmentIntersection(line.s, line.e, p, line.r + radius))
            return false;
    }
    return true;
}

Entity addRobot(std::shared_ptr<World> world, Config config)
{
    Entity robot = world->addEntity("robot");

    // This position will later be overwritten.
    Vec2 rPos(0, 0);
    robot.addComponent<CTransform>(rPos);
    if (!config.fakeRobots) {
        robot.addComponent<CCircleBody>(config.robotRadius, true);
        robot.addComponent<CCircleShape>(config.robotRadius);
        robot.addComponent<CColor>(50, 50, 100, 200);
    }

    // Grid sensors at three points with the most forward point at the tip of the plow.
    double plowAngleRad = config.plowAngleDeg * M_PI / 180.0;

    // Wedge shaped plow at the front of the robot.
    if (config.plowLength > 0 && !config.fakeRobots)
//        robot.addComponent<CPlowBody>(config.plowLength, config.robotRadius, config.plowAngleDeg);
        robot.addComponent<CPlowBody>(config.plowLength, config.robotRadius + 5, config.plowAngleDeg);

    auto& sensors = robot.addComponent<CSensorArray>();

    //sensors.robotSensors.push_back(make_shared<RobotSensor>(robot, "robotAheadClose", config.plowAngleDeg, 0.5*config.robotRadius, 1.5*config.robotRadius));
    sensors.robotSensors.push_back(make_shared<RobotSensor>(robot, "robotAheadClose", config.plowAngleDeg, 0.5*config.robotRadius, config.robotRadius));

    sensors.robotSensors.push_back(make_shared<RobotSensor>(robot, "robotAheadFar", config.plowAngleDeg, 2*config.robotRadius, config.robotRadius));

    return robot;
}

void addRobots(shared_ptr<World> world, uniform_int_distribution<int> Xrng, uniform_int_distribution<int> Yrng, 
               uniform_real_distribution<double> angleRNG, default_random_engine rng, Config config)
{
    int numRobots = (int)config.numRobots;
    for (size_t r = 0; r < numRobots; r++) {
        Entity robot = addRobot(world, config);
        auto& transform = robot.getComponent<CTransform>();

        do {
            transform.p.x = config.robotRadius + Xrng(rng);
            transform.p.y = config.robotRadius + Yrng(rng);
        } while (!checkPosition(world, transform.p, config.robotRadius));

        robot.addComponent<CSteer>();
        auto & steer = robot.getComponent<CSteer>();
        steer.angle = angleRNG(rng);
    }
}


Entity addPuck(string name, size_t red, size_t green, size_t blue, shared_ptr<World> world, size_t puckRadius)
{
    Entity puck = world->addEntity(name);

    // This position will later be overwritten.
    Vec2 pPos(0, 0);
    puck.addComponent<CTransform>(pPos);
    puck.addComponent<CCircleBody>(puckRadius);
    puck.addComponent<CCircleShape>(puckRadius);
    puck.addComponent<CColor>(red, green, blue, 255);

    return puck;
}

void addPucks(string name, size_t red, size_t green, size_t blue, shared_ptr<World> world, uniform_int_distribution<int> Xrng, uniform_int_distribution<int> Yrng, default_random_engine rng,
    Config config)
{
    int puckRadius = (int)config.puckRadius;
    for (size_t r = 0; r < config.numPucks; r++) {
        Entity puck = addPuck(name, red, green, blue, world, puckRadius);

        auto& transform = puck.getComponent<CTransform>();
        do {
            transform.p.x = config.puckRadius + Xrng(rng);
            transform.p.y = config.puckRadius + Yrng(rng);
        } while (!checkPosition(world, transform.p, config.robotRadius));

        //std::cerr << puck.id() << ", x, y: " << transform.p.x << ", " << transform.p.y << std::endl;
    }
}

// A probe is a robot without a body or a controller.
Entity addProbe(std::shared_ptr<World> world, Config config)
{
    Entity probe = world->addEntity("probe");

    // This position will later be overwritten.
    Vec2 rPos(world->width()/2, world->height()/2);
    probe.addComponent<CTransform>(rPos);
    probe.addComponent<CCircleBody>(5, true);
    probe.addComponent<CCircleShape>(5);
    probe.addComponent<CColor>(100, 100, 255, 100);

    return probe;
}

void addFakeRobots(shared_ptr<World> world, Config config)
{
    double w = world->width();
    double h = world->height();
    double gap = 40;
    double nAngles = 16;
    double dFromCentre = 0;

    for (double y = gap/2; y < h; y += gap) {
        for (double x = gap/2; x < w; x += gap) {

            // Position (x, y) is used as a centre, but for each angle we push a little in that
            // direction to arrive at (u, v) the robot centre for that angle.
            for (double angle = 0; angle < 2*M_PI; angle += (2*M_PI) / (nAngles)) {
                double u = x + dFromCentre * cos(angle);
                double v = y + dFromCentre * sin(angle);

                if (checkPosition(world, Vec2{u, v}, 0)) {
                    Entity robot = addRobot(world, config);
                    auto& transform = robot.getComponent<CTransform>();
                    transform.p.x = u;
                    transform.p.y = v;

                    robot.addComponent<CSteer>();
                    auto & steer = robot.getComponent<CSteer>();
                    steer.angle = angle;
                }
            }
        }
    }
}

shared_ptr<World> GetWorld(default_random_engine rng, Config config)
{
    string grid0Filename, grid1Filename;
    if (config.arenaConfig == "sim_stadium_no_wall") {
        grid0Filename = "../../images/sim_stadium_no_wall/travel_time.png";
        grid1Filename = "../../images/sim_stadium_no_wall/start_bar.png";
    } else if (config.arenaConfig == "sim_stadium_one_wall") {
        grid0Filename = "../../images/sim_stadium_one_wall/travel_time.png";
        grid1Filename = "../../images/sim_stadium_one_wall/start_bar.png";
    } else if (config.arenaConfig == "sim_stadium_one_wall_double") {
        grid0Filename = "../../images/sim_stadium_one_wall_double/travel_time.png";
        grid1Filename = "../../images/sim_stadium_one_wall_double/start_bar.png";
    } else if (config.arenaConfig == "sim_stadium_two_walls") {
        grid0Filename = "../../images/sim_stadium_two_walls/travel_time.png";
        grid1Filename = "../../images/sim_stadium_two_walls/start_bar.png";
    } else if (config.arenaConfig == "sim_stadium_three_walls") {
        grid0Filename = "../../images/sim_stadium_three_walls/travel_time.png";
        grid1Filename = "../../images/sim_stadium_three_walls/start_bar.png";
    } else if (config.arenaConfig == "live_no_wall") {
        grid0Filename = "../../images/live_no_wall/travel_time.png";
        grid1Filename = "../../images/live_no_wall/start_bar.png";
    } else if (config.arenaConfig == "live_one_wall") {
        grid0Filename = "../../images/live_one_wall/travel_time.png";
        grid1Filename = "../../images/live_one_wall/start_bar.png";
    } else {
        cerr << "worlds.hpp: Unknown setting for arenaConfig: " << config.arenaConfig << endl;
        exit(1);
    }

    ValueGrid valueGrid0(grid0Filename, 1.0);
    ValueGrid valueGrid1(grid1Filename, 0.0);

    size_t width = valueGrid0.width();
    size_t height = valueGrid0.height();

    auto world = std::make_shared<World>(width, height);

    // Add the two-ends to form the stadium shape.  Note that we don't do this for the live configuration
    // because the physical table plays the role of confining both pucks and robots.
    if (config.arenaConfig == "sim_stadium_no_wall" || config.arenaConfig == "sim_stadium_one_wall" || config.arenaConfig == "sim_stadium_one_wall_double"
        || config.arenaConfig == "sim_stadium_two_walls" || config.arenaConfig == "sim_stadium_three_walls") {
        // Create the left and right arcs to match our stadium-shaped air hockey table
        WorldUtils::AddLineBodyArc(world, 64, width/3, height/2, height/2, -3*M_PI/2, -M_PI/2, 100);
        WorldUtils::AddLineBodyArc(world, 64, 2*width/3, height/2, height/2, -M_PI/2, M_PI/2, 100);

        // Create top and bottom line bodies.  These are not really needed for
        // the simulation aspect, but moreso for visualization.
        Entity topWall = world->addEntity("line");
        Entity botWall = world->addEntity("line");
        topWall.addComponent<CLineBody>(Vec2(0, 0), Vec2(width-1, 0), 1);
        botWall.addComponent<CLineBody>(Vec2(0, height), Vec2(width-1, height), 1);
    }

    // Whenever we add a wall here, we actually create two different entites:
    //  line - Used for regular collisions.
    //  visibility_line - Used only to check for visibility from one part of the arena to another
    //
    if (config.arenaConfig == "live_one_wall") {
        Entity wall = world->addEntity("line");
        wall.addComponent<CLineBody>(Vec2(606, 0), Vec2(606, 404), 25);

        Entity vWall = world->addEntity("visibility_line");
        vWall.addComponent<CLineBody>(wall.getComponent<CLineBody>());

    }
    if (config.arenaConfig == "sim_stadium_one_wall" || config.arenaConfig == "sim_stadium_one_wall_double" ||
        config.arenaConfig == "sim_stadium_two_walls" || config.arenaConfig == "sim_stadium_three_walls" ) {
        Entity wall = world->addEntity("line");
        if (config.arenaConfig == "sim_stadium_one_wall_double")
            // Double-thickness wall
            wall.addComponent<CLineBody>(Vec2(width/2, 0), Vec2(width/2, 0.625*height), 32);
        else
            wall.addComponent<CLineBody>(Vec2(width/2, 0), Vec2(width/2, 0.625*height), 16);

        Entity vWall = world->addEntity("visibility_line");
        vWall.addComponent<CLineBody>(wall.getComponent<CLineBody>());
    }
    if (config.arenaConfig == "sim_stadium_two_walls" || config.arenaConfig == "sim_stadium_three_walls" ) {
        Entity wall = world->addEntity("line");
        wall.addComponent<CLineBody>(Vec2(3*width/4, height), Vec2(3*width/4, 0.4*height), 16);

        Entity vWall = world->addEntity("visibility_line");
        vWall.addComponent<CLineBody>(wall.getComponent<CLineBody>());
    }
    if (config.arenaConfig == "sim_stadium_three_walls" ) {
        Entity wall = world->addEntity("line");
        wall.addComponent<CLineBody>(Vec2(0, 0.6*height), Vec2(width/4, 0.6*height), 16);

        Entity vWall = world->addEntity("visibility_line");
        vWall.addComponent<CLineBody>(wall.getComponent<CLineBody>());
    }


    world->update();

    if (config.fakeRobots)
        addFakeRobots(world, config);

    // Prepare to generate random x and y positions for robots.
    int randXDomain = world->width() - 2 * (int)config.robotRadius;
    int randYDomain = world->height() - 2 * (int)config.robotRadius;
    uniform_int_distribution<int> robotXrng(0, randXDomain);
    uniform_int_distribution<int> robotYrng(0, randYDomain);

    // Prepare to generate robot angles in radians
    uniform_real_distribution<double> robotAngleRNG(-M_PI, M_PI);

    addRobots(world, robotXrng, robotYrng, robotAngleRNG, rng, config);

    // Prepare to generate random x and y positions for pucks.
    // Random number generators for x- and y- position.
    randXDomain = world->width() - 2 * (int)config.puckRadius;
    randYDomain = world->height() - 2 * (int)config.puckRadius;
    uniform_int_distribution<int> puckXrng(0, randXDomain);
    uniform_int_distribution<int> puckYrng(0, randYDomain);

    addPucks("red_puck", 200, 44, 44, world, puckXrng, puckYrng, rng, config);
    //addPucks("green_puck", 44, 200, 44, world, puckXrng, puckYrng, rng, config);

    //addProbe(world, config);

    world->addGrid(valueGrid0);
    world->addGrid(valueGrid1);

    // Four grids (red, green, blue and white) for visualization.
    world->addGrid(ValueGrid{width, height, 0, 0});
    world->addGrid(ValueGrid{width, height, 0, 0});
    world->addGrid(ValueGrid{width, height, 0, 0});
    world->addGrid(ValueGrid{width, height, 0, 0});

    world->update();
    return world;
}

};
