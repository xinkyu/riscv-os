#ifndef __KLOG_H__
#define __KLOG_H__

#include "riscv.h"
#define KLOG_BUFFER_SIZE    256
#define KLOG_MESSAGE_MAX    160
#define KLOG_COMPONENT_MAX  16
// Logging levels (ascending verbosity)
enum klog_level {
    KLOG_LEVEL_TRACE = 0,
    KLOG_LEVEL_DEBUG,
    KLOG_LEVEL_INFO,
    KLOG_LEVEL_WARN,
    KLOG_LEVEL_ERROR,
    KLOG_LEVEL_FATAL,
};

struct klog_stats {
    uint64 total_generated;
    uint64 stored;
    uint64 overwritten;
    uint64 console_emitted;
    int buffered_entries;
    enum klog_level buffer_level;
    enum klog_level console_level;
    int console_enabled;
};

void klog_init(void);
void klog_write(enum klog_level level, const char *component, const char *fmt, ...);
void klog_set_buffer_level(enum klog_level level);
void klog_set_console_level(enum klog_level level);
void klog_enable_console(int enable);
void klog_dump_recent(int max_entries);
void klog_summary(void);
void klog_get_stats(struct klog_stats *stats);
int klog_read(uint64 user_buf, int n);

#define KLOG_TRACE(component, fmt, ...) \
    klog_write(KLOG_LEVEL_TRACE, component, fmt, ##__VA_ARGS__)
#define KLOG_DEBUG(component, fmt, ...) \
    klog_write(KLOG_LEVEL_DEBUG, component, fmt, ##__VA_ARGS__)
#define KLOG_INFO(component, fmt, ...) \
    klog_write(KLOG_LEVEL_INFO, component, fmt, ##__VA_ARGS__)
#define KLOG_WARN(component, fmt, ...) \
    klog_write(KLOG_LEVEL_WARN, component, fmt, ##__VA_ARGS__)
#define KLOG_ERROR(component, fmt, ...) \
    klog_write(KLOG_LEVEL_ERROR, component, fmt, ##__VA_ARGS__)
#define KLOG_FATAL(component, fmt, ...) \
    klog_write(KLOG_LEVEL_FATAL, component, fmt, ##__VA_ARGS__)

#endif // __KLOG_H__
