#ifndef _UTILS_H
#define _UTILS_H

#include <android/log.h>

#define LOG_PREFIX "[+][HackerLogger][Nikki]"
#define DEBUG_LOG(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_PREFIX, __VA_ARGS__)

#endif
