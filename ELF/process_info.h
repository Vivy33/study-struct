#ifndef PROCESS_INFO_H
#define PROCESS_INFO_H

#include <stdint.h>
#include <libelf.h>
#include <gelf.h>

#include "rbtree.h"

#define READ 0x1
#define WRITE 0x2
#define EXECUTE 0x4

// 哈希的槽数 尽量是 质数 或 2^n - 1。为了减少冲突，希望哈希槽数多一点 
// 如果选 256 ，超过 256 个元素，一定出现哈希冲突
#define HASHTABLE_SIZE 8191

// 定义虚拟内存区域（VMA）的数据结构
struct vma {
    uint64_t start;             // 起始地址
    uint64_t end;               // 结束地址
    unsigned int flags;         // 标志位，标识权限或属性，如读写权限
    uint64_t offset;            // 偏移量，通常用于映射文件
    char* region_name;          // 内存区域的名称
    uint64_t file_hash;         // 由文件路径生成的哈希值，用于识别与文件关联的内存映射
    struct rb_node vma_node;    // 红黑树节点，用于在进程的红黑树中管理VMAs
};

// 定义进程结构体
struct process {
    int pid;                     // 进程ID
    char* proc_name;             // 进程的名称
    char* cmdline;               // 命令行
    char* exe_path;              // 可执行文件路径
    uint64_t start_time;         // 进程的启动时间
    int num_vmas;                // VMA数量
    struct rb_root vma_tree;     // 用于管理VMA的红黑树，以VMA起始地址作为K VMA作为V
};

// 描述哈希冲突的情况
// 所以其他场景下，都该使用 process，而不是 process_node
struct process_node {
    struct process procs;         // 进程信息
    struct process_node* next;   // 链表指针，用于处理哈希冲突
};

// 定义哈希表结构体
struct process_hash_table {
    struct process_node* nodes[HASHTABLE_SIZE];  // 哈希表的槽位，每个槽位存储一个进程链表
};

// 定义符号结构，用于表示程序中的符号（如函数、变量）
struct symbol {
    char* name;                  // 符号的名称
    uint64_t start_addr;            // 符号的地址（起始地址）
    uint64_t size;               // 符号的大小（即结束地址 - 起始地址）
    struct rb_node symbol_node;  // 红黑树节点，用于符号红黑树的管理
};

// ELF 符号红黑树管理结构体
struct elf_symbols {
    int symbol_count;            // 符号数量
    struct rb_root symbol_tree;  // 符号红黑树，以符号的首地址为K struct symbol作为V
};

// 定义 ELF 文件结构体，用于管理 ELF 文件及其符号
// 合并了 elf_info 和 elf 结构，避免冗余
struct elf {
    uint64_t file_hash;               // ELF 文件哈希
    char* filename;                   // 文件名
    char* elf_id;                     // ELF 文件唯一标识
    Elf64_Ehdr header;                // ELF 头部信息
    Elf64_Phdr *phdrs;                // 程序头表
    Elf64_Shdr *shdrs;                // 段头表
    char *shstrtab;                   // 段名字符串表
    int ref_count;                    // ELF 文件的引用计数
    struct rb_root_cached symbol_tree; // 带左最节点缓存的红黑树，用于管理 ELF 符号
};

// 描述 ELF 哈希冲突情况下的 ELF 节点
struct elf_node {
    struct elf elf_data;         // 包含 ELF 文件信息
    struct elf_node* next;       // 链表指针，用于处理哈希冲突
};

// 定义 ELF 哈希表结构体
struct elf_hash_table {
    struct elf_node* nodes[HASHTABLE_SIZE];  // 哈希表的槽位，每个槽位存储一个 ELF 节点链表
};

// 定义系统信息结构体，用于管理进程和 ELF 的哈希表
struct system_info {
    struct process_hash_table* procs;  // 进程哈希表
    struct elf_hash_table* elfs;       // ELF 哈希表
};

struct process* read_process_info(struct system_info* system_info, int pid);
int get_process_vma_info(struct system_info* system_info, int pid, uint64_t addr, struct vma* out_vma);
void print_vma_info(const struct vma* vma_info);

void print_elf_header(const Elf64_Ehdr *header);
void print_program_headers(const Elf64_Phdr *phdrs, uint16_t phnum);
void print_section_headers(const Elf64_Shdr *shdrs, uint16_t shnum);
void find_symbol_by_address(int fd, Elf64_Shdr *shdrs, uint16_t shnum, char *shstrtab, uint64_t address);

#endif
