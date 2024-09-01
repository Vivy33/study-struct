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

struct vma {
    uint64_t start;
    uint64_t end;
    unsigned int flags;
    uint64_t offset;
    char* name;
    uint64_t file_hash; // 由文件路径生成的哈希值，例如BuildID[sha1]
    struct rb_node vma_node; // 红黑树节点，用于VMA红黑树
};

struct vma_nodes {
    struct vma vmas[16];
    int count;  // 当前节点中存储的 VMA 数目
    struct vma_nodes* next;
};

struct process {
    int pid;
    char* name;
    char* cmdline;
    char* exe_path;
    uint64_t start_time;
    int num_vmas;
    struct rb_root vma_tree; // 红黑树的根，用于管理VMAs
};

// 描述哈希冲突的情况
// 所以其他场景下，都该使用 process，而不是 process_node
struct process_node {
    struct process procs;
    struct process_node* next;
};

// 定义哈希表结构体
struct process_hash_table {
    struct process_node* nodes[HASHTABLE_SIZE]; // 哈希槽
};

// 定义符号结构
struct symbol {
    uint64_t start;
    uint64_t end;
    char* name;
};

struct elf_symbol {
    char* name;
    uint64_t address;
    uint64_t size;
    struct rb_node symbol_node; // 红黑树节点, 用于符号红黑树
};

// 符号哈希表结构体
struct elf_symbols {
    int symbol_count;
    struct symbol* syms;
    struct rb_root symbol_tree; // 用于管理符号的红黑树
};

// 定义 ELF 符号结构
struct elf {
    uint64_t file_hash;
    char* filename;
    char* elf_id;
    struct rb_root_cached symbol_tree; // 红黑树的根，用于管理符号，带左最节点缓存
};

struct elf_node {
    struct elf elf;
    struct elf_node* next;
};

struct elf_hash_table {
    struct elf_node* nodes[HASHTABLE_SIZE];
};

struct system_info {
    struct process_hash_table* procs;
    struct elf_hash_table* elfs;
};

struct elf_info {
    Elf64_Ehdr header;
    Elf64_Phdr *phdrs;
    Elf64_Shdr *shdrs;
    char *shstrtab;
    char filename[255];
    int ref_count;
};

unsigned int parse_process_maps(struct process* proc);
struct process* read_process_info(struct system_info* system_info, int pid);
int get_process_vma_info(struct system_info* system_info, int pid, uint64_t addr, struct vma* out_vma);
void print_vma_info(const struct vma* vma_info);

void print_elf_header(const Elf64_Ehdr *header);
void print_program_headers(const Elf64_Phdr *phdrs, uint16_t phnum);
void print_section_headers(const Elf64_Shdr *shdrs, uint16_t shnum);
void find_symbol_by_address(int fd, Elf64_Shdr *shdrs, uint16_t shnum, char *shstrtab, uint64_t address);
void read_elf_file(const char *filename, struct elf_info *elf_info);

#endif
