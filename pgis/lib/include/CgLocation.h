/**
 * File: CgNodeLocation.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef CGLOCATION_H
#define CGLOCATION_H

class CgLocation {
 public:
  CgLocation(double time, double inclusiveTime, int threadId, int procId, unsigned long long numCalls) {
    this->time = time;
    this->threadId = threadId;
    this->procId = procId;
    this->numCalls = numCalls;
    this->inclusiveTime = inclusiveTime;
  }

  inline double getTime() const { return time; }
  inline double getInclusiveTime() const { return inclusiveTime; }
  inline int getThreadId() const { return threadId; }
  inline int getProcId() const { return procId; }
  inline unsigned long long getNumCalls() const { return numCalls; }

 private:
  double time;
  double inclusiveTime;
  int threadId;
  int procId;
  unsigned long long numCalls;
};

#endif  // CGLOCATION_H
