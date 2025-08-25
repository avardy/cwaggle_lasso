#pragma once

#include "Tracker.hpp"
#include "Config.hpp"

/**
 * NullTracker - A no-op implementation of the Tracker interface.
 * This tracker does absolutely nothing but satisfy the Tracker interface,
 * allowing simulations to run without any tracking overhead.
 */
class NullTracker : public Tracker {
public:
    NullTracker(Config config) {
        // Constructor takes config for consistency but does nothing with it
    }

    virtual ~NullTracker() override {
        // No cleanup needed - we never allocated anything
    }

    virtual void update(shared_ptr<World> world, int time) override {
        // Do absolutely nothing
    }

    virtual string getStatusString() override {
        return "Tracking disabled";
    }
};
