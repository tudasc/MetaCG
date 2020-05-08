#ifndef CGLOCATION_H
#define CGLOCATION_H

class CgLocation {
 public:
  CgLocation(double time, int threadId, int procId, unsigned long long numCalls) {
    this->time = time;
    this->threadId = threadId;
    this->procId = procId;
    this->numCalls = numCalls;
  }

  inline double get_time() const { return time; }
  inline int get_threadId() const { return threadId; }
  inline int get_procId() const { return procId; }
  inline unsigned long long get_numCalls() const { return numCalls; }

 private:
  double time;
  int threadId;
  int procId;
  unsigned long long numCalls;
};

#endif  // CGLOCATION_H
