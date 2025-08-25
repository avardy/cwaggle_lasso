#pragma once

#include "World.hpp"
#include "MyEval.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>

using LassoEval::Triple;
using std::vector;

vector<Triple> generateAllTriples(int n) {
    vector<Triple> triples;

    // Many orderings are possible. This is just the one that I first wrote
    // down on paper.
    for (int f = 0; f <= n; ++f) {
        for (int s = n - f; s >= 0; --s) {
            int g = n - s - f;
            triples.push_back(Triple{s, g, f});
        }
    }
    return triples;
}

class SGFTracker {
    Config m_config;
    bool m_readyToTrack;
    Triple m_lastCounts;
    int m_lastChangeTime;

    // Vector of all possible triples.
    vector<Triple> m_triples;

    // Matrix of uncensored (i.e. normal) transition durations. Indexeed by the
    // index of the "from" triple, then by the index of the "to" triple. Each
    // cell is then the vector of raw durations.
    vector<vector<vector<int>>> m_uncensoredDurations;

    // Vector of censored transition durations. Indexed by the index of the
    // "from" triple. There is no "to" triple for these. Each cell is the vector
    // of raw durations.
    vector<vector<int>> m_censoredDurations;

public:
    SGFTracker(Config config)
        : m_config(config)
        , m_readyToTrack(false)
    {
        m_triples = generateAllTriples(config.numRobots);
    
        int T = m_triples.size();
        m_uncensoredDurations = vector<vector<vector<int>>>(T, vector<vector<int>>(T, vector<int>()));
        m_censoredDurations = vector<vector<int>>(T, vector<int>());
    }

    ~SGFTracker() {
        // Write triples key file (CSV format for pandas)
        std::ofstream triplesFile("sgf_triples.csv");
        if (triplesFile.is_open()) {
            triplesFile << "index,solo,grupo,fermo\n";
            for (size_t i = 0; i < m_triples.size(); ++i) {
                triplesFile << i << "," << m_triples[i].s << "," 
                           << m_triples[i].g << "," << m_triples[i].f << "\n";
            }
            triplesFile.close();
        }

        // Write survival analysis data (CSV format for pandas)
        std::ofstream survivalFile("sgf_survival_data.csv");
        if (survivalFile.is_open()) {
            survivalFile << "from_index,to_index,duration,censored,from_solo,from_grupo,from_fermo,to_solo,to_grupo,to_fermo\n";
            
            // Export all uncensored durations
            for (size_t i = 0; i < m_uncensoredDurations.size(); ++i) {
                for (size_t j = 0; j < m_uncensoredDurations[i].size(); ++j) {
                    for (int duration : m_uncensoredDurations[i][j]) {
                        survivalFile << i << "," << j << "," << duration << ",0,"
                                   << m_triples[i].s << "," << m_triples[i].g << "," << m_triples[i].f << ","
                                   << m_triples[j].s << "," << m_triples[j].g << "," << m_triples[j].f << "\n";
                    }
                }
            }

            // Now the censored durations.
            for (size_t i = 0; i < m_censoredDurations.size(); ++i) {
                for (int duration : m_censoredDurations[i]) {
                    survivalFile << i << ",-1," << duration << ",1,"
                               << m_triples[i].s << "," << m_triples[i].g << "," << m_triples[i].f
                               << ",-1,-1,-1\n";
                }
            }
            
            survivalFile.close();
        }

        // Write human-readable summary
        std::ostringstream summaryContent;
        summaryContent << "SGF Transition Analysis Summary\n";
        summaryContent << "==============================\n\n";
        
        summaryContent << "Number of robots: " << m_config.numRobots << "\n";
        summaryContent << "Total possible states: " << m_triples.size() << "\n\n";
        
        summaryContent << "State Definitions (Solo, Grupo, Fermo):\n";
        summaryContent << "--------------------------------------\n";
        for (size_t i = 0; i < m_triples.size(); ++i) {
            summaryContent << std::setw(3) << i << ": (" 
                          << std::setw(2) << m_triples[i].s << ", "
                          << std::setw(2) << m_triples[i].g << ", "
                          << std::setw(2) << m_triples[i].f << ")\n";
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
        std::ofstream summaryFile("sgf_transition_summary.txt");
        if (summaryFile.is_open()) {
            summaryFile << summaryContent.str();
            summaryFile.close();
        }
    }

    void actualUpdate(Triple counts, int elapsed, bool endOfTrial) {

        int fromIndex = getTripleIndex(m_lastCounts);

        if (endOfTrial) {
            // There is no "toIndex" here because this event is censored.
            m_censoredDurations[fromIndex].push_back(elapsed);
        } else {
            int toIndex = getTripleIndex(counts);
            m_uncensoredDurations[fromIndex][toIndex].push_back(elapsed);
        }
    }

    void update(shared_ptr<World> world, Triple counts, int time)
    {
        bool endOfTrial = (time == m_config.maxTimeSteps);

        if (!m_readyToTrack) {
            m_readyToTrack = true;
            m_lastChangeTime = time;
            m_lastCounts = counts;
            return;
        }

        if (counts != m_lastCounts || endOfTrial) {
            // SGF counts have changed, or else it is the end of the trial
            int elapsed = time - m_lastChangeTime;
            if (elapsed <= 0) {
                // Log warning or handle edge case
                std::cerr << "Warning: Non-positive elapsed time: " << elapsed 
                          << " (time=" << time << ", lastChange=" << m_lastChangeTime << ")\n";
                return; // Skip this transition
            }
            actualUpdate(counts, elapsed, endOfTrial);

            m_lastChangeTime = time;
            m_lastCounts = counts;

            if (endOfTrial) {
                m_readyToTrack = false;
            }
        }
    }

private:
    int getTripleIndex(const Triple& triple) const {
        auto it = std::find(m_triples.begin(), m_triples.end(), triple);
        if (it == m_triples.end()) {
            throw std::runtime_error("Triple not found in m_triples");
        }
        return std::distance(m_triples.begin(), it);
    }
};
