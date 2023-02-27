#pragma once

#include "EntityControllers.hpp"
#include "World.hpp"

namespace WorldUtils {

/**
 * Add line bodies along the world's boundaries.
 * 
 * @param   world          world to add the line bodies to
 */
void AddBoundaryLines(std::shared_ptr<World> world)
{
    size_t width = world->width();
    size_t height = world->height();

    Entity topWall = world->addEntity("line");
    Entity bottomWall = world->addEntity("line");
    Entity leftWall = world->addEntity("line");
    Entity rightWall = world->addEntity("line");

    // Thickness (divided by 2) of the walls which are positioned outside the visible area
    double t = width / 5.0;
    topWall.addComponent<CLineBody>(Vec2(-t, -t + 1), Vec2(width - 1 + t, -t + 1), t);
    bottomWall.addComponent<CLineBody>(Vec2(-t, height - 1 + t), Vec2(width - 1 + t, height - 1 + t), t);
    leftWall.addComponent<CLineBody>(Vec2(-t, -t), Vec2(-t, height - 1 + t), t);
    rightWall.addComponent<CLineBody>(Vec2(width - 1 + t, -t), Vec2(width - 1 + t, height - 1 + t), t);
}

/**
 * Add line bodies to the corners of the given world.
 * 
 * @param   world          world to add the line bodies to
 * @param   length         length of corner pieces
 */
void AddCornerLines(std::shared_ptr<World> world, double length)
{
    size_t width = world->width();
    size_t height = world->height();
    double t = length / sqrt(2.0);
    Entity topLeftCorner = world->addEntity("line");
    Entity botLeftCorner = world->addEntity("line");
    Entity topRightCorner = world->addEntity("line");
    Entity botRightCorner = world->addEntity("line");
    topLeftCorner.addComponent<CLineBody>(Vec2(-length, 2 * length), Vec2(2 * length, -length), t);
    botLeftCorner.addComponent<CLineBody>(Vec2(-length, height - 2 * length), Vec2(2 * length, height + length), t);
    topRightCorner.addComponent<CLineBody>(Vec2(width + length, 2 * length), Vec2(width - 2 * length, -length), t);
    botRightCorner.addComponent<CLineBody>(Vec2(width + length, height - 2 * length), Vec2(width - 2 * length, height + length), t);
}

/**
 * Add to the given world, an arc of line bodies which approximate the
 * arc of a circle.  The given parameters specify the *inside* of the arc.
 * Each line body will have the given thickness, grown outwards from this arc.
 * 
 * @param   world          world to add the line bodies to
 * @param   n              number of line bodies to create
 * @param   cx             x-coordinate of circle centre
 * @param   cy             y-coordinate of circle centre
 * @param   radius         circle radius
 * @param   startAngle     starting angle for the arc
 * @param   finalAngle     final angle for the arc
 * @param   thickness      thickness of the line bodies
 */
void AddLineBodyArc(std::shared_ptr<World> world, int n, double cx, double cy, double radius, double startAngle, double finalAngle, double thickness)
{
    // Adjusting so that the line bodies' thickness extends outwards from the
    // arc.  In other words, the arc doesn't specify the centre of the line
    // bodies, but their inside edge instead.
    radius += thickness;

    double lastX = cx + radius * cos(startAngle);
    double lastY = cy + radius * sin(startAngle);
    double deltaAngle = (finalAngle - startAngle) / n;

    for (int i=0; i<n; i++) {
        double x = cx + radius * cos(startAngle + (i+1) * deltaAngle);
        double y = cy + radius * sin(startAngle + (i+1) * deltaAngle);

        Entity line = world->addEntity("line");
        line.addComponent<CLineBody>(Vec2(lastX, lastY), Vec2(x, y), thickness);
        lastX = x;
        lastY = y;
    }

    world->update();
}

};