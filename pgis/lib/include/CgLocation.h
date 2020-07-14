#ifndef CGLOCATION_H
#define CGLOCATION_H

class CgLocation;

class CgLocation {
 public:
  CgLocation(double time, int threadId, int procId, unsigned long long numCalls);

  double get_time() const;
  int get_threadId() const;
  int get_procId() const;
  unsigned long long get_numCalls() const;

 private:
  double time;
  int threadId;
  int procId;
  unsigned long long numCalls;
};

#endif  // CGLOCATION_H
