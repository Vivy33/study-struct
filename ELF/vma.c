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
    proc->vma_tree = RB_ROOT; // 初始化红黑树

    while (fgets(line, sizeof(line), file)) {
        struct vma* vma = malloc(sizeof(struct vma));
        if (!vma) {
            perror("Failed to allocate memory for VMA");
            fclose(file);
            return 1;
        }
        char perm[5] = {0};
        char name[256] = {0};

        sscanf(line, "%lx-%lx %4s %lx %*s %*d %255s",
               &vma->start, &vma->end, perm, &vma->offset, name);

        if (strchr(perm, 'r')) {
            flags |= READ;
        }
        if (strchr(perm, 'w')) {
            flags |= WRITE;
        }
        if (strchr(perm, 'x')) {
            flags |= EXECUTE;
        }

        vma->flags = flags;
        vma->name = strdup(name);

        // 将VMA插入红黑树中
        struct rb_node **new = &(proc->vma_tree.rb_node), *parent = NULL;
        while (*new) {
            struct vma *this = rb_entry(*new, struct vma, vma_node);

            parent = *new;
            if (vma->start < this->start)
                new = &((*new)->rb_left);
            else
                new = &((*new)->rb_right);
        }

        // 添加红黑树节点
        rb_link_node(&vma->vma_node, parent, new);
        rb_insert_color(&vma->vma_node, &proc->vma_tree);
        proc->num_vmas++;
    }

    fclose(file);
    return 0;
}

// 打印VMA信息
void print_vma_info(const struct vma* vma_info) {
    printf("VMA Start: 0x%lx, End: 0x%lx, Offset: 0x%lx, Flags: %x, Name: %s\n",
           vma_info->start, vma_info->end, vma_info->offset, vma_info->flags, vma_info->name);
}
