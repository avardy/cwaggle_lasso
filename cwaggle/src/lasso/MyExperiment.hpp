#pragma once

#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>

#include "CWaggle.h"
#include "GUI.hpp"

#include "MyEval.hpp"
#include "Config.hpp"
//#include "LassoController.hpp"
#include "SimplifiedLassoController.hpp"
#include "worlds.hpp"
#include "SpeedManager.hpp"
#include "DataLogger.hpp"

using namespace std;

class MyExperiment {
    Config m_config;
    int m_trialIndex;

    shared_ptr<GUI> m_gui;
    shared_ptr<Simulator> m_sim;
    shared_ptr<World> m_world;

    double m_simulationTime = 0;
    Timer m_simTimer;
    ofstream m_statsStream, m_robotPoseStream, m_robotStateStream, m_puckPositionStream;

    stringstream m_status;
    double m_eval;
    LassoEval::SGFCounts m_counts;

    default_random_engine m_rng;
    bool m_aborted;
    bool m_waitAfterCompletion;

    SpeedManager m_speedManager;
    DataLogger m_dataLogger;

public:
    MyExperiment(Config config, int trialIndex, int rngSeed, bool waitAfterCompletion = false)
        : m_config(config)
        , m_trialIndex(trialIndex)
        , m_rng(rngSeed)
        , m_aborted(false)
        , m_waitAfterCompletion(waitAfterCompletion)
        , m_speedManager(config)
        , m_dataLogger(config, trialIndex)
    {
        resetSimulator();
    }

    void doSimulationStep()
    {
        if (m_config.writeDataSkip && m_speedManager.getStepCount() % m_config.writeDataSkip == 0)
            m_dataLogger.writeToFile(m_sim->getWorld(), m_speedManager.getStepCount(), m_eval, m_counts);

        m_speedManager.incrementStepCount();

        if (!m_gui && (m_speedManager.getStepCount() % 5000 == 0)) {
            cout << "Simulation Step: " << m_speedManager.getStepCount() << " / " << m_config.maxTimeSteps << "\n";
        }

        for (auto& robot : m_sim->getWorld()->getEntities("robot")) {

            EntityAction action = robot.getComponent<CController>().controller->getLastAction();
            if (m_config.controllerSkip == 0 || m_speedManager.getStepCount() % m_config.controllerSkip == 0)
                action = robot.getComponent<CController>().controller->getAction();

            if (!m_config.fakeRobots)
                action.doAction(robot, m_speedManager.getSimTimeStep());
        }

        if (m_speedManager.getSimTimeStep() > 0)
            m_sim->update(m_speedManager.getSimTimeStep());
    }

    void run()
    {
        bool running = true;
        while (running) {
            //m_eval = LassoEval::PuckGridValues(m_world, "red_puck", 2, false);
            if (m_config.numPucks > 0)
                m_eval = LassoEval::PuckSSDFromIdealPosition(m_world, "red_puck", Vec2{m_config.goalX, m_config.goalY});
            //m_propSlowed = LassoEval::ProportionSlowedRobots(m_world);            
            if (isnan(m_eval)) {
                cerr << "nan evaluation encountered!\n";
                m_aborted = true;
                break;
            }

            m_counts = LassoEval::getSGFCounts(m_world);

            m_simTimer.start();
            for (size_t i = 0; i < m_speedManager.getRenderSteps(); i++) {
                if (m_config.maxTimeSteps > 0 && m_speedManager.getStepCount() >= m_config.maxTimeSteps) {
                    running = false;
                }
                doSimulationStep();
            }
            m_simulationTime += m_simTimer.getElapsedTimeInMilliSec();

            if (m_gui) {
                m_status = stringstream();
                m_status << "Step: " << m_speedManager.getStepCount();
                if (m_config.maxTimeSteps > 0) {
                    m_status << " / " << m_config.maxTimeSteps;
                }
                m_status << endl;
                m_status << "Eval: " << m_eval << endl;
                m_status << "Num. Solo: " << m_counts.numSolo << endl;
                m_status << "Num. Grupo: " << m_counts.numGrupo << endl;
                m_status << "Num. Fermo: " << m_counts.numFermo << endl;

                for (auto robot : m_sim->getWorld()->getEntities("robot"))
                {
                    if (!robot.hasComponent<CControllerVis>()) { continue; }
                    auto vis = robot.getComponent<CControllerVis>();

                    if (vis.selected) {
                        m_status << vis.msg;
                    }
                }

                for (auto probe : m_sim->getWorld()->getEntities("probe"))
                {
                    int nGrids = m_sim->getWorld()->getNumberOfGrids();
                    const Vec2& pos = probe.getComponent<CTransform>().p;
                    size_t gX = (size_t)round(pos.x);
                    size_t gY = (size_t)round(pos.y);
                    m_status << "position: " << gX << ", " << gY << endl;
                    for (int gridIndex = 0; gridIndex < nGrids; ++gridIndex) {
                        double reading = m_sim->getWorld()->getGrid(gridIndex).get(gX, gY);
                        m_status << "Grid " << gridIndex << ": " << reading << endl;
                    }
                }

                if (m_config.controllerSkip == 0 || m_speedManager.getStepCount() % m_config.controllerSkip == 0) {
                    // These are the value grids used within the controller for vis
                    m_gui->updateGridImage(2, true, false, false);
                    m_gui->updateGridImage(3, false, true, false);
                    m_gui->updateGridImage(4, false, false, true);
                    m_gui->updateGridImage(5, true, true, true);
                    m_sim->getWorld()->getGrid(2).setAll(0);
                    m_sim->getWorld()->getGrid(3).setAll(0);
                    m_sim->getWorld()->getGrid(4).setAll(0);
                    m_sim->getWorld()->getGrid(5).setAll(0);
                }

                m_gui->setStatus(m_status.str());

                // draw gui
                m_gui->update();

                if (m_config.captureScreenshots) {
                    stringstream filename;
                    filename << m_config.screenshotFilenameBase << m_speedManager.getStepCount() / m_speedManager.getRenderSteps() << ".png";
                    m_gui->saveScreenshot(filename.str());
                }
            }

            // RESET SIMULATOR IF DESIRED
            // resetSimulator();
        }

        // Display final step count
        std::cout << "Simulation completed at step: " << m_speedManager.getStepCount() << std::endl;

        if (m_gui) {
            if (m_waitAfterCompletion) {
                // Keep the GUI open and wait for user to close the window
                std::cout << "\nSimulation complete. Close the window or press ESC to exit..." << std::endl;
                
                while (m_gui && m_gui->isOpen()) {
                    m_gui->update();
                    // Small delay to prevent excessive CPU usage
                    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
                }
            } else {
                m_gui->close();
            }
            m_gui = NULL;
        }
    }

    bool wasAborted()
    {
        return m_aborted;
    }

    double getEvaluation()
    {
        return m_eval;
    }

private:
    void resetSimulator()
    {
        m_aborted = false;

        if (m_world == NULL)
            m_world = lasso_world::GetWorld(m_rng, m_config);

        // randomizeWorld(m_rng, m_config.robotRadius, m_config.puckRadius);

        m_sim = make_shared<Simulator>(m_world);

        if (m_gui) {
            m_gui->setSim(m_sim);
        } else if (m_config.gui) {
            m_gui = make_shared<GUI>(m_sim, 144);
            m_gui->setKeyboardCallback(&m_speedManager);
        }

        for (auto e : m_world->getEntities("robot")) {
            e.addComponent<CController>(make_shared<SimplifiedLassoController>(e, m_world, m_rng, m_config));
        }
    }
};
