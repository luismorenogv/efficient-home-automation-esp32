/**
 * @file Logger.h
 * @brief Definition of logging macros 
 * 
 * 
 * @author Luis Moreno
 * @date Dec 12, 2024
 */
#include "config.h"

#if ENABLE_LOGGING

    #define LOG_ERROR(fmt, ...) \
        Serial.printf("[ERROR] " fmt "\r\n", ##__VA_ARGS__)

    #define LOG_WARNING(fmt, ...) \
        Serial.printf("[WARNING] " fmt "\r\n", ##__VA_ARGS__)

    #define LOG_INFO(fmt, ...) \
        Serial.printf("[INFO] " fmt "\r\n", ##__VA_ARGS__)

#else

    // If logging is disabled, define empty macros
    #define LOG_ERROR(fmt, ...)
    #define LOG_WARNING(fmt, ...)
    #define LOG_INFO(fmt, ...)

#endif