#include "IDGenerator.h"

namespace LinuxSampler {

IDGenerator::IDGenerator(bool simpleAlgorithm) {
    previousId = -1;
    simple = simpleAlgorithm;
}

int IDGenerator::create() {
    int newId = previousId + 1;
    if (newId < 0 /*reached upper value range limit*/ ||
        ids.find(newId) != ids.end() /*already in use*/)
    {
        if (simple) return -1; // whole value range was used, and we dont know what we could pick next

        newId = -1;
        for (int i = 0; i >= 0; i++) {
            if (ids.find(i) == ids.end()) {
                // we found an unused id ...
                newId = i;
                break;
            }
        }
        if (newId < 0) {
            // we didn't find a free ID, the whole value range is full!
            return -1;
        }
    }

    if (!simple) ids.insert(newId);
    previousId = newId;
    return newId;
}

void IDGenerator::destroy(int id) {
    if (simple) return; // nothing to do
    ids.erase(id);
}

} // namespace LinuxSampler
