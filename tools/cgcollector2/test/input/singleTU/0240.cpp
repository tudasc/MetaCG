// A function template that is never instantiated contributes neither a node nor any edges

void sink(int) {}

template <class T>
void tmpl(T t) {
  sink(t);
}

int main() {}
