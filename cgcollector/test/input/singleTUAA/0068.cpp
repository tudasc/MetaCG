// Test for the alias analysis that we can handle nested structs and pointers and arrays of them.
// Note: The AA does not differentiate between values inside an array
struct B;
struct D;

struct D {
  struct D* SelfP;
  struct B* BaseP;
  void* Fptr;
};

struct C {
  struct D Other;
  struct D* OtherPtr;
  void* Fptr;
};

struct A {
  struct C OtherArray[2];
};

struct B {
  struct B* SelfP;
  struct A Other;
  struct A* OtherP;
  struct A* OtherArrayPtr[5];
};

extern void fooA();
extern void fooB();
extern void fooC();  // This does not get called

typedef void (*Ftype)();

int main() {
  struct B Base;
  Base.SelfP = &Base;

  struct A A1, A2, A3;

  Base.Other = A1;
  Base.OtherP = &A2;
  Base.OtherArrayPtr[4] = &A3;

  struct C C1, C2, C3, C4;

  A1.OtherArray[0] = C1;
  A1.OtherArray[1] = C2;
  // A2 is empty
  A3.OtherArray[0] = C3;

  C4.Fptr = (void*)&fooC;

  struct D D1, D2, D3, D4;

  C1.Other = D1;
  C1.OtherPtr = &D2;
  C2.OtherPtr = &D3;

  C3.Other = D4;

  D2.Fptr = (void*)&fooA;
  D3.Fptr = (void*)&fooB;
  D4.BaseP = &Base;
  D4.SelfP = &D4;

  ((Ftype)Base.SelfP->OtherArrayPtr[0]->OtherArray[0].Other.Fptr)();  // This should call nothing

  ((Ftype)Base.Other.OtherArray[1].OtherPtr->Fptr)();  // This should call both fooA and fooB
}