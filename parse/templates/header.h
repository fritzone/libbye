#ifndef _{{libname}}_proxy_header_h_
#define _{{libname}}_proxy_header_h_

#include <memory>
#include <dlfcn.h>

#include <iostream>

class {{libname}}_proxy
{

public:

    explicit {{libname}}_proxy(void* handle)
    {
## for function in functions

        m_ptr_{{ function.name }} = ( {{ function.return_type }} (*) ({{ function.parameters }}) ) dlsym( handle, "{{function.mangled}}" );
        if(! m_ptr_{{ function.name }} )
        {
            std::cerr << "cannot dlsym {{ function.name }} from {{ function.mangled }} :" << dlerror() << std::endl;
            exit(-1);
        }

## endfor
    }

    /**
     * Will load the given proxy library from the resource embedded in the application
     **/
    std::shared_ptr<{{libname}}_proxy> load();

private:

    // the function prototype pointers

## for function in functions
    // {{ function.name }}, {{ function.filename }}:{{ function.line }}
    {{ function.return_type }} (* m_ptr_{{ function.name }} ) ({{ function.parameters }}) = nullptr;

## endfor

};

#endif
