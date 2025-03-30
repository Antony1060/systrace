#define _GNU_SOURCE

#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/ptrace.h>
#include<linux/ptrace.h>
#include<stdbool.h>

#include "util.h"
#include "syscall_util.h"

ptrace_syscall_entry *dump_syscall(struct trace_info *trace, struct ptrace_syscall_info *syscall) {
    ptrace_syscall_entry *entry;

    switch (syscall->op) {
        case PTRACE_SYSCALL_INFO_ENTRY:
            return &syscall->entry;
        case PTRACE_SYSCALL_INFO_EXIT:
            typeof(syscall->exit) exit = syscall->exit;

            entry = trace->current_syscall_entry;
            if (entry == NULL)
                break;

            trace->current_syscall_exit = &exit;
            struct syscall_meta meta = syscall_meta_table[entry->nr]; 
            fprintf(stderr, "%s(", meta.name);
            meta.writer(stderr, trace, entry->args);
            fprintf(stderr, ")");

            fprintf(stderr, " = ");
            if (exit.is_error)
                fprintf(stderr, "%lld, %s (%s)\n", exit.rval, strerrorname_np(-exit.rval), strerror(-exit.rval));
            else
                fprintf(stderr, "%lld\n", exit.rval);
            
            break;
        case PTRACE_SYSCALL_INFO_SECCOMP:
            fprintf(stderr, "syscall seccomp\n");
            break;
        case PTRACE_SYSCALL_INFO_NONE:
            fprintf(stderr, "syscall none\n");
            break;
    }

    return NULL; 
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command...>\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    if (pid == 0) {
/*        long res = ptrace(PTRACE_TRACEME, 0, 0, 0);
        if (res < 0)
            errquit("ptrace(PTRACE_TRACEME)");*/

        if (execvp(argv[1], argv + 1) < 0)
            errquit("Failed to start process");

        return 0;
    }

    fprintf(stderr, "Tracing: %d\n", pid);

    if (ptrace(PTRACE_SEIZE, pid, 0, PTRACE_O_EXITKILL | PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEEXEC) < 0)
        errquit("ptrace(PTRACE_ATTACH)");

    struct ptrace_syscall_info syscall;
    
    bool entry_valid = 0;
    ptrace_syscall_entry *entry = malloc(sizeof(ptrace_syscall_entry));

    int wstatus;
    while (1) {
        if (waitpid(pid, &wstatus, WCONTINUED) < 0)
            errquit("Failed to wait for process");

        if (WIFEXITED(wstatus) || WIFSIGNALED(wstatus)) {
            fprintf(stderr, "\nProcess exit: %d\n", WEXITSTATUS(wstatus));
            break;
        }

        if (WIFSTOPPED(wstatus)) {
            int signal = WSTOPSIG(wstatus);
            if (signal == (SIGTRAP | 0x80)) {
                if (ptrace(PTRACE_GET_SYSCALL_INFO, pid, sizeof(syscall), &syscall) < 0)
                    errquit("ptrace(PTRACE_GET_SYSCALL_INFO)");
                
                struct trace_info trace = { pid, entry_valid ? entry : NULL, NULL };
            
                ptrace_syscall_entry *new_entry = dump_syscall(&trace, &syscall);
                if (new_entry) {
                    memcpy(entry, new_entry, sizeof(ptrace_syscall_entry));
                    entry_valid = 1;
                } else {
                    entry_valid = 0;
                }
            } else {
                fprintf(stderr, "Signal: %d (SIG%s)\n", signal, sigabbrev_np(signal));
            }
        }

        if (ptrace(PTRACE_SYSCALL, pid, 0, 0) < 0)
            errquit("ptrace(PTRACE_SYSCALL)");
    }

    free(entry);

    return 0;
}
