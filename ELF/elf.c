#include <libelf.h>
#include <gelf.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>  // For DIR, struct dirent
#include <sys/stat.h> // For struct stat
#include <unistd.h> // For access() function

#include "process_info.h"
#include "rbtree.h"

#define ELF_MAGIC 0x464c457f

// 哈希函数
unsigned int hash(const char *str) {
    unsigned int hash = 0;
    while (*str) {
        hash = (hash << 5) - hash + *str++;
    }
    return hash % HASHTABLE_SIZE;
}

struct elf_info *elf_table[HASHTABLE_SIZE];

// 打印 ELF 文件头信息
void print_elf_header(const Elf64_Ehdr *header) {
    printf("ELF Header:\n");
    printf("  Type: %u\n", header->e_type);
    printf("  Machine: %u\n", header->e_machine);
    printf("  Version: %u\n", header->e_version);
    printf("  Entry point address: 0x%lx\n", header->e_entry);
    printf("  Start of program headers: %lu (bytes into file)\n", header->e_phoff);
    printf("  Start of section headers: %lu (bytes into file)\n", header->e_shoff);
    printf("  Flags: 0x%x\n", header->e_flags);
    printf("  Size of this header: %u (bytes)\n", header->e_ehsize);
    printf("  Size of program headers: %u (bytes)\n", header->e_phentsize);
    printf("  Number of program headers: %u\n", header->e_phnum);
    printf("  Size of section headers: %u (bytes)\n", header->e_shentsize);
    printf("  Number of section headers: %u\n", header->e_shnum);
    printf("  Section header string table index: %u\n", header->e_shstrndx);
}

// 打印程序头
void print_program_headers(const Elf64_Phdr *phdrs, uint16_t phnum) {
    printf("Program Headers:\n");
    for (int i = 0; i < phnum; ++i) {
        printf("  Type: %u\n", phdrs[i].p_type);
        printf("  Flags: 0x%x\n", phdrs[i].p_flags);
        printf("  Offset: 0x%lx\n", phdrs[i].p_offset);
        printf("  Virtual Address: 0x%lx\n", phdrs[i].p_vaddr);
        printf("  Physical Address: 0x%lx\n", phdrs[i].p_paddr);
        printf("  File Size: %lu\n", phdrs[i].p_filesz);
        printf("  Memory Size: %lu\n", phdrs[i].p_memsz);
        printf("  Alignment: %lu\n", phdrs[i].p_align);
    }
}

// 打印节头
void print_section_headers(const Elf64_Shdr *shdrs, uint16_t shnum) {
    printf("Section Headers:\n");
    for (int i = 0; i < shnum; ++i) {
        printf("  Name: %u\n", shdrs[i].sh_name);
        printf("  Type: %u\n", shdrs[i].sh_type);
        printf("  Flags: 0x%lx\n", shdrs[i].sh_flags);
        printf("  Address: 0x%lx\n", shdrs[i].sh_addr);
        printf("  Offset: %lu\n", shdrs[i].sh_offset);
        printf("  Size: %lu\n", shdrs[i].sh_size);
        printf("  Link: %u\n", shdrs[i].sh_link);
        printf("  Info: %u\n", shdrs[i].sh_info);
        printf("  Address Alignment: %lu\n", shdrs[i].sh_addralign);
        printf("  Entry Size: %lu\n", shdrs[i].sh_entsize);
    }
}

// 打印符号表
void print_symbol_table(Elf *elf, Elf64_Shdr *shdrs, uint16_t shnum) {
    for (int i = 0; i < shnum; i++) {
        if (shdrs[i].sh_type == SHT_SYMTAB) {
            Elf_Scn *scn = elf_getscn(elf, i);
            Elf_Data *data = elf_getdata(scn, NULL);
            int num_symbols = shdrs[i].sh_size / shdrs[i].sh_entsize;

            for (int j = 0; j < num_symbols; j++) {
                GElf_Sym sym;
                gelf_getsym(data, j, &sym);
                const char *name = elf_strptr(elf, shdrs[i].sh_link, sym.st_name);
                printf("Symbol: %s, Value: 0x%lx, Size: %lu\n", name, sym.st_value, sym.st_size);
            }
        }
    }
}

