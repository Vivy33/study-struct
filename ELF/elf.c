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

void print_symbol_table(int fd, Elf64_Shdr *shdrs, uint16_t shnum, char *shstrtab) {
    for (int i = 0; i < shnum; i++) {
        if (shdrs[i].sh_type == SHT_SYMTAB) {
            Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
            Elf_Scn *scn = elf_getscn(elf, i);
            Elf_Data *data = elf_getdata(scn, NULL);
            int num_symbols = shdrs[i].sh_size / shdrs[i].sh_entsize;

            for (int j = 0; j < num_symbols; j++) {
                GElf_Sym sym;
                gelf_getsym(data, j, &sym);
                const char *name = elf_strptr(elf, shdrs[i].sh_link, sym.st_name);
                printf("Symbol: %s, Value: 0x%lx, Size: %lu\n", name, sym.st_value, sym.st_size);
            }
            elf_end(elf);
        }
    }
}

void find_symbol_by_address(int fd, Elf64_Shdr *shdrs, uint16_t shnum, char *shstrtab, uint64_t address) {
    for (int i = 0; i < shnum; i++) {
        if (shdrs[i].sh_type == SHT_SYMTAB) {
            Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
            Elf_Scn *scn = elf_getscn(elf, i);
            Elf_Data *data = elf_getdata(scn, NULL);
            int num_symbols = shdrs[i].sh_size / shdrs[i].sh_entsize;

            for (int j = 0; j < num_symbols; j++) {
                GElf_Sym sym;
                gelf_getsym(data, j, &sym);
                if (address >= sym.st_value && address < (sym.st_value + sym.st_size)) {
                    const char *name = elf_strptr(elf, shdrs[i].sh_link, sym.st_name);
                    printf("Found symbol: %s\n", name);
                    elf_end(elf);
                    return;
                }
            }
            elf_end(elf);
        }
    }
    printf("Symbol not found for address: 0x%lx\n", address);
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

    // 读取并打印程序头
    elf_info->phdrs = malloc(elf_info->header.e_phnum * sizeof(Elf64_Phdr));
    if (pread(fd, elf_info->phdrs, elf_info->header.e_phnum * sizeof(Elf64_Phdr), elf_info->header.e_phoff) != elf_info->header.e_phnum * sizeof(Elf64_Phdr)) {
        perror("Failed to read program headers");
        free(elf_info->phdrs);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 读取并打印节头
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
}

const char* get_symbol_name_by_address(struct elf_info *elf_info, uint64_t address) {
    Elf *elf = elf_memory((char*) &elf_info->header, sizeof(elf_info->header));
    if (elf == NULL) {
        fprintf(stderr, "Error: elf_memory() failed: %s\n", elf_errmsg(-1));
        return NULL;
    }

    for (int i = 0; i < elf_info->header.e_shnum; i++) {
        if (elf_info->shdrs[i].sh_type == SHT_SYMTAB) {
            Elf_Data *data = elf_getdata(elf_getscn(elf, i), NULL);
            int num_symbols = elf_info->shdrs[i].sh_size / elf_info->shdrs[i].sh_entsize;
            for (int j = 0; j < num_symbols; j++) {
                GElf_Sym sym;
                gelf_getsym(data, j, &sym);
                if (address >= sym.st_value && address < (sym.st_value + sym.st_size)) {
                    const char *name = elf_strptr(elf, elf_info->shdrs[i].sh_link, sym.st_name);
                    elf_end(elf);
                    return name;
                }
            }
        }
    }

    elf_end(elf);
    return NULL;
}

void free_elf_info(struct elf_info *elf_info) {
    free(elf_info->phdrs);
    free(elf_info->shdrs);
    free(elf_info->shstrtab);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <elf-file> <address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];
    uint64_t address = strtoull(argv[2], NULL, 0);

    struct elf_info elf_info;
    read_elf_file(filename, &elf_info);

    print_elf_header(&elf_info.header);
    print_program_headers(elf_info.phdrs, elf_info.header.e_phnum);
    print_section_headers(elf_info.shdrs, elf_info.header.e_shnum);
    print_symbol_table(open(filename, O_RDONLY), elf_info.shdrs, elf_info.header.e_shnum, elf_info.shstrtab);

    const char *symbol_name = get_symbol_name_by_address(&elf_info, address);
    if (symbol_name) {
        printf("Symbol at address 0x%lx: %s\n", address, symbol_name);
    } else {
        printf("No symbol found at address 0x%lx\n", address);
    }

    free_elf_info(&elf_info);

    return 0;
}
