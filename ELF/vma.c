#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "process_info.h"

#define INITIAL_VMA_CAPACITY 10

// 解析进程的内存映射信息
unsigned int parse_process_maps(struct process* proc) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", proc->pid);
    
    FILE* file = fopen(path, "r");
    if (!file) {
        perror("Failed to open maps file");
        return 1;
    }

    char line[256];
    unsigned int flags = 0;
    
    // 初始分配内存空间
    int capacity = INITIAL_VMA_CAPACITY;
    proc->vmas = malloc(capacity * sizeof(struct vma));
    if (!proc->vmas) {
        perror("Failed to allocate memory for VMAs");
        fclose(file);
        return 1;
    }

    while (fgets(line, sizeof(line), file)) {
        if (proc->num_vmas >= capacity) {
            // 如果空间不够，增加容量
            capacity *= 2;
            struct vma* new_vmas = malloc(capacity * sizeof(struct vma));
            if (!new_vmas) {
                perror("Failed to allocate memory for VMAs");
                free(proc->vmas);
                fclose(file);
                return 1;
            }
            memcpy(new_vmas, proc->vmas, proc->num_vmas * sizeof(struct vma));
            free(proc->vmas);
            proc->vmas = new_vmas;
        }

        struct vma vma;
        char perm[5] = {0};
        char name[256] = {0};

        sscanf(line, "%lx-%lx %4s %lx %*s %*d %255s",
               &vma.start, &vma.end, perm, &vma.offset, name);

        if (strchr(perm, 'r')) {
            flags |= READ;
        }
        if (strchr(perm, 'w')) {
            flags |= WRITE;
        }
        if (strchr(perm, 'x')) {
            flags |= EXECUTE;
        }

        vma.flags = flags;
        vma.name = strdup(name);
        vma.valid = 1;

        proc->vmas[proc->num_vmas++] = vma;
    }

    fclose(file);
    return 0;
}

// 获取进程的VMA信息
int get_process_vma_info(struct system_info* system_info, int pid, uint64_t addr, struct vma* out_vma) {
    struct process* proc = NULL;
    for (int i = 0; i < system_info->num_procs; i++) {
        if (system_info->procs[i].pid == pid) {
            proc = &system_info->procs[i];
            break;
        }
    }

    if (!proc) return 1;

    for (int i = 0; i < proc->num_vmas; i++) {
        if (addr >= proc->vmas[i].start && addr < proc->vmas[i].end) {
            *out_vma = proc->vmas[i];
            return 0;
        }
    }

    return 1;
}

// 打印VMA信息
void print_vma_info(const struct vma* vma_info) {
    printf("VMA Start: 0x%lx, End: 0x%lx, Offset: 0x%lx, Flags: %x, Name: %s\n",
           vma_info->start, vma_info->end, vma_info->offset, vma_info->flags, vma_info->name);
}
