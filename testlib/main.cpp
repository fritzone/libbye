#include <string>

int examplefunc()
{
    return 42;
}

std::string return_string(double a, int b)
{
    return std::to_string(a) + "ABCD" + std::to_string(b);
}

template<class T> T something_templated(T t)
{
    return T{t};
}

static char infunc()
{
    return something_templated<int>(42);
}

class AClass
{
public:
    AClass() = default;
    void doSomething(float f) {std::string s = ::return_string(f, 32);}
};
