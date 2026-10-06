// Calls inside an instantiated member function template are attributed to the instantiation

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

int main() { C{}.m(3); }
