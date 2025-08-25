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
#include "SGFTracker.hpp"
#include "SGTracker.hpp"
#include "NullTracker.hpp"

using namespace std;

double runExperimentWithTracker(Config config, Tracker& tracker, bool waitAfterCompletion = false)
{
    double avgEval = 0;
    for (int i = config.startTrialIndex; i < config.startTrialIndex + config.numTrials; i++) {
        cerr << "Trial: " << i << "\n";

        // We use i + 1 for the RNG seed because seeds of 0 and 1 seem to generate the
        // same result.
        MyExperiment exp(config, i, i + 1, tracker, waitAfterCompletion);
        exp.run();
        if (exp.wasAborted())
            cerr << "Trial aborted." << "\n";
        else
            avgEval += exp.getEvaluation();
    }

    cout << "\t" << avgEval / config.numTrials << "\n";

    return avgEval / config.numTrials;
}

double singleExperiment(Config config, bool waitAfterCompletion = false)
{
    double avgEval = 0;

    // Create tracker based on trackingMode
    switch (config.trackingMode) {
        case 0:
            // No tracking
            cerr << "Running with no tracking (NullTracker)...\n";
            {
                NullTracker nullTracker(config);
                avgEval = runExperimentWithTracker(config, nullTracker, waitAfterCompletion);
            }
            break;
        case 1:
            // SG tracking only
            cerr << "Running with SG Tracker...\n";
            {
                SGTracker sgTracker(config);
                avgEval = runExperimentWithTracker(config, sgTracker, waitAfterCompletion);
            }
            break;
        case 2:
            // SGF tracking (default)
            cerr << "Running with SGF Tracker...\n";
            {
                SGFTracker sgfTracker(config);
                avgEval = runExperimentWithTracker(config, sgfTracker, waitAfterCompletion);
            }
            break;
        default:
            cerr << "Invalid tracking mode " << config.trackingMode << ", defaulting to SGF Tracker...\n";
            {
                SGFTracker sgfTracker(config);
                avgEval = runExperimentWithTracker(config, sgfTracker, waitAfterCompletion);
            }
            break;
    }

    return avgEval;
}

void paramSweep(Config config, bool waitAfterCompletion = false)
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
        double avgEval = singleExperiment(config, waitAfterCompletion);
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

void runParamOrSingle(Config config, bool waitAfterCompletion = false)
{
    if (config.paramSweep)
        paramSweep(config, waitAfterCompletion);
    else
        singleExperiment(config, waitAfterCompletion);
}

void arenaSweep(Config config, bool waitAfterCompletion = false)
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

        runParamOrSingle(config, waitAfterCompletion);
    }
}

int main(int argc, char** argv)
{
    bool waitAfterCompletion = false;
    
    if (argc == 2 && string(argv[1]) == "--wait") {
        waitAfterCompletion = true;
    } else if (argc != 1) {
        cerr << "Usage\n\tcwaggle_lasso [--wait]" << endl;
        cerr << "\t--wait: Wait for user input before exiting" << endl;
        return -1;
    }

    // Read the config file name from console if it exists
    string configFile = "lasso_config.txt";
    Config config;
    config.load(configFile);

    if (config.arenaSweep)
        arenaSweep(config, waitAfterCompletion);
    else
        runParamOrSingle(config, waitAfterCompletion);

    return 0;
}