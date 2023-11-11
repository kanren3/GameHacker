#include <jni.h>
#include <string>
#include <sstream>
#include <unordered_map>
#include <dobby.h>
#include <unwind.h>
#include <dlfcn.h>
#include "utils.h"

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

        DEBUG_LOG("stack walk:%s+0x%p", filename.c_str(), (void*)(pc - (uintptr_t)info.dli_fbase));
    }

    return _URC_NO_REASON;
}

std::unordered_map<FILE *, std::string> file_logger;

FILE *(*org_fopen)(const char *filepath, const char *modes);
FILE *fake_fopen(const char *filepath, const char *modes)
{
    FILE *result = org_fopen(filepath, modes);

    if (nullptr != result) {
        std::string filepath_string = filepath;
        file_logger.insert({ result, filepath_string});
    }

    return result;
}

int (*org_fclose)(FILE *fp);
int fake_fclose(FILE *fp)
{
    int result;

    result = org_fclose(fp);

    if (0 == result) {
        file_logger.erase(fp);
    }

    return result;
}

size_t (*org_fread)(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fake_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t result;

    result = org_fread(ptr, size, nmemb, stream);

    std::string file_path = file_logger[stream];

    if (!file_path.empty()) {
        if (file_path.find(".zip") != std::string::npos) {
            DEBUG_LOG("fread:%s", file_path.c_str());

            DEBUG_LOG("=============stack walk start=============");
            uint32_t counter = 0;
            _Unwind_Backtrace(unwind_callback, &counter);
            DEBUG_LOG("=============stack walk end=============");
        }
    }

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
    }

    return result;
}

jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
//    void *android_dlopen_ext_address = DobbySymbolResolver(nullptr, "android_dlopen_ext");
//    DobbyHook(android_dlopen_ext_address, (dobby_dummy_func_t)fake_android_dlopen_ext, (dobby_dummy_func_t*)&org_android_dlopen_ext);

    DEBUG_LOG("Hello!");

    void *fopen_address = DobbySymbolResolver(nullptr, "fopen");
    DobbyHook(fopen_address, (dobby_dummy_func_t)fake_fopen, (dobby_dummy_func_t*)&org_fopen);

    void *fclose_address = DobbySymbolResolver(nullptr, "fclose");
    DobbyHook(fclose_address, (dobby_dummy_func_t)fake_fclose, (dobby_dummy_func_t*)&org_fclose);

    void *fread_address = DobbySymbolResolver(nullptr, "fread");
    DobbyHook((void*)fread_address, (dobby_dummy_func_t)fake_fread, (dobby_dummy_func_t*)&org_fread);

    return JNI_VERSION_1_6;
}