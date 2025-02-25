void leaf() {
  while (true) {
    break;
  }
}

void loop();

void x() {
  leaf();
  loop();
}

void left() {
  while (true) {
    x();
    break;
  }
}

void right() {
  while (true) {
    while (true) {
      x();
      break;
    }
    break;
  }
}

void split() {
  left();
  right();
}

void entry() {
  while (true) {
    while (true) {
      left();
      entry();
      break;
    }
    break;
  }
}

void loop() {
  while (true) {
    split();
    break;
  }
}

int main() {
  split();
  return 0;
}
