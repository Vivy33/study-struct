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
        // TODO 
        // 逻辑realloc 弃用
        // 假如文件有1000行，申请释放过多 
        // 红黑树
        // 为什么这里不能用哈希，不支持范围查找

        // 比如要查找 7f6953063001
        // 先获取 第一列 start addr，二分比较第一列，找到 target 属于某两行之间 ，然后取较小的一行，看 target 是否属于之间
        // 如果不属于，则该 地址 在进程地址空间不合法，属于则找到 （map） k为start_addr , value为vma
        // 7f6953063000-7f6953064000 r--p 00000000 08:20 6506                       /usr/lib/locale/C.utf8/LC_MESSAGES/SYS_LC_MESSAGES
        // 7f6953064000-7f6953065000 r--p 00000000 08:20 6514                       /usr/lib/locale/C.utf8/LC_PAPER
        /*
            7f6952e2f000-7f6952e30000 r--p 00000000 08:20 6508                       /usr/lib/locale/C.utf8/LC_MONETARY
            7f6952e30000-7f6952e37000 r--s 00000000 08:20 24553                      /usr/lib/x86_64-linux-gnu/gconv/gconv-modules.cache
            7f6952e37000-7f6952e3a000 rw-p 00000000 00:00 0 
            7f6952e3a000-7f6952e62000 r--p 00000000 08:20 24212                      /usr/lib/x86_64-linux-gnu/libc.so.6
            7f6952e62000-7f6952ff7000 r-xp 00028000 08:20 24212                      /usr/lib/x86_64-linux-gnu/libc.so.6
            7f6952ff7000-7f695304f000 r--p 001bd000 08:20 24212                      /usr/lib/x86_64-linux-gnu/libc.so.6
            7f695304f000-7f6953050000 ---p 00215000 08:20 24212                      /usr/lib/x86_64-linux-gnu/libc.so.6
            7f6953050000-7f6953054000 r--p 00215000 08:20 24212                      /usr/lib/x86_64-linux-gnu/libc.so.6
            7f6953054000-7f6953056000 rw-p 00219000 08:20 24212                      /usr/lib/x86_64-linux-gnu/libc.so.6
            7f6953056000-7f6953063000 rw-p 00000000 00:00 0 
        7f6953063000-7f6953064000 r--p 00000000 08:20 6506                       /usr/lib/locale/C.utf8/LC_MESSAGES/SYS_LC_MESSAGES
        7f6953064000-7f6953065000 r--p 00000000 08:20 6514                       /usr/lib/locale/C.utf8/LC_PAPER
            7f6953065000-7f6953066000 r--p 00000000 08:20 6510                       /usr/lib/locale/C.utf8/LC_NAME
            7f6953066000-7f6953067000 r--p 00000000 08:20 6496                       /usr/lib/locale/C.utf8/LC_ADDRESS
            7f6953067000-7f6953068000 r--p 00000000 08:20 6516                       /usr/lib/locale/C.utf8/LC_TELEPHONE
            7f6953068000-7f6953069000 r--p 00000000 08:20 6504                       /usr/lib/locale/C.utf8/LC_MEASUREMENT
            7f6953069000-7f695306b000 rw-p 00000000 00:00 0 
        */

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
