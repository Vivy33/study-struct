#ifndef PROCESS_INFO_H
#define PROCESS_INFO_H

#include <stdint.h>
#include <libelf.h>
#include <gelf.h>

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
    // TODO
    // 还需要从文件路径生成一个 hash
    // file /bin/ls
    // /bin/ls: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, BuildID[sha1]=897f49cafa98c11d63e619e7e40352f855249c13, for GNU/Linux 3.2.0, stripped
    // BuildID[sha1]=897f4 就是我们要的 hash
};

struct vma_nodes {
    // TODO
    // 需要一个 红黑树 做 map
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
    struct vma_nodes* vmas;
    // TODO
    // 红黑树
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

// 定义 ELF 符号结构
// TODO 
// struct symbol 改成map 跟vma同理，k为staraddr value为symbol
struct elf_symbols {
    int symbol_count;
    struct symbol* syms;
};

struct elf {
    char* filename;
    char* elf_id;
    struct elf_symbols* elf_symbols;
};

struct elf_node {
    struct elf elf;
    struct elf_node* next;
};

struct elf_hash_table {
    struct elf_node* nodes[HASHTABLE_SIZE];
};

struct system_info {
    // TODO
    // 所有的进程 ->
    // 所有的 ELF 文件
    // 进程 通过 vma 查到 对应的 elf 文件
    // 引入一个文件 hash id 作为 elf 的 id
    // 需要一个 map，k 为 elf_id，v 为 struct elf
    // 不同的进程可能会共享一个 elf
    // 先从 vma 中查找 elf 是否解析过 符号表
    // 所以 elf 里面还要包含 符号表
    struct process_hash_table* procs;
    struct elf_hash_table* elfs;
};

struct elf_info {
    Elf64_Ehdr header;
    Elf64_Phdr *phdrs;
    Elf64_Shdr *shdrs;
    char *shstrtab;
};

unsigned int parse_process_maps(struct process* proc);
struct process* read_process_info(struct system_info* system_info, int pid);
int get_process_vma_info(struct system_info* system_info, int pid, uint64_t addr, struct vma* out_vma);
void print_vma_info(const struct vma* vma_info);

void print_elf_header(const Elf64_Ehdr *header);
void print_program_headers(const Elf64_Phdr *phdrs, uint16_t phnum);
void print_section_headers(const Elf64_Shdr *shdrs, uint16_t shnum);
void print_symbol_table(int fd, Elf64_Shdr *shdrs, uint16_t shnum, char *shstrtab);
void find_symbol_by_address(int fd, Elf64_Shdr *shdrs, uint16_t shnum, char *shstrtab, uint64_t address);
void read_elf_file(const char *filename, struct elf_info *elf_info);
const char* get_symbol_name_by_address(struct elf_info *elf_info, uint64_t address);

#endif
