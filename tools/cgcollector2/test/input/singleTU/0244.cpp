// Two function parameter packs: calls in a noexcept-spec clang leaves uninstantiated are resolved per pack element

void probe(int) {}
void probe(double) {}
void sink(int) {}

template <class... A, class... B>
void multi(A... a, B... b, int last) noexcept(noexcept((probe(a), ...))) {
  sink(last);
}

int main() { multi<int, double>(1, 2.0, 3); }
