#pragma once

#include <string>
#include <fstream>
#include <iostream>

struct Config
{
    size_t gui          = 1;
    size_t numRobots    = 20;
    size_t fakeRobots    = 0;
    double robotRadius  = 10.0;

    double plowLength   = 60.0;
    double plowAngleDeg   = 0.0;

    size_t numPucks     = 0;
    double puckRadius   = 10.0;

    std::string arenaConfig = "";

    size_t controllerSkip = 0;

    // Simulation Parameters
    double simTimeStep  = 1.0;
    double renderSteps  = 1;
    size_t maxTimeSteps = 0;

    size_t writeDataSkip    = 0;
    std::string dataFilenameBase   = "";
    size_t numTrials = 10;
    std::string evalName = "";

    size_t captureScreenshots          = 0;
    std::string screenshotFilenameBase   = "";

    double maxForwardSpeed = 2;
    double maxAngularSpeed = 0.05; // 2.0; // 0.5
    double robotSensingDistance = 100000;
    double puckSensingDistance = 100000;
    double goalX         = 0;
    double goalY         = 0;
    double sensorNoise         = 0;
    size_t controllerState         = 0;
    double filterConstant         = 0;
    size_t controllerBlindness         = 0;

    size_t escapeDuration         = 0;

    size_t arenaSweep         = 0;
    size_t paramSweep         = 0;

    Config() {}

    Config(const std::string & filename) {
        load(filename);
    }

    void load(const std::string & filename)
    {
        std::ifstream fin(filename);
        if (fin.fail())
            std::cerr << "Problem opening file: " << filename << std::endl;
        std::string token;
        double tempVal = 0;
 
        while (fin.good())
        {
            fin >> token;
            if (token == "numRobots")      { fin >> numRobots; }
            else if (token == "fakeRobots")    { fin >> fakeRobots; }
            else if (token == "robotRadius")    { fin >> robotRadius; }
            else if (token == "plowLength")    { fin >> plowLength; }
            else if (token == "plowAngleDeg")    { fin >> plowAngleDeg; }
            else if (token == "gui")            { fin >> gui; }
            else if (token == "numPucks")       { fin >> numPucks; }
            else if (token == "puckRadius")     { fin >> puckRadius; }
            else if (token == "arenaConfig")       { fin >> arenaConfig; }
            else if (token == "controllerSkip")     { fin >> controllerSkip; }
            else if (token == "simTimeStep")    { fin >> simTimeStep; }
            else if (token == "renderSteps")     { fin >> renderSteps; }
            else if (token == "maxTimeSteps")   { fin >> maxTimeSteps; }
            else if (token == "writeDataSkip")  { fin >> writeDataSkip; }
            else if (token == "dataFilenameBase")   { fin >> dataFilenameBase; }
            else if (token == "numTrials")   { fin >> numTrials; }
            else if (token == "evalName")   { fin >> evalName; }
            else if (token == "captureScreenshots")   { fin >> captureScreenshots; }
            else if (token == "screenshotFilenameBase")   { fin >> screenshotFilenameBase; }
            else if (token == "maxForwardSpeed")             { fin >> maxForwardSpeed; }
            else if (token == "maxAngularSpeed")        { fin >> maxAngularSpeed; }
            else if (token == "robotSensingDistance")   { fin >> robotSensingDistance; }
            else if (token == "puckSensingDistance")    { fin >> puckSensingDistance; }
            else if (token == "goalX")     { fin >> goalX; }
            else if (token == "goalY")     { fin >> goalY; }
            else if (token == "sensorNoise")     { fin >> sensorNoise; }
            else if (token == "controllerState")     { fin >> controllerState; }
            else if (token == "filterConstant")     { fin >> filterConstant; }
            else if (token == "controllerBlindness")     { fin >> controllerBlindness; }
            else if (token == "escapeDuration")     { fin >> escapeDuration; }
            else if (token == "arenaSweep")     { fin >> arenaSweep; }
            else if (token == "paramSweep")     { fin >> paramSweep; }
        }
    }
};