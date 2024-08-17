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

// 查找 ELF 文件，如果未找到返回 NULL
struct elf* find_elf(struct elf_hash_table* elf_table, uint64_t file_hash, const char* filename) {
    unsigned int index = hash(file_hash);
    struct elf_node* node = elf_table->nodes[index];

    // 遍历链表查找匹配的 ELF 文件
    while (node) {
        if (node->elf.file_hash == file_hash && strcmp(node->elf.filename, filename) == 0) {
            return &node->elf;
        }
        node = node->next;
    }

    return NULL; // 未找到
}

// 创建新的 ELF 结构并插入到哈希表中
struct elf* create_elf(struct elf_hash_table* elf_table, uint64_t file_hash, const char* filename) {
    struct elf new_elf = {0};
    new_elf.filename = strdup(filename);
    if (!new_elf.filename) {
        fprintf(stderr, "Failed to allocate memory for filename\n");
        return NULL;
    }

    new_elf.file_hash = file_hash;
    new_elf.elf_id = file_hash; // 真实场景中应使用 file_hash

    // 解析 ELF 符号表
    if (parse_elf_symbols(&new_elf)) {
        fprintf(stderr, "Failed to parse ELF symbols for %s\n", filename);
        free(new_elf.filename);
        return NULL;
    }

    // 创建新的 elf_node
    struct elf_node* new_node = malloc(sizeof(struct elf_node));
    if (!new_node) {
        fprintf(stderr, "Failed to allocate memory for elf_node\n");
        free(new_elf.filename);
        return NULL;
    }

    new_node->elf = new_elf;
    new_node->next = NULL;

    // 插入到哈希表中
    unsigned int index = hash(file_hash);
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

// 初始化系统信息
int initialize_system(struct system_info* system_info) {
    memset(system_info, 0, sizeof(struct system_info));

    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "Error: ELF library initialization failed: %s\n", elf_errmsg(-1));
        return 1;
    }

    return 0;
}

// 解析进程并获取VMA信息
struct vma* get_process_and_vma(struct system_info* system_info, int pid, uint64_t real_addr) {
    struct process* proc = find_new_process(system_info->procs, pid);
    if (!proc) {
        fprintf(stderr, "无法创建或查找 PID 为 %d 的进程\n", pid);
        return NULL;
    }

    struct vma* vma_info = find_vma_from_process(proc, real_addr);
    if (!vma_info) {
        fprintf(stderr, "无法获取地址 0x%lx 的 VMA 信息\n", real_addr);
        return NULL;
    }

    return vma_info;
}

// 获取ELF文件及符号信息
char* get_elf_and_symbol(struct system_info* system_info, struct vma* vma_info, uint64_t relative_address) {
    struct elf* elf_file = find_elf(system_info->elfs, vma_info->file_hash, vma_info->name);
    if (!elf_file) {
        fprintf(stderr, "无法查找或创建 ELF 文件: %s\n", vma_info->name);
        return NULL;
    }

    struct elf_symbols* elf_syms = (struct elf_symbols*)&elf_file->symbol_tree;
    return find_symbol_name_from_elf(elf_syms, relative_address);
}

// 释放系统资源
void cleanup_system(struct system_info* system_info) {
    free_process_list(system_info->procs);
    free_elf_list(system_info->elfs);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    int pid = atoi(argv[1]);
    uint64_t real_addr = strtoull(argv[2], NULL, 16);

    struct system_info system_info = {0};

    // 初始化系统
    if (initialize_system(&system_info)) {
        return 1;
    }

    // 查找进程并获取VMA信息
    struct vma* vma_info = get_process_and_vma(&system_info, pid, real_addr);
    if (!vma_info) {
        cleanup_system(&system_info);
        return 1;
    }

    // 打印 VMA 信息
    print_vma_info(vma_info);

    // 查找 ELF 文件及符号名称
    uint64_t relative_address = get_relative_address(real_addr, vma_info);
    printf("Relative address in ELF: 0x%lx\n", relative_address);

    const char* symbol_name = get_elf_and_symbol(&system_info, vma_info, relative_address);
    if (symbol_name) {
        printf("Function name: %s\n", symbol_name);
    } else {
        printf("No function name found\n");
    }

    // 清理系统资源
    cleanup_system(&system_info);

    return 0;
}