#include "{{libname}}_proxy_header.h"
#include <{{libname}}_resource.h>

#include <resource.h>
#include <fpaq0.h>

#include <memory>
#include <stdio.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <unistd.h>

// opens the ramfs for the given file
static int open_ramfs(void)
{
    std::string shm_name { "shmf_{{libname}}" };

    int shm_fd = memfd_create(shm_name.c_str(), MFD_CLOEXEC);
    if (shm_fd < 0)
    {
        exit(-1);
    }
    return shm_fd;
}

// Load the shared object
static void* load_so(int shm_fd)
{
#define PATH_LEN 1024
    char path[PATH_LEN] = {0};
    void *handle = nullptr;

    std::string p = "/proc";

    std::string full_path = "%s/%d/fd/%d";

#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    snprintf(path, PATH_LEN, full_path.c_str(), p.c_str(), getpid(), shm_fd);
#pragma GCC diagnostic warning "-Wformat-nonliteral"

    handle = dlopen(path, RTLD_LAZY);
    if (!handle)
    {
        fprintf(stderr, "cannot dlopen: %s\n", dlerror());
        exit(-1);
    }
    return handle;
}

#include <fstream>
std::shared_ptr<{{libname}}_proxy> {{libname}}_proxy::load()
{
    static std::shared_ptr<{{libname}}_proxy> result = nullptr;
    if(result)
    {
        return result;
    }

    resourcer::resource resdata = LOAD_RESOURCE({{libname}});
    std::vector<unsigned char> v(resdata.begin(), resdata.end());

    int shm_fd = open_ramfs();

    auto decompressed = compressor::decompress(v);

    std::ofstream out("output.decompr.1", std::ios::out | std::ios::binary);
    out.write((char*)&decompressed[0], decompressed.size());
    out.close();
    std::cout << "wrote " << decompressed.size() << " bytes" << std::endl;

    if(write(shm_fd, decompressed.data(), decompressed.size()) < 0)
    {
        std::cerr << "write to shared memory failed";
        exit(-1);
    }

    void* handle = load_so(shm_fd);
    result = std::make_shared<{{libname}}_proxy>(handle);

    return result;
}

## for function in functions
{{ function.return_type }} {{libname}}_proxy::{{ function.name }} ( {{function.fundef}} )
{
    {% if function.return_type != "void" %} return {% endif %} (*m_ptr_{{ function.name }})( {{ function.call }} );
}

## endfor

