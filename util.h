#ifndef __STRACE_UTIL
#define __STRACE_UTIL

#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<linux/ptrace.h>

typedef typeof(((struct ptrace_syscall_info *)0)->entry) ptrace_syscall_entry;
typedef typeof(((struct ptrace_syscall_info *)0)->exit) ptrace_syscall_exit;

#define errquit(s) do { \
    fprintf(stderr, "ERROR: "s": %s (%s)\n", strerror(errno), strerrorname_np(errno)); \
    exit(1); \
} while(0);

bool is_control(char c) {
    switch (c) {
        case '\0':
        case '\a':
        case '\b':
        case '\t':
        case '\n':
        case '\v':
        case '\f':
        case '\r':
        case '\\':
            return true;
    }
    return false;
}

const char* str_control(char c) {
    switch (c) {
        case '\0':
            return "\\0";
        case '\a':
            return "\\a";
        case '\b':
            return "\\b";
        case '\t':
            return "\\t";
        case '\n':
            return "\\n";
        case '\v':
            return "\\v";
        case '\f':
            return "\\f";
        case '\r':
            return "\r";
        case '\\':
            return "\\";
    }
    return NULL;
}


#endif // __STRACE_UTIL
