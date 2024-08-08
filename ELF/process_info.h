#ifndef PROCESS_INFO_H
#define PROCESS_INFO_H

#include <stdint.h>
#include <libelf.h>
#include <gelf.h>

#define READ 0x1
#define WRITE 0x2
#define EXECUTE 0x4

struct vma {
    uint64_t start;
    uint64_t end;
    unsigned int flags;
    uint64_t offset;
    char* name;
    int valid;
};

struct process {
    int pid;
    char* name;
    char* cmdline;
    char* exe_path;
    uint64_t start_time;
    struct vma* vmas;
    int num_vmas;
    int valid;
};

struct system_info {
    struct process* procs;
    int num_procs;
};

struct elf_info {
    Elf64_Ehdr header;
    Elf64_Phdr *phdrs;
    Elf64_Shdr *shdrs;
    char *shstrtab;
};

unsigned int parse_process_maps(struct process* proc);
struct process* create_process(struct system_info* system_info, int pid);
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
