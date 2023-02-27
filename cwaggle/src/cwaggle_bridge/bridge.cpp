#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <memory>
#include <fstream>
#include <random>
#include <chrono>

//#include "CWaggle.h"
#include "../lasso/MyExperiment.hpp"
#include "../lasso/Config.hpp"
#include "../lasso/LassoController.hpp"
#include "../lasso/MyEval.hpp"
#include "../lasso/worlds.hpp"

using namespace std;
using namespace std::chrono;

namespace py = pybind11;

void init() {
    // Not needed here, but maybe there is some setup to be done upon import.
}

void runSimulation() {
    string configFile = "lasso_config.txt";
    Config config;
    config.load(configFile);

    double avgEval = 0;
    for (int i = 0; i < config.numTrials; i++) {
        cerr << "Trial: " << i << "\n";

        // We use i + 1 for the RNG seed because seeds of 0 and 1 seem to generate the
        // same result.
        MyExperiment exp(config, i, i + 1);
        exp.run();
        if (exp.wasAborted())
            cerr << "Trial aborted." << "\n";
        else
            avgEval += exp.getEvaluation();
    }

    cout << "\t" << avgEval / config.numTrials << "\n";
}

PYBIND11_MODULE(cwaggle_bridge, m) {
    init();

    m.doc() = "Allows elements of CWaggle to be called from Python.";

    m.def("runSimulation", &runSimulation);
}
