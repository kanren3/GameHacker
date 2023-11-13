#ifndef _UTILS_H
#define _UTILS_H

#include <jni.h>
#include <string>
#include <sstream>
#include <android/log.h>

#define LOG_PREFIX "[+][HackerLogger][RhythmMaster]"
#define DEBUG_LOG(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_PREFIX, __VA_ARGS__)

uintptr_t get_module_from_name(const char *name);

#endif
