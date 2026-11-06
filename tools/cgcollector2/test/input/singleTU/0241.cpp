// An explicit specialization of a function template keeps its own callees, separate from the implicit instantiation

void sink(int) {}
void other(double) {}

template <class T>
void tmpl(T t) {
  sink(t);
}

template <>
void tmpl<double>(double d) {
  other(d);
}

int main() {
  tmpl(1);
  tmpl(1.0);
}
