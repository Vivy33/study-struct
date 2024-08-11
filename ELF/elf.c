#include <libelf.h>
#include <gelf.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "process_info.h"

struct elf_symbol {
    char* name;
    uint64_t address;
    uint64_t size;
    struct elf_symbol* next;
};

// 哈希函数
unsigned int hash(const char* name) {
    unsigned int hash = 5381;
    int c;
    while ((c = *name++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % HASHTABLE_SIZE;
}

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

// 处理符号表节的逻辑，返回哈希表
struct hash_table* process_symbol_table(Elf *elf, GElf_Shdr *shdr) {
    Elf_Data *data = elf_getdata(elf_getscn(elf, shdr->sh_name), NULL);
    if (!data) {
        fprintf(stderr, "Failed to get section data: %s\n", elf_errmsg(-1));
        return NULL;
    }

    size_t symbol_count = shdr->sh_size / shdr->sh_entsize;
    struct hash_table* table = malloc(sizeof(struct hash_table));
    if (!table) {
        fprintf(stderr, "Memory allocation failed for hash table\n");
        return NULL;
    }
    memset(table, 0, sizeof(struct hash_table)); // 初始化哈希表

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

            struct elf_symbol* new_sym = malloc(sizeof(struct elf_symbol));
            if (!new_sym) {
                fprintf(stderr, "Memory allocation failed\n");
                free_elf_symbols(table->nodes[hash(name)]);
                free(table);
                return NULL;
            }

            new_sym->name = strdup(name);
            new_sym->address = sym.st_value;
            new_sym->size = sym.st_size;
            new_sym->next = NULL;

            // 插入到哈希表
            unsigned int index = hash(name);
            new_sym->next = table->nodes[index]; // 在哈希槽中插入
            table->nodes[index] = new_sym;
        }
    }

    return table;
}

// 获取 ELF 文件的符号信息
struct hash_table* get_elf_func_symbols(const char* filename) {
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

    struct hash_table* table = NULL;

    // 遍历节头，找到符号表
    size_t shstrndx;
    if (elf_getshdrstrndx(elf, &shstrndx) != 0) {
        fprintf(stderr, "Failed to get section header string index: %s\n", elf_errmsg(-1));
        elf_end(elf);
        close(fd);
        return NULL;
    }

    Elf_Scn *scn = NULL;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        GElf_Shdr shdr;
        if (gelf_getshdr(scn, &shdr) != &shdr) {
            fprintf(stderr, "Failed to get section header: %s\n", elf_errmsg(-1));
            continue;
        }

        // 处理符号表节
        if (shdr.sh_type == SHT_SYMTAB || shdr.sh_type == SHT_DYNSYM) {
            table = process_symbol_table(elf, &shdr);
            if (!table) {
                fprintf(stderr, "Failed to process symbol table\n");
            }
            break; // 只处理第一个找到的符号表
        }
    }

    elf_end(elf);
    close(fd);
    return table;
}

// 打印哈希表中的符号信息
void print_elf_symbols(struct hash_table* table) {
    for (int i = 0; i < HASHTABLE_SIZE; i++) {
        struct elf_symbol* symbol = table->nodes[i];
        while (symbol) {
            printf("Function: %s, Address: 0x%lx, Size: %lu\n", symbol->name, symbol->address, symbol->size);
            symbol = symbol->next;
        }
    }
}

// 主函数
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <elf-file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 初始化 libelf
    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "ELF library initialization failed: %s\n", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }

    // 获取 ELF 符号信息
    struct hash_table* symbols = get_elf_func_symbols(argv[1]);
    if (!symbols) {
        fprintf(stderr, "Failed to get symbols from ELF file.\n");
        exit(EXIT_FAILURE);
    }

    // 打印符号信息
    print_elf_symbols(symbols);

    // 释放符号哈希表内存
    for (int i = 0; i < HASHTABLE_SIZE; i++) {
        free_elf_symbols(symbols->nodes[i]);
    }
    free(symbols);

    return 0;
}