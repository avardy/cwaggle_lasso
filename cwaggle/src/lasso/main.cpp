#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// For mkdir
#include <sys/stat.h>
#include <sys/types.h>

#include "CWaggle.h"
#include "MyExperiment.hpp"

using namespace std;

double singleExperiment(Config config)
{
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

    return avgEval / config.numTrials;
}

void paramSweep(Config config)
{
    // You should just configure the following four lines.
    using choiceType = size_t;
    string choiceName = "numRobots";
    choiceType *choiceVariable = &config.numRobots;
    vector<choiceType> choices{ 4, 8, 12, 16 };

    string filenameBase = config.dataFilenameBase;
    vector<pair<choiceType, double>> sweepResults;
    for (choiceType choice : choices) {
        ostringstream oss;
        oss << filenameBase << "/" << choiceName << "_" << choice;
        config.dataFilenameBase = oss.str();

        *choiceVariable = choice;

        cout << choiceName << ": " << choice << endl;
        double avgEval = singleExperiment(config);
        sweepResults.push_back(make_pair(choice, avgEval));
    }

    for (const pair<choiceType, double> result : sweepResults)
        cout << ": " << result.first << ": " << result.second << endl;

    /*
    cout << "\n\nALL RESULTS: \n" << endl;
    printResults(sweepResults);

    cout << "\n\nSORTED RESULTS: \n" << endl;
    sort(sweepResults.begin(), sweepResults.end(),
        [](const tuple<double, double, double>& lhs, const tuple<double, double, double>& rhs) {
            return get<2>(lhs) < get<2>(rhs);
        }
    );
    printResults(sweepResults);
    */
}

void runParamOrSingle(Config config)
{
    if (config.paramSweep)
        paramSweep(config);
    else
        singleExperiment(config);
}

void arenaSweep(Config config)
{
    string choiceName = "arenaConfig";
    vector<string> arenas{ "sim_stadium_no_wall", "sim_stadium_one_wall", "sim_stadium_two_walls", "sim_stadium_three_walls" };

    string filenameBase = config.dataFilenameBase;
    for (string arena : arenas) {
        ostringstream oss;
        oss << filenameBase << "/" << arena;
        config.dataFilenameBase = oss.str();

        config.arenaConfig = arena;
        cout << config.arenaConfig << endl;
        if (mkdir(config.dataFilenameBase.c_str(), 0777) == -1 && errno != EEXIST)
            cerr << "Error creating directory: " << config.dataFilenameBase << endl;

        runParamOrSingle(config);
    }
}

int main(int argc, char** argv)
{
    if (argc != 1) {
        cerr << "Usage\n\tcwaggle_lasso NO_ARGUMENTS_PLEASE" << endl;
        return -1;
    }

    // Read the config file name from console if it exists
    string configFile = "lasso_config.txt";
    Config config;
    config.load(configFile);

    if (config.arenaSweep)
        arenaSweep(config);
    else
        runParamOrSingle(config);

    return 0;
}