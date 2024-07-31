//在 C++ 中，在构造函数中调用虚函数会导致调用基类的实现，而不是派生类的实现。
//这是因为在构造函数期间，对象的类型是基类，虚函数表（vtable）还没有完全初始化为派生类的版本。
//因此，调用虚函数不会产生预期的效果，通常应该避免。

//这里有一个示例说明为什么在构造函数中调用虚函数是不建议的，以及如何通过设计避免这种情况：
#include <iostream>

class Base {
public:
    Base() {
        std::cout << "Base constructor called" << std::endl;
        virtualFunction();  // 不建议在构造函数中调用虚函数
    }

    virtual ~Base() {
        std::cout << "Base destructor called" << std::endl;
        virtualFunction();  // 可以在析构函数中调用虚函数，但也要小心
    }

    virtual void virtualFunction() const {
        std::cout << "Base implementation of virtualFunction" << std::endl;
    }
};

class Derived : public Base {
public:
    Derived() {
        std::cout << "Derived constructor called" << std::endl;
    }

    ~Derived() {
        std::cout << "Derived destructor called" << std::endl;
    }

    void virtualFunction() const override {
        std::cout << "Derived implementation of virtualFunction" << std::endl;
    }
};

int main() {
    Derived d;
    return 0;
}
//代码解释
/*Base 类

Base 类有一个构造函数和一个析构函数。它们都调用了一个虚函数 virtualFunction。
virtualFunction 在 Base 类中有一个默认实现。
Derived 类

Derived 类继承自 Base 并覆盖了 virtualFunction。
Derived 类的构造函数和析构函数中没有额外的逻辑。

当 Derived 对象 d 被创建时，首先调用 Base 类的构造函数。
这时调用 virtualFunction 会执行 Base 类中的实现，因为 Derived 类的构造函数还没有执行，虚表还没有指向 Derived 类的实现。
然后调用 Derived 类的构造函数。
当对象销毁时，首先调用 Derived 类的析构函数，然后调用 Base 类的析构函数。
在 Base 类的析构函数中调用 virtualFunction 时，同样会执行 Base 类中的实现，因为此时对象已经开始销毁，虚表指向 Base 类的实现。
*/

//为了避免在构造函数中调用虚函数，可以采用不同的方法，例如延迟初始化或在构造函数中明确调用特定的函数，而不是虚函数。以下是一个改进示例：
class Base {
public:
    Base() {
        std::cout << "Base constructor called" << std::endl;
        nonVirtualFunction();
    }

    virtual ~Base() {
        std::cout << "Base destructor called" << std::endl;
    }

    void nonVirtualFunction() const {
        std::cout << "Base specific initialization" << std::endl;
        customInitialization();
    }

    virtual void customInitialization() const {
        std::cout << "Base implementation of customInitialization" << std::endl;
    }
};

class Derived : public Base {
public:
    Derived() {
        std::cout << "Derived constructor called" << std::endl;
    }

    ~Derived() {
        std::cout << "Derived destructor called" << std::endl;
    }

    void customInitialization() const override {
        std::cout << "Derived implementation of customInitialization" << std::endl;
    }
};

int main() {
    Derived d;
    return 0;
}
//改进后的代码解释
/*
在 Base 类中引入了一个非虚函数 nonVirtualFunction，用于在构造函数中调用。
nonVirtualFunction 可以在构造过程中执行一些非虚的特定初始化逻辑。
nonVirtualFunction 调用一个虚函数 customInitialization，但是这个虚函数是在对象完全初始化后由派生类实现的。
通过这种方法，避免了在构造函数中直接调用虚函数，从而防止了意外行为。
*/