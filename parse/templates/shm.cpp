#include <{{libname}}_proxy_header.h>
#include <memory>
#include <resource.h>

std::shared_ptr<{{libname}}_proxy> {{libname}}_proxy::load()
{
    static std::shared_ptr<{{libname}}_proxy> result = nullptr;
    if(result)
    {
        return result;
    }

    if ((outfile = fmemopen(buffer, static_cast<size_t>(size), "wb")) == NULL)
    {
        qFatal("fmemopen failed");
    }

    resource text = LOAD_RESOURCE(frag_glsl);

    int shm_fd = open_ramfs();

    if(write(shm_fd, buffer, size) < 0)
    {
        qFatal("write failed");
    }

    void* handle = load_so(shm_fd);
    result = std::make_shared<chameleon::tongue>(handle, buffer);
    qf.close();

    return result;
}
