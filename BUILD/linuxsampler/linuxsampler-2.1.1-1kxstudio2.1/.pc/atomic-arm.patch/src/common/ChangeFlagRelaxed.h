/*
    Copyright (C) 2013 Christian Schoenebeck
 */

#ifndef LS_CHANGEFLAGRELAXED_H
#define LS_CHANGEFLAGRELAXED_H

#include "atomic.h"

/**
 * May be used as boolean variable to synchronize that some shared resource has
 * been changed, with relaxed memory order. It is designed for exactly 1 reading
 * thread (calling readAndReset()) and n writing threads (calling raise()).
 * Synchronization with "relaxed memory order" means here that the information
 * written by raise() calls are never lost, but it may take some time to
 * propagate to the reading thread, thus readAndReset() may not always return
 * the very latest information. So the informations might be delivered with a
 * delay.
 */
class ChangeFlagRelaxed {
public:
    ChangeFlagRelaxed() {
        newval = (atomic_t) ATOMIC_INIT(0);
        oldval = 0;
    }
    
    inline bool readAndReset() {
        int v = atomic_read(&newval);
        bool changed = (oldval != v);
        oldval = v;
        return changed;
    }
    
    inline void raise() {
        atomic_inc(&newval);
    }
private:
    atomic_t newval;
    int      oldval;
};

#endif // LS_CHANGEFLAGRELAXED_H
