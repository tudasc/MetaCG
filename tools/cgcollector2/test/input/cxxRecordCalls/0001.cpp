/**
* File: AllTheHeaderFiles.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#include "math.h"

int main() {
  int (*a)(int, int) = nullptr;
  double (*b)(double, double) = nullptr;
  double (*c)(double) = nullptr;
  return b(c(2.0), 4.0);
}