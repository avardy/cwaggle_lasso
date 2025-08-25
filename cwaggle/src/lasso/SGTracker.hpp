#pragma once

// SGTracker tracks the number of solo and grupo robots only
// (excluding fermo robots from the SGF model)

#include "Tracker.hpp"
#include "World.hpp"
#include "MyEval.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>

using std::vector;

struct Pair {
    int s;
    int g;
    
    bool operator==(const Pair& other) const {
        return s == other.s && g == other.g;
    }
    
    bool operator!=(const Pair& other) const {
        return !(*this == other);
    }
};

Pair getSGCounts(std::shared_ptr<World> world)
{
    Pair counts{0, 0};

    int total = 0;
    for (auto& robot : world->getEntities("robot")) {
        // Ugly!
        std::shared_ptr<LassoController> lassoCtrl = std::dynamic_pointer_cast<LassoController>(robot.getComponent<CController>().controller);

        int sgf = lassoCtrl->getSGFstate();
        if (sgf == 0)
            ++counts.s;
        else if (sgf == 1)
            ++counts.g;
        // Note: we ignore fermo robots (sgf == 2)

        ++total;
    }
    // Total includes fermo robots, but we only track solo + grupo
    return counts;
}

vector<Pair> generateAllPairs(int n) {
    vector<Pair> pairs;

    // Generate all possible (solo, grupo) combinations where solo + grupo <= n
    // The rest are assumed to be fermo robots
    for (int s = 0; s <= n; ++s) {
        for (int g = 0; g <= n - s; ++g) {
            pairs.push_back(Pair{s, g});
        }
    }
    return pairs;
}

class SGTracker : public Tracker {
private:
    Config m_config;
    bool m_readyToTrack;
    Pair m_counts, m_lastCounts;
    int m_lastChangeTime;

    // Vector of all possible pairs.
    vector<Pair> m_pairs;

    // Matrix of uncensored (i.e. normal) transition durations. Indexed by the
    // index of the "from" pair, then by the index of the "to" pair. Each
    // cell is then the vector of raw durations.
    vector<vector<vector<int>>> m_uncensoredDurations;

    // Vector of censored transition durations. Indexed by the index of the
    // "from" pair. There is no "to" pair for these. Each cell is the vector
    // of raw durations.
    vector<vector<int>> m_censoredDurations;

public:
    SGTracker(Config config)
        : m_config(config)
        , m_readyToTrack(false)
    {
        m_pairs = generateAllPairs(config.numRobots);
    
        int T = m_pairs.size();
        m_uncensoredDurations = vector<vector<vector<int>>>(T, vector<vector<int>>(T, vector<int>()));
        m_censoredDurations = vector<vector<int>>(T, vector<int>());
    }

