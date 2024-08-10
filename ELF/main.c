#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process_info.h"

// 查找进程，如果未找到返回 NULL
struct process* find_process(struct system_info* sys_info, int pid) {
    for (int i = 0; i < sys_info->num_procs; i++) {
        if (sys_info->procs[i].pid == pid) {
            return &sys_info->procs[i];
        }
    }
    return NULL;
}

// 查找进程，如果未找到则创建新进程
struct process* find_new_process(struct system_info* sys_info, int pid) {
    struct process* proc = find_process(sys_info, pid);
    if (proc) {
        return proc;
    }

    // 创建新的 process 结构
    struct process new_proc = {0};
    new_proc.pid = pid;

    // 解析 VMA 信息
    if (parse_process_maps(&new_proc)) {
        fprintf(stderr, "Failed to parse VMA maps for pid %d\n", pid);
        return NULL;
    }

    // 将新进程添加到系统信息中
    sys_info->procs = realloc(sys_info->procs, (sys_info->num_procs + 1) * sizeof(struct process));
    if (!sys_info->procs) {
        perror("Failed to allocate memory for new process");
        free(new_proc.vmas);
        return NULL;
    }

    sys_info->procs[sys_info->num_procs++] = new_proc;
    return &sys_info->procs[sys_info->num_procs - 1];
}

// 从进程中查找 VMA 信息
struct vma* find_vma_from_process(struct process* proc, uint64_t real_addr) {
    for (int i = 0; i < proc->num_vmas; i++) {
        if (real_addr >= proc->vmas[i].start && real_addr < proc->vmas[i].end) {
            return &proc->vmas[i];
        }
    }
    return NULL;
}

// 从 VMA 中获取 ELF 文件名
char* get_elfname_from_vma(struct vma* vma) {
    return vma->name;
}

// 获取 ELF 文件的符号信息
struct elf_symbol* get_elf_func_symbols(const char* filename) {
    struct elf_symbol* elf_sym = malloc(sizeof(struct elf_symbol));
    if (!elf_sym) {
        return NULL;
    }
    // 假设已经实现了读取 ELF 符号表的逻辑
    // ...
    return elf_sym;
}

// 获取相对地址
uint64_t get_relative_address(uint64_t real_addr, struct vma* vma) {
    return real_addr - vma->start + vma->offset;
}

// 根据相对地址查找符号名称
char* find_symbol_name_from_elf(struct elf_symbol* syms, const uint64_t relative_address) {
    for (int i = 0; i < syms->symbol_count; i++) {
        if (relative_address >= syms->syms[i].start && relative_address < syms->syms[i].end) {
            return syms->syms[i].name;
        }
    }
    return NULL;
}

// 打印使用说明
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

    // 查找或创建进程
    struct process* proc = find_new_process(&system_info, pid);
    if (!proc) {
        fprintf(stderr, "Failed to create or find process for pid %d\n", pid);
        return 1;
    }

    // 查找 VMA
    struct vma* vma_info = find_vma_from_process(proc, pc);
    if (!vma_info) {
        fprintf(stderr, "Failed to get VMA info for address 0x%lx\n", pc);
        return 1;
    }

    print_vma_info(vma_info);

    // 计算 ELF 中的相对地址
    uint64_t relative_address = get_relative_address(pc, vma_info);
    printf("Relative address in ELF: 0x%lx\n", relative_address);

    // 获取 ELF 符号
    struct elf_symbol* elf_sym = get_elf_func_symbols(vma_info->name);
    if (!elf_sym) {
        fprintf(stderr, "Failed to get ELF symbols from %s\n", vma_info->name);
        return 1;
    }

    // 查找符号名称
    const char* symbol_name = find_symbol_name_from_elf(elf_sym, relative_address);
    if (symbol_name) {
        printf("Function name: %s\n", symbol_name);
    } else {
        printf("Function name not found\n");
    }

    // 释放内存
    free(elf_sym->syms);
    free(elf_sym);
    free(proc->vmas);
    free(system_info.procs);

    return 0;
}
