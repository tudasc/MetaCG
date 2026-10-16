struct Base {
    virtual void foo() {}
};

struct A : Base {
    virtual void foo() override;
};