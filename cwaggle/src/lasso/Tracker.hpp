#pragma once

#include "World.hpp"
#include <memory>
#include <string>

using std::shared_ptr;
using std::string;

/**
 * Abstract base class for tracking system state transitions and behaviors.
 * This class defines the interface that all tracker implementations must follow.
 */
class Tracker {
public:
    /**
     * Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~Tracker() = default;

    /**
     * Update the tracker with the current world state and time.
     * This method should be called at each simulation step to track state changes.
     * 
     * @param world Shared pointer to the current world state
     * @param time Current simulation time step
     */
    virtual void update(shared_ptr<World> world, int time) = 0;

    /**
     * Get a human-readable status string describing the current tracked state.
     * This is typically used for display or logging purposes.
     * 
     * @return String representation of the current tracker status
     */
    virtual string getStatusString() = 0;
};
