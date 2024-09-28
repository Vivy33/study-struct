#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "rbtree.h"

#include "process_info.h"
#include "elf.h"

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
    struct process_node* new_node = (struct process_node*)malloc(sizeof(struct process_node));
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
        if (node->elf_data.file_hash == file_hash && strcmp(node->elf_data.filename, filename) == 0) {
            return &node->elf_data;
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
        fprintf(stderr, "无法为文件名分配内存\n");
        return NULL;
    }

    new_elf.file_hash = file_hash;
    new_elf.elf_id = strdup(filename); // 在真实场景中，应使用 file_hash

    // 解析 ELF 符号表
    if (get_elf_func_symbols(filename, &new_elf)) {
        fprintf(stderr, "无法解析 ELF 符号表 %s\n", filename);
        return NULL;
    }

    // 创建新的 elf_node
    struct elf_node* new_node = (struct elf_node*)malloc(sizeof(struct elf_node));
    if (!new_node) {
        fprintf(stderr, "无法为 elf_node 分配内存\n");
        return NULL;
    }

    new_node->elf_data = new_elf;
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

    return &new_node->elf_data;
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
    return NULL; // 未找到
}

// 从 VMA 中获取 ELF 文件名
char* get_elfname_from_vma(struct vma* vma) {
    return vma->region_name;
}

// 获取相对地址
uint64_t get_relative_address(uint64_t real_addr, struct vma* vma) {
    return real_addr - vma->start + vma->offset;
}

// 查找符号名称的辅助函数
static struct symbol* find_symbol_from_elf(Elf* elf, uint64_t address) {
    Elf_Scn *scn = NULL;
    GElf_Shdr gshdr;

    // 优先查找 .symtab 符号表
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        if (gelf_getshdr(scn, &gshdr) != &gshdr) {
            fprintf(stderr, "Failed to get section header: %s\n", elf_errmsg(-1));
            continue;
        }

        // 查找符号表
        if (gshdr.sh_type == SHT_SYMTAB || gshdr.sh_type == SHT_DYNSYM) {
            Elf_Data *data = elf_getdata(scn, NULL);
            if (data == NULL) {
                fprintf(stderr, "Failed to get section data: %s\n", elf_errmsg(-1));
                continue;
            }

            size_t sym_count = gshdr.sh_size / gshdr.sh_entsize;
            for (size_t j = 0; j < sym_count; j++) {
                GElf_Sym symbol;
                if (gelf_getsym(data, j, &symbol) != &symbol) {
                    fprintf(stderr, "Failed to get symbol: %s\n", elf_errmsg(-1));
                    continue;
                }

                if (address >= symbol.st_value && address < symbol.st_value + symbol.st_size) {
                    struct symbol* result = (struct symbol*)malloc(sizeof(struct symbol));
                    if (!result) {
                        perror("Failed to allocate memory for symbol");
                        return NULL;
                    }
                    result->start_addr = symbol.st_value;
                    result->size = symbol.st_size;
                    result->name = strdup(elf_strptr(elf, gshdr.sh_link, symbol.st_name));
                    if (!result->name) {
                        perror("Failed to duplicate symbol name");
                        free(result);
                        return NULL;
                    }
                    return result;
                }
            }
        }
    }
    return NULL; // 符号未找到
}

/*
// 查找地址对应的符号名称
struct symbol* find_symbol_name_from_vma(struct process* proc, uint64_t address) {
    struct rb_node* node = proc->vma_tree.rb_node; // 从 VMA 树的根节点开始

    while (node) {
        struct vma* vma = container_of(node, struct vma, vma_node);

        if (address < vma->start) {
            node = node->rb_left; // 在左子树中查找
        } else if (address >= vma->end) {
            node = node->rb_right; // 在右子树中查找
        } else {
            // 找到包含该地址的 VMA
            Elf *elf = NULL;
            int fd = open(vma->file_hash, O_RDONLY, 0);
            if (fd < 0) {
                perror("Failed to open ELF file");
                return NULL;
            }
            if (elf_version(EV_CURRENT) == EV_NONE) {
                fprintf(stderr, "ELF library version mismatch\n");
                close(fd);
                return NULL;
            }
            elf = elf_begin(fd, ELF_C_READ, NULL);
            if (!elf) {
                fprintf(stderr, "Failed to open ELF file: %s\n", elf_errmsg(-1));
                close(fd);
                return NULL;
            }

            struct symbol* result = find_symbol_from_elf(elf, address);

            elf_end(elf);
            close(fd);

            return result; // 返回找到的符号
        }
    }
    return NULL; // VMA 中未找到地址
}
*/

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
        fprintf(stderr, "错误: ELF 库初始化失败: %s\n", elf_errmsg(-1));
        return 1;
    }
    return 0;
}

// 解析进程
struct process* get_process(struct system_info* sys_info, int pid) {
    struct process* proc = find_new_process(sys_info->procs, pid);
    if (!proc) {
        fprintf(stderr, "无法创建或查找 PID 为 %d 的进程\n", pid);
        return NULL;
    }
    return proc;
}

