#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process_info.h"
#include "rbtree.h"

// 哈希函数
unsigned int hash(int pid) {
    return pid % HASHTABLE_SIZE;
}

// process不需要范围查找 用hash
// 无序数据不可以用二分查找, 哈希是无序数据
// 查找进程，如果未找到返回 NULL
struct process* find_process(struct process_hash_table* table, int pid) {
    unsigned int index = hash(pid);
    struct process_node* node = table->nodes[index];
    
    while (node) {
        if (node->procs.pid == pid) {
            return &node->procs;
        }
        node = node->next;
    }
    return NULL;
}


// 查找进程，如果未找到则创建新进程
struct process* find_new_process(struct process_hash_table* process_table, int pid) {
    struct process* proc = find_process(process_table, pid);
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
    new_node->procs = new_proc;
    new_node->next = NULL;

    unsigned int index = hash(pid);
    struct process_node* current_node = process_table->nodes[index];

    if (!current_node) {
        process_table->nodes[index] = new_node;
    } else {
        while (current_node->next) {
            current_node = current_node->next;
        }
        current_node->next = new_node;
    }

    return &new_node->procs;
}

// 查找 ELF 文件，如果未找到则创建新的 ELF 结构
struct elf* find_elf(struct elf_hash_table* elf_table, uint64_t file_hash, const char* filename) {
    unsigned int index = hash(file_hash);
    struct elf_node* node = elf_table->nodes[index];

    while (node) {
        if (strcmp(node->elf.elf_id, filename) == 0) {
            return &node->elf;
        }
        node = node->next;
    }

    // 如果未找到，创建新的 ELF 结构
    struct elf new_elf = {0};
    new_elf.filename = strdup(filename);
    new_elf.elf_id = strdup(filename); // 暂时用文件名代替，真实场景中应使用 file_hash
    // 解析 ELF 符号表
    if (parse_elf_symbols(&new_elf)) {
        fprintf(stderr, "Failed to parse ELF symbols for %s\n", filename);
        return NULL;
    }

    // 创建新的 elf_node
    struct elf_node* new_node = malloc(sizeof(struct elf_node));
    new_node->elf = new_elf;
    new_node->next = NULL;

    if (!elf_table->nodes[index]) {
        elf_table->nodes[index] = new_node;
    } else {
        struct elf_node* current_node = elf_table->nodes[index];
        while (current_node->next) {
            current_node = current_node->next;
        }
        current_node->next = new_node;
    }

    return &new_node->elf;
}

// 从进程中查找 VMA 信息
struct vma* find_vma_from_process(struct process* proc, unsigned long real_addr) {
    struct rb_node* node = proc->vma_tree.rb_node;

    while (node) {
        struct vma* vma_info = rb_entry(node, struct vma, vma_node);

        if (real_addr < vma_info->start)
            node = node->rb_left;
        else if (real_addr >= vma_info->end)
            node = node->rb_right;
        else
            return vma_info;
    }
    return NULL; // 未找到合法的VMA
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
char* find_symbol_name_from_elf(struct elf_symbols* syms, const uint64_t relative_address) {
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
    struct process* proc = find_new_process(system_info.procs, pid);
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

    // 查找或创建 ELF 文件
    struct elf* elf_file = find_elf(system_info.elfs, vma_info->file_hash, vma_info->name);
    if (!elf_file) {
        fprintf(stderr, "无法查找或创建 ELF 文件: %s\n", vma_info->name);
        return 1;
    }

    // 查找符号名称
    struct elf_symbols* elf_syms = (struct elf_symbols*)&elf_file->symbol_tree;
    const char* symbol_name = find_symbol_name_from_elf(elf_syms, relative_address);
    if (symbol_name) {
        printf("函数名称: %s\n", symbol_name);
    } else {
        printf("未找到函数名称\n");
    }

        if (argc != 2) {
        fprintf(stderr, "Usage: %s <elf-file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 初始化 libelf
    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "ELF library initialization failed: %s\n", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }
    
    struct elf_info elf_info;  // 定义 elf_info 结构体实例
    read_elf_file(argv[1], &elf_info);  // 读取 ELF 文件并初始化 elf_info

    // 获取 ELF 符号信息
    struct elf_symbols* symbols = get_elf_func_symbols(argv[1]);
    if (!symbols) {
        fprintf(stderr, "Failed to get symbols from ELF file.\n");
        exit(EXIT_FAILURE);
    }

    // 打印符号信息
    print_elf_symbols(symbols);

    // 释放符号红黑树内存
    struct rb_node *node;
    for (node = rb_first(&symbols->symbol_tree); node; ) {
        struct elf_symbol *symbol = rb_entry(node, struct elf_symbol, symbol_node);
        node = rb_next(node);
        free_elf_symbols(symbol);
    }

    // 释放内存
    free_process_list(system_info.procs);
    free_elf_list(system_info.elfs);
    free(symbols);
    free_elf_info(&elf_info); 

    return 0;
}
