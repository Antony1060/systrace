#ifndef __SYSCALL_UTIL
#define __SYSCALL_UTIL

#include<stdint.h>
#include<stdbool.h>
#include<ctype.h>
#include<stdio.h>
#include<sys/uio.h>

#include "util.h"

struct trace_info {
    pid_t pid;
    ptrace_syscall_entry *current_syscall_entry;
    ptrace_syscall_exit *current_syscall_exit;
};

typedef void (*fprint_syscall_args)(FILE *file, struct trace_info *trace, __u64 args[6]);

struct syscall_meta {
    const char *name;
    fprint_syscall_args writer;
};

void fprint_syscall_default(FILE *file, struct trace_info *trace, __u64 args[6]) {
    (void) trace;

    const size_t arg_count = 6;
    for (size_t i = 0; i < arg_count; i++) {
        fprintf(file, "%llu", args[i]);
        if (i != arg_count - 1)
            fprintf(file, ", ");
    }
}

void fprint_string_remote(FILE *file, struct trace_info *trace, char* str_remote, size_t len) {
    char str[len];
    struct iovec remote = { str_remote, len };
    struct iovec local = { str, len };

    if (process_vm_readv(trace->pid, &local, 1, &remote, 1, 0) < 0)
        errquit("process_vm_readv(pid)");

    const size_t LEN_MAX = 20;

    fprintf(file, "\"");
    size_t print_len = len <= LEN_MAX ? len : LEN_MAX - 3;
    for (size_t i = 0; i < print_len; i++) {
        if (isprint(str[i]) && str[i] != '"')
            fprintf(file, "%c", str[i]);
        else if (is_control(str[i]))
            fprintf(file, "%s", str_control(str[i]));
        else
            fprintf(file, "\\x%x", str[i]);
    }

    if (len > LEN_MAX)
        fprintf(file, "...");
    
    fprintf(file, "\"");
}

void fprint_syscall_write(FILE *file, struct trace_info *trace, __u64 args[6]) {
    const size_t arg_count = 3;
    for (size_t i = 0; i < arg_count; i++) {
        __u64 arg = args[i];
        switch (i) {
            case 0:
                fprintf(file, "%u", (unsigned int) arg);
                break;
            case 1:
                fprint_string_remote(file, trace, (char*) arg, args[2]);
                break;
            case 2:
                fprintf(file, "%zu", (size_t) arg);
                break;
        }

        if (i != arg_count - 1)
            fprintf(file, ", ");
    }
}

void fprint_syscall_read(FILE *file, struct trace_info *trace, __u64 args[6]) {
    const size_t arg_count = 3;
    for (size_t i = 0; i < arg_count; i++) {
        __u64 arg = args[i];
        switch (i) {
            case 0:
                fprintf(file, "%u", (unsigned int) arg);
                break;
            case 1:
                ptrace_syscall_exit *exit = trace->current_syscall_exit;
                fprint_string_remote(file, trace, (char*) arg, exit->is_error ? args[2] : (size_t) exit->rval);
                break;
            case 2:
                fprintf(file, "%zu", (size_t) arg);
                break;
        }

        if (i != arg_count - 1)
            fprintf(file, ", ");
    }
}

#ifdef __x86_64__
    #include "syscall_meta_x86_64.h"
#endif // __x86_64__

#endif // __SYSCALL_UTIL
