#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "process_info.h"

unsigned int parse_process_maps(struct process* proc) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", proc->pid);
    FILE* file = fopen(path, "r");
    if (!file) {
        perror("fopen");
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        struct vma vma;
        char perm[5] = {0};
        char name[256] = {0};
        sscanf(line, "%lx-%lx %4s %lx %*s %*d %255s",
               &vma.start, &vma.end, perm, &vma.offset, name);
        vma.flags = (strchr(perm, 'r') ? READ : 0) |
                    (strchr(perm, 'w') ? WRITE : 0) |
                    (strchr(perm, 'x') ? EXECUTE : 0);
        vma.name = strdup(name);
        vma.valid = 1;

        proc->vmas = realloc(proc->vmas, (proc->num_vmas + 1) * sizeof(struct vma));
        proc->vmas[proc->num_vmas++] = vma;
    }
    fclose(file);
    return 0;
}

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

void print_vma_info(const struct vma* vma_info) {
    printf("VMA Start: 0x%lx, End: 0x%lx, Offset: 0x%lx, Flags: %x, Name: %s\n",
           vma_info->start, vma_info->end, vma_info->offset, vma_info->flags, vma_info->name);
}
