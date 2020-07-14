#include "CgLocation.h"

CgLocation::CgLocation(double time, int threadId, int procId, unsigned long long numCalls) {
  this->time = time;
  this->threadId = threadId;
  this->procId = procId;
  this->numCalls = numCalls;
}

double CgLocation::get_time() const { return time; }
int CgLocation::get_threadId() const { return threadId; }
int CgLocation::get_procId() const { return procId; }
unsigned long long CgLocation::get_numCalls() const { return numCalls; }
