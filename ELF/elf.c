#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <libelf.h>
#include <gelf.h>

#include "process_info.h"

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

    // 读取 ELF 文件信息
    struct elf_info elf_info;
    read_elf_file(argv[1], &elf_info);

    // 打印 ELF 文件头、程序头和节头
    print_elf_header(&elf_info.header);
    print_program_headers(elf_info.phdrs, elf_info.header.e_phnum);
    print_section_headers(elf_info.shdrs, elf_info.header.e_shnum);

    // 打开 ELF 文件以进行符号表处理
    Elf *elf = elf_begin(open(argv[1], O_RDONLY), ELF_C_READ, NULL);
    if (elf == NULL) {
        fprintf(stderr, "Failed to initialize ELF: %s\n", elf_errmsg(-1));
        free_elf_info(&elf_info);
        exit(EXIT_FAILURE);
    }

    // 打印符号表
    print_symbol_table(elf, elf_info.shdrs, elf_info.header.e_shnum);

    // 查找符号名称
    uint64_t address;
    printf("Enter an address to find the corresponding symbol (in hexadecimal, e.g., 0x400000): ");
    if (scanf("%lx", &address) == 1) {
        const char *symbol_name = get_symbol_name_by_address(elf, &elf_info, address);
        if (symbol_name) {
            printf("Symbol at address 0x%lx: %s\n", address, symbol_name);
        } else {
            printf("No symbol found at address 0x%lx.\n", address);
        }
    } else {
        fprintf(stderr, "Invalid address input.\n");
    }

    // 释放资源
    elf_end(elf);
    free_elf_info(&elf_info);

    return 0;
}