// 根据地址查找符号名
const char* get_symbol_name_by_address(Elf *elf, struct elf_info *elf_info, uint64_t address) {
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

// 创建 ELF 文件并初始化头和节头
int create_elf_file(const char* filename) {
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("Failed to create file");
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

    if (elf_update(elf, ELF_C_WRITE) < 0) {
        fprintf(stderr, "Failed to write ELF header: %s\n", elf_errmsg(-1));
        elf_end(elf);
        close(fd);
        return -1;
    }

    // 关闭 ELF 文件和文件描述符
    elf_end(elf);
    close(fd);
    return 0;
}

// 读取 ELF 文件并进行解析
void read_elf_file(const char *filename, struct elf_info *elf_info) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    // 读取 ELF 文件头
    if (pread(fd, &elf_info->header, sizeof(elf_info->header), 0) != sizeof(elf_info->header)) {
        perror("Failed to read ELF header");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 读取程序头
    elf_info->phdrs = malloc(elf_info->header.e_phnum * sizeof(Elf64_Phdr));
    if (pread(fd, elf_info->phdrs, elf_info->header.e_phnum * sizeof(Elf64_Phdr), elf_info->header.e_phoff) != elf_info->header.e_phnum * sizeof(Elf64_Phdr)) {
        perror("Failed to read program headers");
        free(elf_info->phdrs);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 读取节头
    elf_info->shdrs = malloc(elf_info->header.e_shnum * sizeof(Elf64_Shdr));
    if (pread(fd, elf_info->shdrs, elf_info->header.e_shnum * sizeof(Elf64_Shdr), elf_info->header.e_shoff) != elf_info->header.e_shnum * sizeof(Elf64_Shdr)) {
        perror("Failed to read section headers");
        free(elf_info->shdrs);
        free(elf_info->phdrs);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 读取节头字符串表
    elf_info->shstrtab = malloc(elf_info->shdrs[elf_info->header.e_shstrndx].sh_size);
    if (pread(fd, elf_info->shstrtab, elf_info->shdrs[elf_info->header.e_shstrndx].sh_size, elf_info->shdrs[elf_info->header.e_shstrndx].sh_offset) != elf_info->shdrs[elf_info->header.e_shstrndx].sh_size) {
        perror("Failed to read section header string table");
        free(elf_info->shstrtab);
        free(elf_info->shdrs);
        free(elf_info->phdrs);
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
}

// 释放 ELF 信息占用的内存
void free_elf_info(struct elf_info *elf_info) {
    free(elf_info->phdrs);
    free(elf_info->shdrs);
    free(elf_info->shstrtab);
}

// 释放符号节点
void free_elf_symbols(struct elf_symbol* symbol) {
    if (symbol) {
        free(symbol->name);
        free(symbol);
    }
}

struct elf_symbols* process_symbol_table(Elf *elf, GElf_Shdr *shdr, struct elf_info *elf_info) {
    Elf_Data *data = elf_getdata(elf_getscn(elf, shdr->sh_name), NULL);
    if (!data) {
        fprintf(stderr, "Failed to get section data: %s\n", elf_errmsg(-1));
        return NULL;
    }

    size_t symbol_count = shdr->sh_size / shdr->sh_entsize;
    struct elf_symbols* symbols = malloc(sizeof(struct elf_symbols));
    if (!symbols) {
        fprintf(stderr, "Memory allocation failed for symbols\n");
        return NULL;
    }
    symbols->symbol_tree.rb_node = NULL; // 初始化红黑树根

    for (size_t i = 0; i < symbol_count; ++i) {
        GElf_Sym sym;
        if (gelf_getsym(data, i, &sym) != &sym) {
            fprintf(stderr, "Failed to get symbol: %s\n", elf_errmsg(-1));
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

            // 检查红黑树中是否已有相同的符号
            if (rb_find_node(&symbols->symbol_tree, name)) {
                continue;
            }

            struct elf_symbol* new_sym = malloc(sizeof(struct elf_symbol));
            if (!new_sym) {
                fprintf(stderr, "Memory allocation failed\n");
                free_elf_symbols(new_sym);
                return NULL;
            }

            new_sym->name = strdup(name);
            new_sym->address = sym_address;
            new_sym->size = sym.st_size;

            // 插入到红黑树中
            rb_insert_symbol(&symbols->symbol_tree, new_sym);
        }
    }

    return symbols;
}

void merge_symbol_trees(struct rb_root *dst, struct rb_root *src) {
    struct rb_node *node = rb_first(src);
    while (node) {
        struct elf_symbol *src_sym = rb_entry(node, struct elf_symbol, symbol_node);
        node = rb_next(node);

        struct rb_node **new = &(dst->rb_node), *parent = NULL;
        int duplicate = 0;
        while (*new) {
            struct elf_symbol *this = rb_entry(*new, struct elf_symbol, symbol_node);

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
            struct elf_symbol *new_sym = malloc(sizeof(struct elf_symbol));
            if (!new_sym) {
                fprintf(stderr, "Memory allocation failed during merging symbols\n");
                continue;
            }

            new_sym->name = strdup(src_sym->name);
            new_sym->address = src_sym->address;
            new_sym->size = src_sym->size;
            new_sym->symbol_node.rb_left = new_sym->symbol_node.rb_right = NULL;

            rb_link_node(&new_sym->symbol_node, parent, new);
            rb_insert_color(&new_sym->symbol_node, dst);
        }
    }
}

struct elf_symbols* get_elf_func_symbols(const char* filename, struct elf_info *elf_info) {
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

    struct elf_symbols* symbols = NULL;
    struct elf_symbols* final_symbols = NULL;

    size_t shstrndx;
    if (elf_getshdrstrndx(elf, &shstrndx) != 0) {
        fprintf(stderr, "Failed to get section header string index: %s\n", elf_errmsg(-1));
        elf_end(elf);
        close(fd);
        return NULL;
    }

    Elf_Scn *scn = NULL;
    int found_symtab = 0, found_dynsym = 0;

    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        GElf_Shdr shdr;
        if (gelf_getshdr(scn, &shdr) != &shdr) {
            fprintf(stderr, "Failed to get section header: %s\n", elf_errmsg(-1));
            continue;
        }

        struct elf_symbols* symbols = NULL;
        if (shdr.sh_type == SHT_SYMTAB) {
            symbols = process_symbol_table(elf, &shdr, elf_info);
        } else if (shdr.sh_type == SHT_DYNSYM) {
            symbols = process_dynamic_symbol_table(elf, &shdr, elf_info);
        }

        if (symbols) {
            // 合并符号表
            if (final_symbols) {
                merge_symbols(final_symbols, symbols);
                free_symbols(symbols);
            } else {
                final_symbols = symbols;
            }
        }
    }

    elf_end(elf);
    close(fd);
    return final_symbols;
}

// 检查进程是否需要清理，并执行清理操作
void cleanup_process(int pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d", pid);

    struct stat stat_buf;
    if (stat(path, &stat_buf) == -1) {
        // 进程目录不存在，说明进程已退出，需要清理
        cleanup_process(pid);
    } else {
        // 检查start time，判断是否是同一个进程
        if (is_pid_reused(pid, &stat_buf)) {
            // 如果PID被复用了，清理旧的进程数据
            cleanup_process(pid);
        }
    }
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
        int pid = atoi(entry->d_name); // atoi 将字符串转换为整数
        if (pid > 0) {
            cleanup_process_if_needed(pid);
        }
    }

    closedir(dir);
}

// 更新 ELF 文件的引用计数
void update_elf_references(const char *path) {
    if (strstr(path, ".so") || strstr(path, ".exe")) {
        // 是共享对象或可执行文件，更新ELF对象的引用计数
        update_elf_ref_count(path, 1);
    }
}

// 解析进程的内存映射文件  /proc/[pid]/maps
void parse_process_maps(struct process* proc) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", proc->pid);
    
    FILE* file = fopen(path, "r");
    if (!file) {
        perror("Failed to open maps file");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char *start_addr, *end_addr, *mapped_path;
        // 假设每行格式符合规范
        sscanf(line, "%p-%p %*s %*s %*s %*s %s", &start_addr, &end_addr, mapped_path);

        update_elf_references(mapped_path);
    }

    fclose(file);
}

void update_elf_ref_count(const char *filename, int count) {
    unsigned int hash_value = hash(filename);
    struct elf_info *elf = elf_table[hash_value];
    
    if (elf) {
        elf->ref_count += count;
        if (elf->ref_count == 0) {
            free_elf_info(elf);
            elf_table[hash_value] = NULL; // 从表中移除
        }
    } else if (count > 0) {
        // ELF对象首次被引用，加载并初始化ELF对象
        elf = malloc(sizeof(struct elf_info));
        read_elf_file(filename, elf);
        elf->ref_count = count;
        elf_table[hash_value] = elf;
    }
}

void start_cleanup_timer() {
    while (1) {
        scan_processes_and_cleanup();
        sleep(30); // 每30秒执行一次
    }
}


// #ifdef TESTELF

// xxxxxx

// #endif