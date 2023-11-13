#include "utils.h"

uintptr_t get_module_from_name(const char *name)
{
    char line[512];
    uintptr_t result = 0;
    std::string module_info;

    FILE *fd = fopen("/proc/self/maps","r");

    if (nullptr != fd) {
        memset(line, 0, sizeof(line));

        while (fgets(line,sizeof(line),fd)) {

            if (strstr(line, name)) {

                module_info = line;
                module_info = module_info.substr(0,module_info.find('-'));
                std::istringstream iss(module_info);
                iss >> std::hex >> result;
                break;
            }

            memset(line, 0, sizeof(line));
        }
    }

    return result;
}