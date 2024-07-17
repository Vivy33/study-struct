#include "process_info.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

unsigned int parse_flags(const char* flags) {
    unsigned int result = 0;
    if (strlen(flags) != 4) return result;

    if (flags[0] == 'r') result |= READ;
    if (flags[1] == 'w') result |= WRITE;
    if (flags[2] == 'x') result |= EXECUTE;

    return result;
}

int parse_virtual_memory_area(const char* line, struct vma* out_vma) {
    char start_end[32], flags[5], offset[32], dev[32], inode[32], pathname[256];

    if (sscanf(line, "%s %s %s %s %s %255[^\n]", start_end, flags, offset, dev, inode, pathname) != 6) {
        return 0; // 解析失败
    }

    if (pathname[0] == ' ') {
        memmove(pathname, pathname + 1, strlen(pathname)); // 去除前导空格
    }

    char* dash_pos = strchr(start_end, '-');
    if (!dash_pos) {
        return 0; // 格式错误，缺少 '-'
    }

    out_vma->start = strtoull(start_end, &dash_pos, 16);
    out_vma->end = strtoull(dash_pos + 1, NULL, 16);
    out_vma->flags = parse_flags(flags);
    out_vma->offset = strtoull(offset, NULL, 16);
    out_vma->name = strdup(pathname);
    out_vma->valid = 1;

    return 1; // 解析成功
}

int load_process_vmas(int pid, struct vma** vmas, int* num_vmas) {
    char filename[256];
    FILE* maps_file;

    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    maps_file = fopen(filename, "r");
    if (!maps_file) {
        return 0; // 打开文件失败
    }

    *num_vmas = 0;
    while (!feof(maps_file)) {
        char line[512];
        if (!fgets(line, sizeof(line), maps_file)) {
            break;
        }
        struct vma vma_info;
        if (parse_virtual_memory_area(line, &vma_info) && vma_info.valid) {
            *vmas = (struct vma*) realloc(*vmas, (*num_vmas + 1) * sizeof(struct vma));
            (*vmas)[*num_vmas] = vma_info;
            (*num_vmas)++;
        }
    }

    fclose(maps_file);
    return 1; // 加载成功
}

char* read_file(const char* filepath) {
    FILE* file = fopen(filepath, "r");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* content = (char*) malloc(size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }

    fread(content, 1, size, file);
    content[size] = '\0';
    fclose(file);
    return content;
}

char* read_cmdline(int pid) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/proc/%d/cmdline", pid);
    char* content = read_file(filepath);
    if (!content) return NULL;

    // 替换 \0 为 空格
    for (char* p = content; *p; ++p) {
        if (*p == '\0') *p = ' ';
    }
    return content;
}

char* read_link(const char* filepath) {
    char link_target[PATH_MAX];
    ssize_t len = readlink(filepath, link_target, sizeof(link_target) - 1);
    if (len == -1) return NULL;
    link_target[len] = '\0';
    return strdup(link_target);
}

int read_process_info(struct process* proc) {
    char filepath[256];

    // 读取进程名
    snprintf(filepath, sizeof(filepath), "/proc/%d/comm", proc->pid);
    proc->name = read_file(filepath);
    if (!proc->name) return 0;

    // 读取命令行参数
    proc->cmdline = read_cmdline(proc->pid);
    if (!proc->cmdline) return 0;

    // 读取进程启动时间
    snprintf(filepath, sizeof(filepath), "/proc/%d/stat", proc->pid);
    char* stat_content = read_file(filepath);
    if (!stat_content) return 0;

    // stat 文件的第 22 个字段是启动时间
    char* token = strtok(stat_content, " ");
    for (int i = 1; i < 22 && token; ++i) {
        token = strtok(NULL, " ");
    }
    if (token) {
        proc->start_time = strtoull(token, NULL, 10);
    }
    free(stat_content);

    // 读取可执行文件路径
    snprintf(filepath, sizeof(filepath), "/proc/%d/exe", proc->pid);
    proc->exe_path = read_link(filepath);

    return 1;
}

struct process* create_process(struct system_info* system_info, int pid) {
    // 检查是否已经存在此进程
    struct process* existing_proc = find_process(system_info, pid);
    if (existing_proc) {
        return existing_proc;
    }

    // 创建新的进程结构体
    struct process new_proc;
    new_proc.pid = pid;
    new_proc.valid = read_process_info(&new_proc);

    if (!new_proc.valid) {
        return NULL; // 读取进程信息失败
    }

    // 加载进程的 VMA 信息
    new_proc.num_vmas = 0;
    new_proc.vmas = NULL;
    if (!load_process_vmas(pid, &new_proc.vmas, &new_proc.num_vmas)) {
        free(new_proc.name);
        free(new_proc.cmdline);
        free(new_proc.exe_path);
        return NULL; // 加载 VMA 信息失败
    }

    // 添加到系统信息中
    system_info->procs = (struct process*) realloc(system_info->procs, (system_info->num_procs + 1) * sizeof(struct process));
    if (!system_info->procs) {
        // 内存分配失败
        free(new_proc.name);
        free(new_proc.cmdline);
        free(new_proc.exe_path);
        for (int j = 0; j < new_proc.num_vmas; j++) {
            free(new_proc.vmas[j].name);
        }
        free(new_proc.vmas);
        return NULL;
    }

    system_info->procs[system_info->num_procs++] = new_proc;

    // 返回新创建的进程结构体的指针
    return &system_info->procs[system_info->num_procs - 1];
}

struct process* find_process(struct system_info* system_info, int pid) {
    for (int i = 0; i < system_info->num_procs; ++i) {
        if (system_info->procs[i].pid == pid) {
            return &system_info->procs[i];
        }
    }
    return NULL;
}

struct vma* find_vma(struct process* proc, uint64_t addr) {
    for (int i = 0; i < proc->num_vmas; ++i) {
        if (addr >= proc->vmas[i].start && addr < proc->vmas[i].end) {
            return &proc->vmas[i];
        }
    }
    return NULL;
}

void print_vma_info(const struct vma* vma_info) {
    printf("地址范围: 0x%lx - 0x%lx\n", vma_info->start, vma_info->end);
    printf("权限: %s%s%s\n",
           vma_info->flags & READ ? "读取 " : "",
           vma_info->flags & WRITE ? "写入 " : "",
           vma_info->flags & EXECUTE ? "执行 " : "");
    printf("偏移量: 0x%x\n", vma_info->offset);
    printf("路径: %s\n", vma_info->name);
}

int get_process_vma_info(struct system_info* system_info, int pid, uint64_t addr, struct vma* out_vma) {
    struct process* proc = find_process(system_info, pid);
    if (!proc) {
        return 0; // 没有找到指定 PID 的进程
    }

    struct vma* vma = find_vma(proc, addr);
    if (!vma) {
        return 0; // 没有找到指定地址的 VMA
    }

    *out_vma = *vma;
    return 1;
}