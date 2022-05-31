
int main(int argc, char **argv){
  const auto l = [](int a, int b){
    float alpha = a / (1.0 * b);
    double delta = .0;
    for (int i = 0; i < 4; ++i){
      delta = a * i * alpha;
    }
    return delta;
  };
  auto d = l(2, 4);
  return 0;
}