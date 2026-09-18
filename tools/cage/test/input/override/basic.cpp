struct Base {
    virtual void foo();
};

struct Derived : Base {
    void foo() override;
};

void Base::foo() {}
void Derived::foo() {}

int main() {
    Base* b = new Derived();
    b->foo();
    return 1;
}
