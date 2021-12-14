float foo(float a, int b) { return a * b; }

float g;
double h;

void baa(float *arr, int count) {
  for (int i = 0; i < count; ++i) {
    g *= arr[i];
  }
  h++;
}
