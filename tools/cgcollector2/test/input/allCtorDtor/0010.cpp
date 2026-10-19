// Aggregate initialization does not call a constructor, so no constructor node or edge must be generated

struct Agg {
  int x;
  void use() const {}
};

int main() {
  Agg a{1};
  a.use();
  Agg{}.use();
}
