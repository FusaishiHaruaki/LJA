#include "rolling_hash.hpp"

size_t hashing::KWH::accumulatedTime_extendRight = 0;
int hashing::KWH::numCalls_extendRight = 0;
size_t hashing::KWH::accumulatedTime_extendLeft = 0;
int hashing::KWH::numCalls_extendLeft = 0;
size_t hashing::KWH::accumulatedTime_next = 0;
int hashing::KWH::numCalls_next = 0;
size_t hashing::KWH::accumulatedTime_prev = 0;
int hashing::KWH::numCalls_prev = 0;
size_t hashing::KWH::accumulatedTime_hasNext = 0;
int hashing::KWH::numCalls_hasNext = 0;
size_t hashing::KWH::accumulatedTime_hasPrev = 0;
int hashing::KWH::numCalls_hasPrev = 0;