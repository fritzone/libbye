#pragma once

#include <cstddef>
#include <string>

namespace resourcer
{
class resource
{
public:
    resource(const unsigned char* start, const size_t len) : resource_data(start), data_len(len) {}

    const unsigned char * const &data() const { return resource_data; }
    const size_t &size() const { return data_len; }

    const unsigned char *begin() const { return resource_data; }
    const unsigned char *end() const { return resource_data + data_len; }

    std::string to_string() { return std::string(reinterpret_cast<const char*>(data()), size()); }

private:
    const unsigned char* resource_data;
    const size_t data_len;
};

#define LOAD_RESOURCE(RESOURCE) ([]() {                                              \
        return resourcer::resource(resourcer::resource_##RESOURCE,resourcer::resource_##RESOURCE##_len);  \
    })()
}
