#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process_info.h"

// 哈希函数
unsigned int hash(int pid) {
    return pid % HASHTABLE_SIZE;
}

// 查找进程，如果未找到返回 NULL
struct process* find_process(struct hash_table* table, int pid) {
    unsigned int index = hash(pid);
    struct process_node* node = table->nodes[index];
    //二分
    while (node) {
        int low = 0, high = node->count - 1;
        while (low <= high) {
            int mid = (low + high) / 2;
            if (node->procs[mid].pid == pid) {
                return &node->procs[mid];
            } else if (node->procs[mid].pid < pid) {
                low = mid + 1;
            } else {
                high = mid - 1;
            }
        }
        node = node->next;
    }
    return NULL;
}

// 查找进程，如果未找到则创建新进程
struct process* find_new_process(struct hash_table* table, int pid) {
    struct process* proc = find_process(table, pid);
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

    // 创建新的 process_node
    struct process_node* new_node = malloc(sizeof(struct process_node));
    new_node->procs[0] = new_proc;
    new_node->count = 1;
    new_node->next = NULL;

    unsigned int index = hash(pid);
    struct process_node* current_node = table->nodes[index];

    if (!current_node) {
        table->nodes[index] = new_node;
    } else {
        while (current_node->next) {
            current_node = current_node->next;
        }
        current_node->next = new_node;
    }

    return &new_node->procs[0];
}


// 从进程中查找 VMA 信息
struct vma* find_vma_from_process(struct process* proc, uint64_t real_addr) {
    struct vma_node* node = proc->vma_list;
    while (node) {
        // 使用二分查找
        int low = 0, high = node->count - 1;
        while (low <= high) {
            int mid = (low + high) / 2;
            if (real_addr >= node->vmas[mid].start && real_addr < node->vmas[mid].end) {
                return &node->vmas[mid];
            } else if (real_addr < node->vmas[mid].start) {
                high = mid - 1;
            } else {
                low = mid + 1;
            }
        }
        node = node->next;
    }
    return NULL;
}

// 从 VMA 中获取 ELF 文件名
char* get_elfname_from_vma(struct vma* vma) {
    return vma->name;
}

// 获取 ELF 文件的符号信息
extern struct elf_symbol* get_elf_func_symbols(const char* filename);

// 获取相对地址
uint64_t get_relative_address(uint64_t real_addr, struct vma* vma) {
    return real_addr - vma->start + vma->offset;
}

// 根据相对地址查找符号名称
char* find_symbol_name_from_elf(struct elf_symbol* syms, const uint64_t relative_address) {
    int left = 0;
    int right = syms->symbol_count - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (relative_address >= syms->syms[mid].start && relative_address < syms->syms[mid].end) {
            return syms->syms[mid].name;
        } else if (relative_address < syms->syms[mid].start) {
            right = mid - 1;
        } else {
            left = mid + 1;
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
    uint64_t real_addr = strtoull(argv[2], NULL, 16);

    struct system_info system_info = {0};

    // 查找或创建进程
    struct process* proc = find_new_process(&system_info, pid);
    if (!proc) {
        fprintf(stderr, "无法创建或查找 PID 为 %d 的进程\n", pid);
        return 1;
    }

    // 查找 VMA
    struct vma* vma_info = find_vma_from_process(proc, real_addr);
    if (!vma_info) {
        fprintf(stderr, "无法获取地址 0x%lx 的 VMA 信息\n", real_addr);
        return 1;
    }

    print_vma_info(vma_info);

    // 计算 ELF 中的相对地址
    uint64_t relative_address = get_relative_address(real_addr, vma_info);
    printf("ELF 中的相对地址: 0x%lx\n", relative_address);

    // 获取 ELF 符号
    struct elf_symbol* elf_sym = get_elf_func_symbols(vma_info->name);
    if (!elf_sym) {
        fprintf(stderr, "无法从 %s 获取 ELF 符号\n", vma_info->name);
        return 1;
    }

    // 查找符号名称
    const char* symbol_name = find_symbol_name_from_elf(elf_sym, relative_address);
    if (symbol_name) {
        printf("函数名称: %s\n", symbol_name);
    } else {
        printf("未找到函数名称\n");
    }

    // 释放内存
    free(elf_sym->syms);
    free(elf_sym);
    free_process_list(system_info.head);

    return 0;
}