    virtual ~SGTracker() override {
        // Write pairs key file (CSV format for pandas)
        std::ofstream pairsFile("sg_pairs.csv");
        if (pairsFile.is_open()) {
            pairsFile << "index,solo,grupo\n";
            for (size_t i = 0; i < m_pairs.size(); ++i) {
                pairsFile << i << "," << m_pairs[i].s << "," 
                         << m_pairs[i].g << "\n";
            }
            pairsFile.close();
        }

        // Write survival analysis data (CSV format for pandas)
        std::ofstream survivalFile("sg_survival_data.csv");
        if (survivalFile.is_open()) {
            survivalFile << "from_index,to_index,duration,censored,from_solo,from_grupo,to_solo,to_grupo\n";
            
            // Export all uncensored durations
            for (size_t i = 0; i < m_uncensoredDurations.size(); ++i) {
                for (size_t j = 0; j < m_uncensoredDurations[i].size(); ++j) {
                    for (int duration : m_uncensoredDurations[i][j]) {
                        survivalFile << i << "," << j << "," << duration << ",0,"
                                   << m_pairs[i].s << "," << m_pairs[i].g << ","
                                   << m_pairs[j].s << "," << m_pairs[j].g << "\n";
                    }
                }
            }

            // Now the censored durations.
            for (size_t i = 0; i < m_censoredDurations.size(); ++i) {
                for (int duration : m_censoredDurations[i]) {
                    survivalFile << i << ",-1," << duration << ",1,"
                               << m_pairs[i].s << "," << m_pairs[i].g
                               << ",-1,-1\n";
                }
            }
            
            survivalFile.close();
        }

        // Write human-readable summary
        std::ostringstream summaryContent;
        summaryContent << "SG Transition Analysis Summary\n";
        summaryContent << "==============================\n\n";
        
        summaryContent << "Number of robots: " << m_config.numRobots << "\n";
        summaryContent << "Total possible states: " << m_pairs.size() << "\n\n";
        
        summaryContent << "State Definitions (Solo, Grupo):\n";
        summaryContent << "--------------------------------\n";
        for (size_t i = 0; i < m_pairs.size(); ++i) {
            int fermo = m_config.numRobots - m_pairs[i].s - m_pairs[i].g;
            summaryContent << std::setw(3) << i << ": (" 
                          << std::setw(2) << m_pairs[i].s << ", "
                          << std::setw(2) << m_pairs[i].g << ") "
                          << "[" << fermo << " fermo]\n";
        }
        
        summaryContent << "\nTransition Statistics:\n";
        summaryContent << "---------------------\n";
        int numUncensored = 0;
        for (size_t i = 0; i < m_uncensoredDurations.size(); ++i)
            for (size_t j = 0; j < m_uncensoredDurations[i].size(); ++j)
                numUncensored += m_uncensoredDurations[i][j].size();
        int numCensored = 0;
        for (size_t i = 0; i < m_censoredDurations.size(); ++i)
            numCensored += m_censoredDurations[i].size();
        int total = numUncensored + numCensored;
        
        summaryContent << "\nEvent Summary:\n";
        summaryContent << "-------------\n";
        summaryContent << "Uncensored transition events: " << numUncensored << "\n";
        summaryContent << "Censored events (right-censored): " << numCensored << "\n";
        summaryContent << "Total events recorded: " << total << "\n";
        summaryContent << "Censoring rate: " << std::fixed << std::setprecision(1) 
                      << (total > 0 ? (100.0 * numCensored / total) : 0.0) << "%\n";

        // Output to stdout
        std::cout << "\n" << summaryContent.str() << std::endl;
        
        // Write to file
        std::ofstream summaryFile("sg_transition_summary.txt");
        if (summaryFile.is_open()) {
            summaryFile << summaryContent.str();
            summaryFile.close();
        }
    }

    virtual void update(shared_ptr<World> world, int time) override
    {
        m_counts = getSGCounts(world);
        bool endOfTrial = (time == m_config.maxTimeSteps);

        if (!m_readyToTrack) {
            m_readyToTrack = true;
            m_lastChangeTime = time;
            m_lastCounts = m_counts;
            return;
        }

        if (m_counts != m_lastCounts || endOfTrial) {
            // SG counts have changed, or else it is the end of the trial
            int elapsed = time - m_lastChangeTime;
            if (elapsed <= 0) {
                // Log warning or handle edge case
                std::cerr << "Warning: Non-positive elapsed time: " << elapsed 
                          << " (time=" << time << ", lastChange=" << m_lastChangeTime << ")\n";
                return; // Skip this transition
            }
            actualUpdate(elapsed, endOfTrial);

            m_lastChangeTime = time;
            m_lastCounts = m_counts;

            if (endOfTrial) {
                m_readyToTrack = false;
            }
        }
    }

    virtual string getStatusString() override {
        int fermo = m_config.numRobots - m_counts.s - m_counts.g;
        return "Num. Solo: " + to_string(m_counts.s) + "\n"
            "Num. Grupo: " + to_string(m_counts.g) + "\n"
            "Num. Fermo: " + to_string(fermo) + " (inferred)";
    }

private:
    void actualUpdate(int elapsed, bool endOfTrial) {

        int fromIndex = getPairIndex(m_lastCounts);

        if (endOfTrial) {
            // There is no "toIndex" here because this event is censored.
            m_censoredDurations[fromIndex].push_back(elapsed);
        } else {
            int toIndex = getPairIndex(m_counts);
            m_uncensoredDurations[fromIndex][toIndex].push_back(elapsed);
        }
    }

    int getPairIndex(const Pair& pair) const {
        auto it = std::find(m_pairs.begin(), m_pairs.end(), pair);
        if (it == m_pairs.end()) {
            throw std::runtime_error("Pair not found in m_pairs");
        }
        return std::distance(m_pairs.begin(), it);
    }
};
