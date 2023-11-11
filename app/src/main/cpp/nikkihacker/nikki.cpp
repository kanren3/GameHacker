#include <jni.h>
#include <string>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include "utils.h"

jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    DEBUG_LOG("hello world");
    return JNI_VERSION_1_4;
}