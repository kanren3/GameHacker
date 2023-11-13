#include <jni.h>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <dobby.h>
#include <unwind.h>
#include <dlfcn.h>
#include "utils.h"

std::string (*org_onGetStringFromFile)(void *a1, void *path);
std::string fake_onGetStringFromFile(void *a1, std::string *path)
{
    std::string result = org_onGetStringFromFile(a1, path);

    if (!result.empty()) {
        std::string js_name = *path;
        std::string js_path = *path;

        if (std::string::npos != js_name.rfind('/')) {
            js_name = js_name.substr(js_name.rfind('/') + 1);
            js_path = js_path.substr(0, js_path.rfind('/') + 1);
        }
        else{
            js_path.clear();
        }

        try {
            std::string export_path = std::string("/data/data/com.tencent.game.rhythmmaster/files/js_export/") + js_path;

            if (!std::__fs::filesystem::exists(export_path) ||
                std::__fs::filesystem::is_directory(export_path)) {
                std::__fs::filesystem::create_directories(export_path);
            }

            export_path += js_name;

            std::ofstream ostream(export_path, std::ios::binary);

            if (ostream.is_open()) {
                ostream.write(result.data(), (std::streamsize)result.size());
                ostream.close();
            }
            DEBUG_LOG("export js succeed:%s", path->c_str());
        }
        catch (const std::exception& e) {
            DEBUG_LOG("export js failed:%s", path->c_str());
        }

        std::string js_data;

        std::string import_path = std::string("/data/data/com.tencent.game.rhythmmaster/files/js_import/") + js_path + js_name;
        std::ifstream istream(import_path);

        if (istream.is_open()) {
            js_data.assign(std::istreambuf_iterator<char>(istream), std::istreambuf_iterator<char>());
            istream.close();

            if (!js_data.empty()) {
                DEBUG_LOG("import js:%s", import_path.c_str());
                return js_data;
            }
            else {
                return result;
            }
        }
        else {
            return result;
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

        if (filename == "libcocos2djs.so") {
            uintptr_t libcocos2djs_base = get_module_from_name("libcocos2djs.so");
            uintptr_t onGetStringFromFile_address = libcocos2djs_base + 0x88F1C8;
            DEBUG_LOG("onGetStringFromFile_address:%p", (void *)onGetStringFromFile_address);
            DobbyHook((void *)onGetStringFromFile_address, (dobby_dummy_func_t)fake_onGetStringFromFile, (dobby_dummy_func_t *)&org_onGetStringFromFile);
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