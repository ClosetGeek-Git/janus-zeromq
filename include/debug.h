/*! \file   debug.h (Mock for testing)
 * \brief  Debug and logging macros (mock)
 * This is a simplified mock version for build verification only.
 */

#ifndef JANUS_DEBUG_H
#define JANUS_DEBUG_H

#include <stdio.h>

/* Log levels */
#define LOG_NONE     0
#define LOG_FATAL    1
#define LOG_ERR      2
#define LOG_WARN     3
#define LOG_INFO     4
#define LOG_VERB     5
#define LOG_HUGE     6
#define LOG_DBG      7

/* Simple logging macros */
#define JANUS_LOG(level, fmt, ...) \
    do { \
        const char *level_str = "LOG"; \
        if (level == LOG_FATAL) level_str = "FATAL"; \
        else if (level == LOG_ERR) level_str = "ERROR"; \
        else if (level == LOG_WARN) level_str = "WARN"; \
        else if (level == LOG_INFO) level_str = "INFO"; \
        else if (level == LOG_VERB) level_str = "VERB"; \
        else if (level == LOG_HUGE) level_str = "HUGE"; \
        else if (level == LOG_DBG) level_str = "DEBUG"; \
        fprintf(stderr, "[%s] " fmt, level_str, ##__VA_ARGS__); \
    } while(0)

#endif
