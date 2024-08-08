#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process_info.h"

void print_usage(const char* progname) {
    printf("Usage: %s <pid> <pc>\n", progname);
    printf("  pid: Process ID\n");
    printf("  pc: Program Counter (address)\n");
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    int pid = atoi(argv[1]);
    uint64_t pc = strtoull(argv[2], NULL, 16);

    struct system_info system_info = {0};
    struct process* proc = create_process(&system_info, pid);
    if (!proc) {
        fprintf(stderr, "Failed to create process for pid %d\n", pid);
        return 1;
    }

    if (parse_process_maps(proc)) {
        fprintf(stderr, "Failed to parse VMA maps for pid %d\n", pid);
        return 1;
    }

    struct vma vma_info = {0};
    if (get_process_vma_info(&system_info, pid, pc, &vma_info)) {
        fprintf(stderr, "Failed to get VMA info for address 0x%lx\n", pc);
        return 1;
    }

    print_vma_info(&vma_info);

    uint64_t relative_address = pc - vma_info.start + vma_info.offset;
    printf("Relative address in ELF: 0x%lx\n", relative_address);

    struct elf_info elf_info = {0};
    read_elf_file(vma_info.name, &elf_info);

    const char* symbol_name = get_symbol_name_by_address(&elf_info, relative_address);
    if (symbol_name) {
        printf("Function name: %s\n", symbol_name);
    } else {
        printf("Function name not found\n");
    }

    free(proc->vmas);
    free(system_info.procs);

    return 0;
}
