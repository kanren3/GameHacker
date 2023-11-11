#include <jni.h>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <dobby.h>
#include <unwind.h>
#include <dlfcn.h>
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

_Unwind_Reason_Code unwind_callback(_Unwind_Context *context, void *data)
{
    Dl_info info;
    std::string filename;
    uint32_t *counter;

    if (nullptr == context) {
        return _URC_END_OF_STACK;
    }

    _Unwind_Word pc = _Unwind_GetIP(context);

    if (0 == pc) {
        return _URC_END_OF_STACK;
    }

    counter = (uint32_t *)data;

    if (*counter == 5) {
        return _URC_END_OF_STACK;
    }

    *counter += 1;

    if (dladdr((void *)pc, &info)) {
        filename = info.dli_fname;

        if (std::string::npos != filename.rfind('/')) {
            filename = filename.substr(filename.rfind('/') + 1);
        }

        DEBUG_LOG("stack walk:%s+0x%p(%p)", filename.c_str(), (void*)(pc - (uintptr_t)info.dli_fbase), (void *)pc);
    }

    return _URC_NO_REASON;
}

void create_so_dump(const char *name)
{
    char line[512];
    std::string module_info;
    std::string start_string;
    std::string end_string;
    uintptr_t start_address;
    uintptr_t end_address;
    uintptr_t last_address = 0;

    std::string dump_path = std::string("/data/data/com.shining.nikki4.tw/files/") + name + ".dump";
    std::ofstream ostream(dump_path, std::ios::binary);

    if (ostream.is_open()) {
        FILE *fd = fopen("/proc/self/maps","r");

        if (nullptr != fd) {
            memset(line, 0, sizeof(line));

            while (fgets(line,sizeof(line),fd)) {

                if (strstr(line, name)) {
                    module_info = line;

                    start_string = module_info.substr(0, module_info.find('-'));
                    std::istringstream start_stream(start_string);
                    start_stream >> std::hex >> start_address;

                    end_string = module_info.substr(module_info.find('-') + 1, module_info.find(' '));
                    std::istringstream end_stream(end_string);
                    end_stream >> std::hex >> end_address;

                    DEBUG_LOG("dump start_address:%p", (void *)start_address);
                    DEBUG_LOG("dump end_address:%p", (void *)end_address);

                    if (0 != last_address && (start_address > last_address)) {
                        for (uintptr_t i = 0; i < (start_address - last_address); i++) {
                            ostream.put('\0');
                        }
                    }

                    if (ostream.is_open()) {
                        ostream.write((char*)start_address, std::streamsize(end_address - start_address));
                    }

                    last_address = end_address;
                }

                memset(line, 0, sizeof(line));
            }
            fclose(fd);
        }
        ostream.close();
    }
}

int (*org_lua_loadx)(void *L, void *reader, void *data, const char *chunkname, const char *mode);
int fake_lua_loadx(void *L, void *reader, void *data, const char *chunkname, const char *mode)
{
    int result;
    Dl_info info;

    if (nullptr != reader) {
        if (dladdr(reader, &info)) {
            std::string filename = info.dli_fname;

            if (std::string::npos != filename.rfind('/')) {
                filename = filename.substr(filename.rfind('/') + 1);
            }

            DEBUG_LOG("lua_loadx reader:%s+0x%p", filename.c_str(), (void*)((uintptr_t)reader - (uintptr_t)info.dli_fbase));
        }
    }

    DEBUG_LOG("lua_loadx chunkname:%s", chunkname);

    DEBUG_LOG("=============stack walk start=============");
    uint32_t counter = 0;
    _Unwind_Backtrace(unwind_callback, &counter);
    DEBUG_LOG("=============stack walk end=============");

    result = org_lua_loadx(L, reader, data, chunkname, mode);

    return result;
}

uint64_t lua_counter = 0;

bool isSpecialChar(char c) {
    return !std::isalnum(static_cast<unsigned char>(c));
}

int (*org_luaL_loadbufferx)(void *L, const char *buf, size_t size, const char *name, const char *mode);
int fake_luaL_loadbufferx(void *L, const char *buf, size_t size, const char *name, const char *mode)
{
    int result;

    if (nullptr != buf && nullptr != name) {
        if (lua_counter == 5) {
            DEBUG_LOG("=============stack walk start=============");
            uint32_t counter = 0;
            _Unwind_Backtrace(unwind_callback, &counter);
            DEBUG_LOG("=============stack walk end=============");
            create_so_dump("libil2cpp.so");
        }

        lua_counter += 1;

        std::string data = std::string(buf, size);
        std::string lua_name = name;

        std::replace_if(lua_name.begin(), lua_name.end(), isSpecialChar, '-');

        std::string out_path = std::string("/data/data/com.shining.nikki4.tw/files/lua/") + lua_name + ".lua";

        std::ofstream ostream(out_path, std::ios::binary);

        if (ostream.is_open()) {
            ostream.write(data.data(), (std::streamsize)data.size());
            ostream.put('\n');
            ostream.close();
            lua_counter += 1;
        }
    }

    result = org_luaL_loadbufferx(L, buf, size, name, mode);

    return result;
}

void *(*org_android_dlopen_ext)(const char *filepath, int flags, void *extinfo);
void *fake_android_dlopen_ext(const char *filepath, int flags, void *extinfo) {

    void* result = org_android_dlopen_ext(filepath, flags, extinfo);

    if (nullptr != result) {
        std::string filename = filepath;

        if (std::string::npos != filename.rfind('/')) {
            filename = filename.substr(filename.rfind('/') + 1);
        }

        DEBUG_LOG("dlopen so:%s", filename.c_str());

        if (filename == "libtolua.so") {
            uintptr_t libtolua_base = get_module_from_name("libtolua.so");
 //           uintptr_t lua_loadx_address = libtolua_base + 0x609F8;
 //           DobbyHook((void *)lua_loadx_address, (dobby_dummy_func_t)fake_lua_loadx, (dobby_dummy_func_t *)&org_lua_loadx);
 //           DEBUG_LOG("lua_loadx_address:%p", (void *)lua_loadx_address);

            uintptr_t luaL_loadbufferx_address = libtolua_base + 0x60C80;
            DobbyHook((void *)luaL_loadbufferx_address, (dobby_dummy_func_t)fake_luaL_loadbufferx, (dobby_dummy_func_t *)&org_luaL_loadbufferx);
            DEBUG_LOG("luaL_loadbufferx_address:%p", (void *)luaL_loadbufferx_address);
        }
    }

    return result;
}

jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    DEBUG_LOG("Hello!");

    void *android_dlopen_ext_address = DobbySymbolResolver(nullptr, "android_dlopen_ext");
    DobbyHook(android_dlopen_ext_address, (dobby_dummy_func_t)fake_android_dlopen_ext, (dobby_dummy_func_t*)&org_android_dlopen_ext);

    return JNI_VERSION_1_6;
}