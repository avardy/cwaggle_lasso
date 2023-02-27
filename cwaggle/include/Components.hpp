#pragma once

#include <SFML/Graphics.hpp>
#include <bitset>
#include <array>
#include <memory>

#include "Vec2.hpp"

using std::string;

class CTransform
{
public:
    
    Vec2 p = { 0.0, 0.0 };
    Vec2 v = { 0.0, 0.0 };
    Vec2 a = { 0.0, 0.0 };
    bool moved = false;

    CTransform() {}
    CTransform(const Vec2 & pin) : p(pin) {}
};

class CCircleBody
{
public:
    double r = 10;
    double m = 0;
    bool slowAfterCollision = false;
    bool collided = true;

    CCircleBody() {}
    CCircleBody(double radius)
        : r(radius), m(radius * 10) { }
    CCircleBody(double radius, bool inSlowAfterCollision)
        : r(radius), m(radius * 10), slowAfterCollision(inSlowAfterCollision) { }
};

class CCircleShape
{
public:
    sf::CircleShape shape;
    CCircleShape() {}
    CCircleShape(double radius)
        : shape((float)radius, 32)
    {
        shape.setOrigin((float)radius, (float)radius);
    }
};

//class GridSensor;
class RobotSensor;
class CSensorArray
{
public:
    //std::vector<std::shared_ptr<GridSensor>>     gridSensors;
    std::vector<std::shared_ptr<RobotSensor>>     robotSensors;
    CSensorArray() {}
};

class CLineBody
{
public:
    Vec2 s;
    Vec2 e;
    double r = 1.0;

    CLineBody() {}

    CLineBody(Vec2 start, Vec2 end, double radius)
        : s(start), e(end), r(radius) { }
};


class CRobotType
{
public:
    size_t type = 0;
    CRobotType(size_t t = 0)
        : type(t) {}
};

class CSteer
{
public:
    double angle = 0;
    double angularSpeed = 0;
    double speed = 0;
    int slowedCount = 0;
    bool frozen = false;
    CSteer() {}
};

class CColor
{
public:
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
    CColor() {}
    CColor(int rr, int gg, int bb, int aa)
        : r((uint8_t)rr), g((uint8_t)gg), b((uint8_t)bb), a((uint8_t)aa) {}
};

// Used to hold on to visualization-related info from a robot's controller.
class CControllerVis
{
public:
    string msg;
    bool selected = false;

    CControllerVis() {}
    CControllerVis(string m)
        : msg(m)
    {}
};

// Used to draw a vector from a robot (or potentially other entity)
class CVectorIndicator
{
public:
    double angle, length;

    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
    CVectorIndicator() {}
    CVectorIndicator(double a, double l, int rr, int gg, int bb, int aa)
        : angle(a), length(l), r((uint8_t)rr), g((uint8_t)gg), b((uint8_t)bb), a((uint8_t)aa) { }
};

// Simulates a triangular plow attached to the front of a robot.
class CPlowBody
{
public:
    double length, angle, startLength, width;
    sf::ConvexShape shape;

    CPlowBody() {}
    CPlowBody(double l, double cbRadius, double angleDeg)
        : length(l)
        , angle(angleDeg * M_PI / 180.0)
        , shape()
    {
        startLength = cbRadius*cbRadius / length;
        // This is right, but seems to cause problems getting pucks away from borders/corners.
        width = 2 * cbRadius * sqrt(length*length - cbRadius*cbRadius)/length;
//        width = 0.9 * 2 * cbRadius * sqrt(length*length - cbRadius*cbRadius)/length;

        shape.setPointCount(3);

        shape.setPoint(0, sf::Vector2f(startLength, -width/2.0f));
        shape.setPoint(1, sf::Vector2f(length, 0));
        shape.setPoint(2, sf::Vector2f(startLength, width/2.0f));
    }
};

class CTerritory
{
public:
    Vec2 centre{ 0.0, 0.0 };
    double radius = 0;
    sf::CircleShape shape;
    sf::Color color;

    CTerritory() {}
    CTerritory(const Vec2 & c, double r, int red=255, int green=255, int blue=255, int alpha=255) 
        : centre(c)
        , radius(r)
        , shape((float)radius, 32)
        , color(red, green, blue, alpha)
    {
        shape.setOrigin((float)radius, (float)radius);
    }
};

class EntityController;
class CController
{
public:
    std::shared_ptr<EntityController> controller;
    CController() {}
    CController(decltype(controller) c)
        : controller(c) {}
};
