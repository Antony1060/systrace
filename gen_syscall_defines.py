f = open("syscall_meta_x86_64.h", "w")

f.write("""#ifndef __SYSCALL_META
#define __SYSCALL_META

struct syscall_meta syscall_meta_table[] = {
""")

s = open("syscall_64.tbl", "r")

writers = open("syscall_util.h", "r")
writers_content = writers.read()
writers.close()

syscalls = {}

DEFINED_WRITERS = []

for line in s.readlines():
    line = line.strip()
    if line == "" or line.startswith("#"):
        continue

    num, abi, name = line.split("\t")[0:3]

    if abi not in ["common", "64"]:
        continue

    syscalls[int(num)] = name

s.close()

i = 0
si = 0
while si < len(syscalls):
    if i not in syscalls:
        f.write("    {0},\n")
        i += 1
        continue

    name = syscalls[i]
    if f"void fprint_syscall_{name}" in writers_content:
        f.write(f"    {{ \"{name}\", &fprint_syscall_{name} }},\n")
    else:
        f.write(f"    {{ \"{name}\", &fprint_syscall_default }},\n")
    i += 1
    si += 1


f.write("};\n");
f.write("\n#endif // __SYSCALL_META")

f.close()
