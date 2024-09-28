#include <libelf.h>
#include <gelf.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> // For access() function
#include <string.h>
#include <dirent.h>  // For DIR, struct dirent
#include <sys/stat.h> // For struct stat
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>

#include "process_info.h"
#include "rbtree.h"

#define ELF_MAGIC 0x464c457f
#define SECTION_NAME ".text"

struct elf_cache g_cache;

// 哈希函数
unsigned int hash(const char *str) {
    unsigned int hash = 0;
    while (*str) {
        hash = (hash << 5) - hash + *str++;
    }
    return hash % HASHTABLE_SIZE;
}

// 根据地址查找符号名
const char* get_symbol_name_by_address(Elf *elf, struct elf *elf_info, uint64_t address) {
    for (int i = 0; i < elf_info->header.e_shnum; i++) {
        if (elf_info->shdrs[i].sh_type == SHT_SYMTAB) {
            Elf_Data *data = elf_getdata(elf_getscn(elf, i), NULL);
            int num_symbols = elf_info->shdrs[i].sh_size / elf_info->shdrs[i].sh_entsize;
            for (int j = 0; j < num_symbols; j++) {
                GElf_Sym sym;
                gelf_getsym(data, j, &sym);
                if (address >= sym.st_value && address < (sym.st_value + sym.st_size)) {
                    return elf_strptr(elf, elf_info->shdrs[i].sh_link, sym.st_name);
                }
            }
        }
    }
    return NULL;
}

#if 0
// 创建 ELF 文件并初始化头和节头
int create_elf_file(const char* filename) {
    // 打开文件
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("Failed to create file");
        return -1;
    }

    // 初始化 ELF
    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "ELF library is out of date\n");
        close(fd);
        return -1;
    }
    
    Elf *elf = elf_begin(fd, ELF_C_WRITE, NULL);
    if (elf == NULL) {
        fprintf(stderr, "Failed to initialize ELF: %s\n", elf_errmsg(-1));
        close(fd);
        return -1;
    }

    // 初始化 ELF 头
    Elf64_Ehdr ehdr;
    memset(&ehdr, 0, sizeof(ehdr));
    ehdr.e_ident[EI_MAG0] = ELFMAG0;
    ehdr.e_ident[EI_MAG1] = ELFMAG1;
    ehdr.e_ident[EI_MAG2] = ELFMAG2;
    ehdr.e_ident[EI_MAG3] = ELFMAG3;
    ehdr.e_ident[EI_CLASS] = ELFCLASS64;
    ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
    ehdr.e_ident[EI_VERSION] = EV_CURRENT;
    ehdr.e_type = ET_REL;
    ehdr.e_machine = EM_X86_64;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_ehsize = sizeof(ehdr);
    ehdr.e_phentsize = sizeof(Elf64_Phdr);
    ehdr.e_shentsize = sizeof(Elf64_Shdr);
    ehdr.e_shnum = 2; // 1 for section headers + 1 for the null section
    ehdr.e_shstrndx = 1; // Index of the section header string table

    if (elf_update(elf, ELF_C_WRITE) < 0) {
        fprintf(stderr, "Failed to write ELF header: %s\n", elf_errmsg(-1));
        elf_end(elf);
        close(fd);
        return -1;
    }

    // 添加节头
    Elf_Scn *scn;
    Elf64_Shdr shdr;
    memset(&shdr, 0, sizeof(shdr));
    shdr.sh_name = 0; // Null section
    shdr.sh_type = SHT_NULL;
    shdr.sh_flags = 0;
    shdr.sh_addr = 0;
    shdr.sh_offset = 0;
    shdr.sh_size = 0;
    shdr.sh_link = 0;
    shdr.sh_info = 0;
    shdr.sh_addralign = 0;
    shdr.sh_entsize = 0;

    // 创建 null section
    scn = elf_newscn(elf);
    if (!scn) {
        fprintf(stderr, "Failed to create section: %s\n", elf_errmsg(-1));
        elf_end(elf);
        close(fd);
        return -1;
    }
    if (elf_update(elf, ELF_C_WRITE) < 0) {
        fprintf(stderr, "Failed to write section header: %s\n", elf_errmsg(-1));
        elf_end(elf);
        close(fd);
        return -1;
    }

    // 创建 .text 节
    scn = elf_newscn(elf);
    if (!scn) {
        fprintf(stderr, "Failed to create section: %s\n", elf_errmsg(-1));
        elf_end(elf);
        close(fd);
        return -1;
    }

    shdr.sh_name = elf_strptr(elf, ehdr.e_shstrndx, SECTION_NAME);
    shdr.sh_type = SHT_PROGBITS;
    shdr.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    shdr.sh_addr = 0;
    shdr.sh_offset = 0;
    shdr.sh_size = 0;
    shdr.sh_link = 0;
    shdr.sh_info = 0;
    shdr.sh_addralign = 16;
    shdr.sh_entsize = 0;

    if (elf32_update(elf, ELF_C_WRITE) < 0) {
        fprintf(stderr, "Failed to write section header: %s\n", elf_errmsg(-1));
        elf_end(elf);
        close(fd);
        return -1;
    }

    // 关闭 ELF 文件和文件描述符
    elf_end(elf);
    close(fd);
    return 0;
}
#endif