// 获取VMA信息
struct vma* get_vma_from_process(struct process* proc, uint64_t real_addr) {
    struct vma* vma_info = find_vma_from_process(proc, real_addr);
    if (!vma_info) {
        fprintf(stderr, "无法获取地址 0x%lx 的 VMA 信息\n", real_addr);
        return NULL;
    }
    return vma_info;
}

// 获取ELF文件
struct elf* get_elf(struct system_info* sys_info, uint64_t file_hash, const char* name) {
    struct elf* elf_file = find_elf(sys_info->elfs, file_hash, name);
    if (!elf_file) {
        elf_file = create_elf(sys_info->elfs, file_hash, name);
        if (!elf_file) {
            fprintf(stderr, "无法查找或创建 ELF 文件: %s\n", name);
            return NULL;
        }
    } else {
        // 如果 ELF 已存在，增加引用计数
        elf_file->ref_count++;
    }
    return elf_file;
}

int compare_symbols(const void *a, const void *b) {
    struct symbol *sym_a = (struct symbol *)a;
    struct symbol *sym_b = (struct symbol *)b;

    if (sym_a->start_addr < sym_b->start_addr) return -1;
    if (sym_a->start_addr > sym_b->start_addr) return 1;
    return 0;
}

// rbtree 二分查找
struct symbol* rb_search_symbol(struct rb_root *root, uint64_t addr) {
    struct rb_node *node = root->rb_node; // 从红黑树根节点开始

    while (node) {
        struct symbol *sym = rb_entry(node, struct symbol, symbol_node); // 获取当前节点对应的符号

        if (addr < sym->start_addr) // 如果目标地址小于当前符号的起始地址
            node = node->rb_left; // 移动到左子树
        else if (addr >= sym->start_addr + sym->size) // 如果目标地址不在当前符号的范围内
            node = node->rb_right; // 移动到右子树
        else // 如果目标地址在当前符号的范围内
            return sym; // 找到目标符号，返回
    }
    return NULL; // 如果没有找到，返回NULL
}

// 查找符号名称
const char* find_symbol_name_from_elf(struct elf* elf, uint64_t relative_address) {
    if (!elf || !elf->syms || elf->syms->symbol_count == 0) {
        return NULL;
    }

    const struct symbol *sym = rb_search_symbol(&elf->syms->symbol_tree, relative_address);
    if (sym != NULL) {
        return sym->name;
    } else {
        return NULL;
    }
}

// 获取符号名称
const char* get_symbol_name(struct elf* elf, uint64_t relative_address) {

    return find_symbol_name_from_elf(elf, relative_address);
}

void free_process_list(struct process_hash_table *hash_table) {
    // 遍历哈希表的每一个槽位
    for (int i = 0; i < HASHTABLE_SIZE; i++) {
        struct process_node* node = hash_table->nodes[i];
        // 遍历每一个槽位中的链表，释放所有节点
        while (node) {
            struct process_node* temp = node;
            node = node->next;
            free(temp->procs.proc_name);   // 释放进程名称
            free(temp->procs.cmdline);     // 释放命令行
            free(temp->procs.exe_path);    // 释放可执行文件路径
            // 注意: 如果 vma_tree 中有动态分配的资源，也需要在此释放
            free(temp); // 释放进程节点
        }
    }
    free(hash_table); // 释放哈希表结构
}

// 释放系统资源
void cleanup_system(struct system_info* system_info) {
    // 释放 ELF 节点
    for (int i = 0; i < 1024; i++) {
        struct elf_node* node = system_info->elfs->nodes[i];
        while (node) {
            struct elf_node* temp = node;
            node = node->next;
            free(temp->elf_data.filename);
            free(temp);
        }
        
    }
    
    free_process_list(system_info->procs);
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

    // 获取进程信息
    struct process* proc_info = get_process(&system_info, pid);
    if (!proc_info) {
        cleanup_system(&system_info);
        return 1;
    }

    // 获取 VMA 信息
    struct vma* vma_info = get_vma_from_process(proc_info, real_addr);
    if (!vma_info) {
        cleanup_system(&system_info);
        return 1;
    }

    // 打印 VMA 信息
    print_vma_info(vma_info);

    // 计算相对地址
    uint64_t relative_address = get_relative_address(real_addr, vma_info);
    printf("Relative address in ELF: 0x%lx\n", relative_address);

    // 更新 ELF 引用计数（增加引用）
    struct elf* elf = upsert_elf(vma_info->region_name);
    
    // 获取符号名称
    const char* symbol_name = get_symbol_name(elf, relative_address);
    if (symbol_name) {
        printf("Function name: %s\n", symbol_name);
    } else {
        printf("No function name found\n");
    }
    
    // 清理系统资源
    cleanup_system(&system_info);

    // 清理符号表
    clear_symbol_gcache();

    return 0;
}