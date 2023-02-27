#pragma once

#include <sstream>

#include "Entity.hpp"
#include "Sensors.hpp"
#include "World.hpp"

struct SensorReading {
    double gridForward0 = 0;
    double gridCentre0 = 0;
    double gridRight0 = 0;
    double robotAheadFar = 0;
    double robotAheadClose = 0;

    std::string toString()
    {
        std::stringstream ss;

        //ss << "gridForward0, gridCentre0, gridRight0: " << gridForward0 << "\t"
        //   << gridCentre0 << "\t" << gridRight0 << "\n";
        ss << "robotAheadFar: " << robotAheadFar << "\n";
        ss << "robotAheadClose: " << robotAheadClose << "\n";

        return ss.str();
    }
};

namespace SensorTools {

inline void ReadSensorArray(Entity e, std::shared_ptr<World> world, SensorReading& reading)
{
    reading = {};

    auto& sensors = e.getComponent<CSensorArray>();

    /*
    for (auto& sensor : sensors.gridSensors) {
        if (sensor->name() == "gridCentre0")
            reading.gridCentre0 = sensor->getReading(world);
        else if (sensor->name() == "gridForward0")
            reading.gridForward0 = sensor->getReading(world);
        else if (sensor->name() == "gridRight0")
            reading.gridRight0 = sensor->getReading(world);
    }
    */

    for (auto& sensor : sensors.robotSensors) {
        if (sensor->name() == "robotAheadFar")
            reading.robotAheadFar = sensor->getReading(world);
        if (sensor->name() == "robotAheadClose")
            reading.robotAheadClose = sensor->getReading(world);
    }
}

}