// 读取 ELF 文件并进行解析
struct elf* init_elf(const char *filename, struct elf *elf_info) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open file");
        return NULL;
    }

    // 读取 ELF 文件头
    if (pread(fd, &elf_info->header, sizeof(elf_info->header), 0) != sizeof(elf_info->header)) {
        perror("Failed to read ELF header");
        close(fd);
        return NULL;
    }

    // 检查 ELF 文件是否合法
    if (elf_info->header.e_ident[EI_MAG0] != ELFMAG0 ||
        elf_info->header.e_ident[EI_MAG1] != ELFMAG1 ||
        elf_info->header.e_ident[EI_MAG2] != ELFMAG2 ||
        elf_info->header.e_ident[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "Not a valid ELF file\n");
        close(fd);
        return NULL;
    }

    // 读取程序头
    elf_info->phdrs = (Elf64_Phdr*)malloc(elf_info->header.e_phnum * sizeof(Elf64_Phdr));
    if (elf_info->phdrs == NULL) {
        perror("Failed to allocate memory for program headers");
        close(fd);
        return NULL;
    }
    if (pread(fd, elf_info->phdrs, elf_info->header.e_phnum * sizeof(Elf64_Phdr), elf_info->header.e_phoff) != (ssize_t)(elf_info->header.e_phnum * sizeof(Elf64_Phdr))) {
        perror("Failed to read program headers");
        free(elf_info->phdrs);
        close(fd);
        return NULL;
    }

    // 读取节头
    elf_info->shdrs = (Elf64_Shdr*)malloc(elf_info->header.e_shnum * sizeof(Elf64_Shdr));
    if (elf_info->shdrs == NULL) {
        perror("Failed to allocate memory for section headers");
        free(elf_info->phdrs);
        close(fd);
        return NULL;
    }
    if (pread(fd, elf_info->shdrs, elf_info->header.e_shnum * sizeof(Elf64_Shdr), elf_info->header.e_shoff) != (ssize_t)(elf_info->header.e_shnum * sizeof(Elf64_Shdr))) {
        perror("Failed to read section headers");
        free(elf_info->shdrs);
        free(elf_info->phdrs);
        close(fd);
        return NULL;
    }

    // 读取节头字符串表
    elf_info->shstrtab = (char*)malloc(elf_info->shdrs[elf_info->header.e_shstrndx].sh_size);
    if (elf_info->shstrtab == NULL) {
        perror("Failed to allocate memory for section header string table");
        free(elf_info->shdrs);
        free(elf_info->phdrs);
        close(fd);
        return NULL;
    }
    if (pread(fd, elf_info->shstrtab, elf_info->shdrs[elf_info->header.e_shstrndx].sh_size, elf_info->shdrs[elf_info->header.e_shstrndx].sh_offset) != (ssize_t)(elf_info->shdrs[elf_info->header.e_shstrndx].sh_size)) {
        perror("Failed to read section header string table");
        free(elf_info->shstrtab);
        free(elf_info->shdrs);
        free(elf_info->phdrs);
        close(fd);
        return NULL;
    }

    close(fd);
    return elf_info;
}

