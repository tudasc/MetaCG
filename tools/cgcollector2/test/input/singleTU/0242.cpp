// Recursive function template instantiations form a call chain down to the terminating instantiation

void sink(int) {}

template <int N>
void rec() {
  if constexpr (N > 0) {
    rec<N - 1>();
  } else {
    sink(N);
  }
}

int main() { rec<2>(); }
