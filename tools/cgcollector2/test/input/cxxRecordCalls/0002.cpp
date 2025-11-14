/**
* File: CalledDeclTest.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
 */

#include <vector>
#include <iostream>

int main(int argc, char** argv) {
  std::vector<int> v;
  v.push_back(argc);

  std::cout << "Hello world!" << std::endl;
  return 0;
}