// 释放 ELF 信息占用的内存
void free_elf_info(struct elf *elf_info) {
    if (elf_info) {
        free(elf_info->phdrs);
        free(elf_info->shdrs);
        free(elf_info->shstrtab);
    }
}

// 从 ELF 文件中解析传入的 scn 符号表，函数符号表中的每个符号包含符号在文件中的相对地址和符号名，将函数符号存储在红黑树中
struct elf_symbols* process_symbol_table(Elf *elf, GElf_Shdr *shdr, struct elf *elf_info) {
    if (!elf || !shdr || !elf_info) {
        fprintf(stderr, "Invalid input parameters\n");
        return NULL;
    }

    Elf_Data *data = elf_getdata(elf_getscn(elf, shdr->sh_name), NULL);
    if (!data) {
        fprintf(stderr, "Failed to get section data: %s\n", elf_errmsg(-1));
        return NULL;
    }

    size_t symbol_count = shdr->sh_size / shdr->sh_entsize;
    struct elf_symbols* symbols = (struct elf_symbols*)malloc(sizeof(struct elf_symbols));
    if (!symbols) {
        fprintf(stderr, "Memory allocation failed for symbols\n");
        return NULL;
    }
    symbols->symbol_tree.rb_node = NULL; // 初始化红黑树

    for (size_t i = 0; i < symbol_count; ++i) {
        GElf_Sym sym;
        if (gelf_getsym(data, i, &sym) != &sym) {
            fprintf(stderr, "Failed to get symbol at index %zu: %s\n", i, elf_errmsg(-1));
            continue;
        }

        if (GELF_ST_TYPE(sym.st_info) == STT_FUNC) {
            const char *name = elf_strptr(elf, shdr->sh_link, sym.st_name);
            if (!name) {
                fprintf(stderr, "Failed to get symbol name: %s\n", elf_errmsg(-1));
                continue;
            }

            uint64_t sym_address = sym.st_value;
            for (int j = 0; j < elf_info->header.e_phnum; ++j) {
                Elf64_Phdr *phdr = &elf_info->phdrs[j];
                if (phdr->p_type == PT_LOAD &&
                    sym_address >= phdr->p_vaddr &&
                    sym_address < phdr->p_vaddr + phdr->p_memsz) {
                    sym_address = sym.st_value - phdr->p_vaddr + phdr->p_offset;
                    break;
                }
            }
#if 0
            // 检查红黑树中是否有相同的符号
            if (rb_find_node(&symbols->symbol_tree, name)) {
                continue;
            }
#endif
            struct symbol* new_sym = (struct symbol*)malloc(sizeof(struct symbol));
            if (!new_sym) {
                fprintf(stderr, "Memory allocation failed for symbol\n");
                return NULL;
            }

            new_sym->name = strdup(name);
            if (!new_sym->name) {
                fprintf(stderr, "Memory allocation failed for symbol name\n");
                free(new_sym);
                return NULL;
            }
            new_sym->start_addr = sym_address;
            new_sym->size = sym.st_size;

            // 插入红黑树
            rb_insert_color(&new_sym->symbol_node, &symbols->symbol_tree);
        }
    }

    return symbols;
}

/*
void merge_symbol_trees(struct rb_root *dst, struct rb_root *src) {
    struct rb_node *node = rb_first(src);
    while (node) {
        struct symbol *src_sym = rb_entry(node, struct symbol, symbol_node);
        node = rb_next(node);

        struct rb_node **new = &(dst->rb_node), *parent = NULL;
        int duplicate = 0;
        while (*new) {
            struct symbol *this = rb_entry(*new, struct symbol, symbol_node);

            parent = *new;
            int cmp = strcmp(src_sym->name, this->name);
            if (cmp < 0)
                new = &((*new)->rb_left);
            else if (cmp > 0)
                new = &((*new)->rb_right);
            else {
                // 如果符号重名，处理重名符号的情况
                // 这里选择直接跳过重名符号，可以根据实际需求自定义逻辑
                duplicate = 1;
                break;
            }
        }

        if (!duplicate) {
            struct symbol *new_sym = malloc(sizeof(struct symbol));
            if (!new_sym) {
                fprintf(stderr, "Memory allocation failed during merging symbols\n");
                continue;
            }

            new_sym->name = strdup(src_sym->name);
            new_sym->start_addr = src_sym->start_addr;
            new_sym->size = src_sym->size;
            new_sym->symbol_node.rb_left = new_sym->symbol_node.rb_right = NULL;

            rb_link_node(&new_sym->symbol_node, parent, new);
            rb_insert_color(&new_sym->symbol_node, dst);
        }
    }
}
*/

