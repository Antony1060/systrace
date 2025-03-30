## Build
```sh
python3 gen_syscall_defines.py
cc strace.c -o strace
```

## Run
```sh
./strace <command...>
```
