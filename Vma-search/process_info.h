#ifndef PROCESS_INFO_H
#define PROCESS_INFO_H

#include <stdint.h>

#define READ 4
#define WRITE 2
#define EXECUTE 1

struct vma {
    uint64_t start;
    uint64_t end;
    uint32_t offset;
    uint32_t flags; // rwx
    char* name;
    int valid; // 是否为有效的vma
};

struct process {
    int pid;
    char* name;
    char* cmdline;
    uint64_t start_time;
    char* exe_path;
    struct vma* vmas;
    int num_vmas;
    int valid; // 是否为有效的进程
};

struct system_info {
    struct process* procs; // pid为key, process为value的map
    int num_procs;
};

unsigned int parse_flags(const char* flags);

int parse_virtual_memory_area(const char* line, struct vma* out_vma);
int load_process_vmas(int pid, struct vma** vmas, int* num_vmas);

char* read_file(const char* filepath);
char* read_cmdline(int pid);
char* read_link(const char* filepath);

int read_process_info(struct process* proc);

struct process* create_process(struct system_info* system_info, int pid);
struct process* find_process(struct system_info* system_info, int pid);
struct vma* find_vma(struct process* proc, uint64_t addr);

void print_vma_info(const struct vma* vma_info);

int get_process_vma_info(struct system_info* system_info, int pid, uint64_t addr, struct vma* out_vma);

#endif // PROCESS_INFO_H
