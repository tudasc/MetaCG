// Calls inside an instantiated member function template are attributed to the instantiation, the implicit default
// constructor of the class is captured as well. (C{} would be aggregate initialization without a constructor call)

void sink(int) {}

template <class T>
void tmpl(T t) {
  sink(t);
}

struct C {
  template <class T>
  void m(T t) {
    tmpl(t);
  }
};

int main() {
  C c;
  c.m(3);
}
