#include <string>
#include <testlib_proxy_header.h>

int main(int argc, char *argv[])
{
    std::shared_ptr<testlib_proxy> p = testlib_proxy::load();
    p->examplefunc();

    return 0;
}
