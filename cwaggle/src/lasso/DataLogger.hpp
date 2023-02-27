#pragma once

#include <fstream>
#include <memory>
#include <string>
#include <iostream>

// For mkdir
#include <sys/stat.h>
#include <sys/types.h>

#include "CWaggle.h"

#include "Config.hpp"
#include "LassoController.hpp"

using namespace std;

class DataLogger {
    Config m_config;
    int m_trialIndex;
    ofstream m_statsStream, m_robotPoseStream, m_robotStateStream, m_puckPositionStream;

public:
    DataLogger(Config config, int trialIndex)
        : m_config(config)
        , m_trialIndex(trialIndex)
    {
        //cout << "rngSeed: " << rngSeed << endl;
        if (m_config.writeDataSkip) {

            if (mkdir(config.dataFilenameBase.c_str(), 0777) == -1 && errno != EEXIST)
                cerr << "Error creating directory: " << m_config.dataFilenameBase << endl;

            stringstream statsFilename, robotPoseFilename, robotStateFilename, puckPositionFilename;
            statsFilename << m_config.dataFilenameBase << "/stats_" << trialIndex << ".dat";
            robotPoseFilename << m_config.dataFilenameBase << "/robotPose_" << trialIndex << ".dat";
            robotStateFilename << m_config.dataFilenameBase << "/robotState_" << trialIndex << ".dat";
            puckPositionFilename << m_config.dataFilenameBase << "/puckPosition_" << trialIndex << ".dat";

            m_statsStream = ofstream(statsFilename.str());
            m_robotPoseStream = ofstream(robotPoseFilename.str());
            m_robotStateStream = ofstream(robotStateFilename.str());
            m_puckPositionStream = ofstream(puckPositionFilename.str());
        }
    }

    void writeToFile(shared_ptr<World> world, double stepCount, double eval, double propSlowed, double cumPropSlowed)
    {
        // Compute the average tau and filtered tau values for all robots.
        double avgTau = 0;
        double avgMedianTau = 0;
        double avgFilteredTau = 0;
        double avgState = 0;
        double n = 0;
        for (auto& robot : world->getEntities("robot")) {
            // Ugly!
            std::shared_ptr<LassoController> lassoCtrl = std::dynamic_pointer_cast<LassoController>(robot.getComponent<CController>().controller);
            avgTau += lassoCtrl->m_tau;
            avgMedianTau += lassoCtrl->m_medianTau;
            avgFilteredTau += lassoCtrl->m_filteredTau;
            avgState += lassoCtrl->getStateAsInt();
            ++n;
        }
        if (n > 0) {
            avgTau /= n;
            avgMedianTau /= n;
            avgFilteredTau /= n;
            avgState /= n;
        }

        m_statsStream << stepCount << " " << eval << " " << propSlowed << " " << cumPropSlowed << " " << avgTau << " " << avgMedianTau << " " << avgFilteredTau << " " << avgState << "\n";
        m_statsStream.flush();

        m_robotPoseStream << stepCount;
        m_robotStateStream << stepCount;
        for (auto& robot : world->getEntities("robot")) {
            Vec2& pos = robot.getComponent<CTransform>().p;
            CSteer& steer = robot.getComponent<CSteer>();
            std::shared_ptr<LassoController> lassoCtrl = std::dynamic_pointer_cast<LassoController>(robot.getComponent<CController>().controller);
            m_robotPoseStream << " " << (int)pos.x << " " << (int)pos.y << " " << ((int)(1000 * steer.angle)) / 1000.0; // Rounding angle to 3 decimals
            m_robotStateStream << " " << lassoCtrl->getStateAsInt();
        }
        m_robotPoseStream << "\n";
        m_robotStateStream << "\n";
        m_robotPoseStream.flush();
        m_robotStateStream.flush();

        m_puckPositionStream << stepCount;
        for (auto& puck : world->getEntities("red_puck")) {
            Vec2& pos = puck.getComponent<CTransform>().p;
            m_puckPositionStream << " " << (int)pos.x << " " << (int)pos.y;
        }
        m_puckPositionStream << "\n";
        m_puckPositionStream.flush();
    }

};
