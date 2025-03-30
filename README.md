## systrace
A very basic implementation of the `strace` utility, not meant to be actaully used.

## Build
```sh
python3 gen_syscall_defines.py
cc strace.c -o strace
```

## Run
```sh
./strace <command...>
```
