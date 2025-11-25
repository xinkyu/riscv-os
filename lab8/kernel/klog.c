#include <stdarg.h>
#include "defs.h"
#include "klog.h"

#define KLOG_BUFFER_SIZE    256
#define KLOG_MESSAGE_MAX    160
#define KLOG_COMPONENT_MAX  16

struct klog_record {
    uint64 timestamp;
    int level;
    int cpu;
    int pid;
    char component[KLOG_COMPONENT_MAX];
    char message[KLOG_MESSAGE_MAX];
};

struct klog_state {
    struct spinlock lock;
    struct klog_record buffer[KLOG_BUFFER_SIZE];
    int next;
    int count;
    enum klog_level buffer_level;
    enum klog_level console_level;
    int console_enabled;
    int initialized;
    uint64 total_generated;
    uint64 stored;
    uint64 overwritten;
    uint64 console_emitted;
};

static struct klog_state klog;

static const char *level_names[] = {
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

static int clamp_level(enum klog_level level) {
    if (level < KLOG_LEVEL_TRACE) return KLOG_LEVEL_TRACE;
    if (level > KLOG_LEVEL_FATAL) return KLOG_LEVEL_FATAL;
    return level;
}

static void copy_string(char *dst, int max, const char *src) {
    if (!src) {
        src = "kernel";
    }
    int i = 0;
    for (; i < max - 1 && src[i]; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

static void append_char(char **ptr, int *remaining, char c) {
    if (*remaining <= 1) {
        if (*remaining == 1) {
            **ptr = '\0';
            (*remaining)--;
        }
        return;
    }
    **ptr = c;
    (*ptr)++;
    (*remaining)--;
}

static void append_string(char **ptr, int *remaining, const char *s) {
    if (!s) {
        s = "(null)";
    }
    while (*s && *remaining > 1) {
        append_char(ptr, remaining, *s++);
    }
}

static void append_uint(char **ptr, int *remaining, unsigned long long x, int base, int prefix0x) {
    char buf[32];
    int i = 0;
    if (x == 0) {
        buf[i++] = '0';
    } else {
        while (x && i < (int)sizeof(buf)) {
            int digit = x % base;
            buf[i++] = "0123456789abcdef"[digit];
            x /= base;
        }
    }
    if (prefix0x && *remaining > 2) {
        append_char(ptr, remaining, '0');
        append_char(ptr, remaining, 'x');
    }
    while (i > 0 && *remaining > 1) {
        append_char(ptr, remaining, buf[--i]);
    }
}

static void append_int(char **ptr, int *remaining, long long v) {
    if (v < 0) {
        append_char(ptr, remaining, '-');
        append_uint(ptr, remaining, (unsigned long long)(-v), 10, 0);
    } else {
        append_uint(ptr, remaining, (unsigned long long)v, 10, 0);
    }
}

static void kvsnprintf(char *dst, int max, const char *fmt, va_list ap) {
    char *ptr = dst;
    int remaining = max;
    if (max <= 0) return;

    while (*fmt && remaining > 1) {
        if (*fmt != '%') {
            append_char(&ptr, &remaining, *fmt++);
            continue;
        }
        fmt++;
        char spec = *fmt++;
        if (spec == '\0') break;
        switch (spec) {
            case 's': {
                const char *s = va_arg(ap, const char*);
                append_string(&ptr, &remaining, s);
                break;
            }
            case 'c':
                append_char(&ptr, &remaining, (char)va_arg(ap, int));
                break;
            case 'd':
                append_int(&ptr, &remaining, va_arg(ap, int));
                break;
            case 'u':
                append_uint(&ptr, &remaining, va_arg(ap, unsigned int), 10, 0);
                break;
            case 'x':
                append_uint(&ptr, &remaining, va_arg(ap, unsigned int), 16, 0);
                break;
            case 'p':
                append_uint(&ptr, &remaining, va_arg(ap, uint64), 16, 1);
                break;
            case 'l': {
                char next = *fmt++;
                if (next == 'd') {
                    append_int(&ptr, &remaining, va_arg(ap, long));
                } else if (next == 'u') {
                    append_uint(&ptr, &remaining, va_arg(ap, unsigned long), 10, 0);
                } else if (next == 'x') {
                    append_uint(&ptr, &remaining, va_arg(ap, unsigned long), 16, 0);
                } else {
                    append_char(&ptr, &remaining, '%');
                    append_char(&ptr, &remaining, 'l');
                    append_char(&ptr, &remaining, next);
                }
                break;
            }
            case '%':
                append_char(&ptr, &remaining, '%');
                break;
            default:
                append_char(&ptr, &remaining, '%');
                append_char(&ptr, &remaining, spec);
                break;
        }
    }
    if (remaining > 0) {
        *ptr = '\0';
    } else {
        dst[max - 1] = '\0';
    }
}

static void print_record(const struct klog_record *rec) {
    printf("[%s][time=%lu][cpu=%d pid=%d][%s] %s\n",
           level_names[clamp_level(rec->level)],
           rec->timestamp,
           rec->cpu,
           rec->pid,
           rec->component,
           rec->message);
}

void klog_init(void) {
    if (klog.initialized) {
        return;
    }
    spinlock_init(&klog.lock, "klog");
    klog.next = 0;
    klog.count = 0;
    klog.buffer_level = KLOG_LEVEL_TRACE;
    klog.console_level = KLOG_LEVEL_WARN;
    klog.console_enabled = 1;
    klog.total_generated = 0;
    klog.stored = 0;
    klog.overwritten = 0;
    klog.console_emitted = 0;
    klog.initialized = 1;
    printf("[klog] initialized (buffer=%d, console-level=%d)\n", KLOG_BUFFER_SIZE, klog.console_level);
}

void klog_write(enum klog_level level, const char *component, const char *fmt, ...) {
    if (!klog.initialized || fmt == 0) {
        return;
    }

    struct klog_record rec;
    rec.timestamp = get_time();
    rec.level = clamp_level(level);

    struct cpu *cpu = mycpu();
    if (cpu) {
        int idx = (int)(cpu - cpus);
        if (idx >= 0 && idx < NCPU) {
            rec.cpu = idx;
        } else {
            rec.cpu = -1;
        }
    } else {
        rec.cpu = -1;
    }

    struct proc *p = myproc();
    rec.pid = p ? p->pid : -1;
    copy_string(rec.component, KLOG_COMPONENT_MAX, component);

    va_list args;
    va_start(args, fmt);
    kvsnprintf(rec.message, KLOG_MESSAGE_MAX, fmt, args);
    va_end(args);

    int should_console = 0;

    acquire(&klog.lock);
    klog.total_generated++;

    if (rec.level >= klog.buffer_level) {
        if (klog.count == KLOG_BUFFER_SIZE) {
            klog.overwritten++;
        } else {
            klog.count++;
        }
        klog.buffer[klog.next] = rec;
        klog.next = (klog.next + 1) % KLOG_BUFFER_SIZE;
        klog.stored++;
    }

    if (klog.console_enabled && rec.level >= klog.console_level) {
        should_console = 1;
    }
    release(&klog.lock);

    if (should_console) {
        print_record(&rec);
        acquire(&klog.lock);
        klog.console_emitted++;
        release(&klog.lock);
    }
}

void klog_set_buffer_level(enum klog_level level) {
    if (!klog.initialized) return;
    acquire(&klog.lock);
    klog.buffer_level = clamp_level(level);
    release(&klog.lock);
}

void klog_set_console_level(enum klog_level level) {
    if (!klog.initialized) return;
    acquire(&klog.lock);
    klog.console_level = clamp_level(level);
    release(&klog.lock);
}

void klog_enable_console(int enable) {
    if (!klog.initialized) return;
    acquire(&klog.lock);
    klog.console_enabled = enable ? 1 : 0;
    release(&klog.lock);
}

void klog_dump_recent(int max_entries) {
    if (!klog.initialized) return;
    if (max_entries <= 0 || max_entries > KLOG_BUFFER_SIZE) {
        max_entries = KLOG_BUFFER_SIZE;
    }

    acquire(&klog.lock);
    int available = klog.count;
    if (available > max_entries) {
        available = max_entries;
    }
    int start = (klog.next - available + KLOG_BUFFER_SIZE) % KLOG_BUFFER_SIZE;
    for (int i = 0; i < available; i++) {
        int idx = (start + i) % KLOG_BUFFER_SIZE;
        struct klog_record rec = klog.buffer[idx];
        release(&klog.lock);
        print_record(&rec);
        acquire(&klog.lock);
    }
    release(&klog.lock);
}

void klog_summary(void) {
    if (!klog.initialized) {
        printf("[klog] not initialized\n");
        return;
    }
    acquire(&klog.lock);
    printf("[klog] total=%lu stored=%lu overwritten=%lu console=%lu buffer_level=%d console_level=%d enabled=%d size=%d\n",
           klog.total_generated,
           klog.stored,
           klog.overwritten,
           klog.console_emitted,
           klog.buffer_level,
           klog.console_level,
           klog.console_enabled,
           klog.count);
    release(&klog.lock);
}

void klog_get_stats(struct klog_stats *stats) {
    if (!stats) return;
    if (!klog.initialized) {
        stats->total_generated = 0;
        stats->stored = 0;
        stats->overwritten = 0;
        stats->console_emitted = 0;
        stats->buffered_entries = 0;
        stats->buffer_level = KLOG_LEVEL_TRACE;
        stats->console_level = KLOG_LEVEL_INFO;
        stats->console_enabled = 0;
        return;
    }

    acquire(&klog.lock);
    stats->total_generated = klog.total_generated;
    stats->stored = klog.stored;
    stats->overwritten = klog.overwritten;
    stats->console_emitted = klog.console_emitted;
    stats->buffered_entries = klog.count;
    stats->buffer_level = klog.buffer_level;
    stats->console_level = klog.console_level;
    stats->console_enabled = klog.console_enabled;
    release(&klog.lock);
}
