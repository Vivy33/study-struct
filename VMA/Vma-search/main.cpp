#include "process_info.h"
#include <stdio.h>
#include <stdlib.h>

void print_process_info(const struct process* proc) {
    if (!proc->valid) {
        printf("进程信息无效\n");
        return;
    }

    printf("进程 PID: %d\n", proc->pid);
    printf("进程名: %s\n", proc->name);
    printf("命令行: %s\n", proc->cmdline);
    printf("启动时间: %lu\n", proc->start_time);
    printf("可执行文件: %s\n", proc->exe_path);

    printf("内存映射区域:\n");
    for (int i = 0; i < proc->num_vmas; i++) {
        print_vma_info(&proc->vmas[i]);
        printf("\n");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "用法: %s <pid> <address>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int pid = atoi(argv[1]);
    uint64_t address = strtoull(argv[2], NULL, 16);

    struct system_info system_info = {nullptr, 0};
    struct process* proc = create_process(&system_info, pid);

    if (!proc) {
        fprintf(stderr, "无法获取 PID %d 的进程信息\n", pid);
        return EXIT_FAILURE;
    }

    print_process_info(proc);

    struct vma vma_info;
    if (get_process_vma_info(&system_info, pid, address, &vma_info)) {
        printf("\n指定地址 0x%lx 的 VMA 信息:\n", address);
        print_vma_info(&vma_info);
    } else {
        printf("\n无法找到指定地址 0x%lx 的 VMA 信息\n", address);
    }

    // 释放动态分配的内存
    for (int i = 0; i < system_info.num_procs; i++) {
        free(system_info.procs[i].name);
        free(system_info.procs[i].cmdline);
        free(system_info.procs[i].exe_path);
        for (int j = 0; j < system_info.procs[i].num_vmas; j++) {
            free(system_info.procs[i].vmas[j].name);
        }
        free(system_info.procs[i].vmas);
    }
    free(system_info.procs);

    return EXIT_SUCCESS;
}
