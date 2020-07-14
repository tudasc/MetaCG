
void x() {}
void y() {}
void z() {}

void a() { x(); z(); }
void b() { y(); z(); }
void c() { x(); y(); }


int main() {

	a();
	b();
	c();

	return 0;
}
