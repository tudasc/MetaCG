// Calls inside an instantiated function template are attributed to the instantiation

void sink(int) {}

template <class T>
void tmpl(T t) {
  sink(t);
}

int main() { tmpl(1); }
