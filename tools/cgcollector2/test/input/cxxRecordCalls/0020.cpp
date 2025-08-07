/**
* File: MemberFunctions.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

struct A {
  int foo() { return 6; }
};

int main() {
  A a = A();
  int r = a.foo();
  return r;
}