// 查找缓存中的 ELF 符号
struct elf_symbols* find_in_cache(const char* filename) {
    struct elf_cache_entry* entry = g_cache.head;
    while (entry) {
        if (strcmp(entry->filename, filename) == 0) {
            return entry->symbols;
        }
        entry = entry->next;
    }
    return NULL;
}

// 将 ELF 符号添加到缓存
void add_to_cache(const char* filename, struct elf_symbols* symbols) {
    struct elf_cache_entry* entry = (struct elf_cache_entry*)malloc(sizeof(struct elf_cache_entry));
    if (!entry) {
        fprintf(stderr, "Failed to allocate memory for cache entry\n");
        return;
    }
    entry->filename = strdup(filename);
    entry->symbols = symbols;
    entry->next = g_cache.head;
    g_cache.head = entry;
}

// 获取 ELF 文件的符号
struct elf_symbols* get_elf_func_symbols(const char* filename, struct elf* elf_info) {
    // 先查找缓存
    struct elf_symbols* cached_symbols = find_in_cache(filename);
    if (cached_symbols) {
        elf_info->syms = cached_symbols;
        return cached_symbols;
    }

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return NULL;
    }

    Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
    if (elf == NULL) {
        fprintf(stderr, "Failed to initialize ELF: %s\n", elf_errmsg(-1));
        close(fd);
        return NULL;
    }

    struct elf_symbols* final_symbols = NULL;

    size_t shstrndx;
    if (elf_getshdrstrndx(elf, &shstrndx) != 0) {
        fprintf(stderr, "Failed to get section header string index: %s\n", elf_errmsg(-1));
        elf_end(elf);
        close(fd);
        return NULL;
    }

    Elf_Scn *scn = NULL;
    GElf_Shdr symtab_shdr;
    GElf_Shdr dynsym_shdr;
    int has_symtab = 0;
    int has_dynsym = 0;

    // 第一轮遍历记录 .symtab 和.dynsym 节的位置
    // 判断是否存在 .symtab 或 .dynsym
    // 如果存在 .symtab ,则处理 .symtab 符号表
    // 如果不存在,则处理 .dynsym 符号表 (先完成一遍遍历)
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        GElf_Shdr shdr;
        if (gelf_getshdr(scn, &shdr) != &shdr) {
            fprintf(stderr, "Failed to get section header: %s\n", elf_errmsg(-1));
            continue;
        }

        if (shdr.sh_type == SHT_SYMTAB) {
            memcpy(&symtab_shdr, &shdr, sizeof(shdr));
            has_symtab = 1;
        } else if (shdr.sh_type == SHT_DYNSYM) {
            memcpy(&dynsym_shdr, &shdr, sizeof(shdr));
            has_dynsym = 1;
        }
    }

    // 判断并处理符号表
    if (has_symtab) {
        final_symbols = process_symbol_table(elf, &symtab_shdr, elf_info);
    } else if (has_dynsym) {
        final_symbols = process_symbol_table(elf, &dynsym_shdr, elf_info);
    }

    elf_end(elf);
    close(fd);

    // 将结果添加到缓存
    if (final_symbols) {
        add_to_cache(filename, final_symbols);
    }

    elf_info->syms = final_symbols;
    return final_symbols;
}

// 清理缓存
void clear_symbol_gcache() {
    struct elf_cache_entry* entry = g_cache.head;
    while (entry) {
        struct elf_cache_entry* next = entry->next;
        free(entry->filename);
        free(entry->symbols);
        free(entry);
        entry = next;
    }
    g_cache.head = NULL;
}

// 从 /proc/[pid]/stat 获取进程的启动时间
long long get_process_start_time(pid_t pid) {
    char path[256];
    FILE *file;
    long long start_time = -1;
    
    // 构建 /proc/[pid]/stat 的路径
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    
    // 打开 /proc/[pid]/stat 文件
    file = fopen(path, "r");
    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    // 读取第 22 个字段（启动时间）
    int i;
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), file) != NULL) {
        char *token = strtok(buffer, " ");
        for (i = 1; i < 22; i++) {
            token = strtok(NULL, " ");
        }
        if (token != NULL) {
            start_time = atoll(token);
        }
    }

    fclose(file);
    return start_time;
}

int is_pid_reused(pid_t pid, long long original_start_time) {
    long long current_start_time;

    // 获取当前进程的启动时间
    current_start_time = get_process_start_time(pid);
    if (current_start_time == -1) {
        // 获取启动时间失败，可能是进程不存在
        if (errno == ENOENT) {
            return 1;
        } else {
            // 处理其他错误
            perror("get_process_start_time");
            return -1;
        }
    }

    // 比较原始启动时间和当前启动时间
    if (current_start_time != original_start_time) {
        // 启动时间不同，PID 已被重新分配
        return 1;
    }

    // 启动时间相同，PID 未被重新分配
    return 0;
}

// 检查进程是否需要清理，并执行清理操作
void cleanup_process(int pid) {
    // 获取进程的启动时间
    long long original_start_time = get_process_start_time(pid);
    if (original_start_time == -1) {
        // 无法获取启动时间，可能是进程已退出
        printf("Cleaning up process data for PID: %d\n", pid);
        // 清理操作代码
        return;
    }

    // 检查进程目录是否存在
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d", pid);

    struct stat stat_buf;
    if (stat(path, &stat_buf) == -1) {
        // 进程目录不存在，说明进程已退出，需要清理
        printf("Cleaning up process data for PID: %d\n", pid);
        // 清理操作代码
        return;
    } else {
        // 检查start time，判断是否是同一个进程
        if (is_pid_reused(pid, original_start_time)) {
            // 如果PID被复用了，清理旧的进程数据
            printf("PID %d reused, cleaning up old process data\n", pid);
            // 清理操作代码
        }
    }
}

int is_numer(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }

    while (*str) {
        if (!isdigit(*str)) {
            return 0;
        }
        str++;
    }

    return 1;
}

// 扫描 /proc 目录以找到所有正在运行的进程
void scan_processes() {
    DIR *dir = opendir("/proc");
    if (!dir) {
        perror("Failed to open /proc directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // 检查名称是否为数字，即 PID
        if (is_numer(entry->d_name)) {
            int pid = atoi(entry->d_name);
            if (pid > 0) {
                cleanup_process(pid);
            }
        }
    }

    closedir(dir);
}


// 更新 ELF 文件的引用计数
void update_elf_references(const char *path) {
    if (strstr(path, ".so") || strstr(path, ".exe")) {
        // 是共享对象或可执行文件，更新ELF对象的引用计数
        upsert_elf(path);
    }
}

struct elf *elf_table[HASHTABLE_SIZE];

// 更新 ELF 引用计数
struct elf* upsert_elf(const char *filename) {                        
    unsigned int hash_value = hash(filename);
    struct elf *elf = elf_table[hash_value];
    
    if (elf) {
        elf->ref_count += 1;
        return elf;
    }
    // ELF 对象首次被引用，加载并初始化 ELF 对象
    elf = (struct elf*)malloc(sizeof(struct elf));
    if (!elf) {
        perror("Failed to allocate memory for ELF");
        exit(EXIT_FAILURE);
    }
    init_elf(filename, elf);
    get_elf_func_symbols(filename, elf);
    elf->ref_count = 1;
    elf_table[hash_value] = elf;
    return elf;
}

void start_cleanup_timer() {
    while (1) {
        scan_processes();
        sleep(30); // 每30秒执行一次
    }
}


// #ifdef TESTELF

// xxxxxx

// #endif