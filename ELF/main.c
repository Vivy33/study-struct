#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "elf.h"

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

// 读取 ELF 文件并进行解析
void read_elf_file(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // 读取 ELF 文件头
    Elf64_Ehdr header;
    if (pread(fd, &header, sizeof(header), 0) != sizeof(header)) {
        perror("pread");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 打印 ELF 文件头信息
    print_elf_header(&header);

    // 读取并打印程序头
    Elf64_Phdr *phdrs = malloc(header.e_phnum * sizeof(Elf64_Phdr));
    if (pread(fd, phdrs, header.e_phnum * sizeof(Elf64_Phdr), header.e_phoff) != header.e_phnum * sizeof(Elf64_Phdr)) {
        perror("pread");
        free(phdrs);
        close(fd);
        exit(EXIT_FAILURE);
    }
    print_program_headers(phdrs, header.e_phnum);
    free(phdrs);

    // 读取并打印节头
    Elf64_Shdr *shdrs = malloc(header.e_shnum * sizeof(Elf64_Shdr));
    if (pread(fd, shdrs, header.e_shnum * sizeof(Elf64_Shdr), header.e_shoff) != header.e_shnum * sizeof(Elf64_Shdr)) {
        perror("pread");
        free(shdrs);
        close(fd);
        exit(EXIT_FAILURE);
    }
    print_section_headers(shdrs, header.e_shnum);

    // 读取节头字符串表
    char *shstrtab = malloc(shdrs[header.e_shstrndx].sh_size);
    if (pread(fd, shstrtab, shdrs[header.e_shstrndx].sh_size, shdrs[header.e_shstrndx].sh_offset) != shdrs[header.e_shstrndx].sh_size) {
        perror("pread");
        free(shstrtab);
        free(shdrs);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 打印符号表
    print_symbol_table(fd, shdrs, header.e_shnum, shstrtab);

    free(shstrtab);
    free(shdrs);
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ELF file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // 读取并解析 ELF 文件
    read_elf_file(argv[1]);
    return EXIT_SUCCESS;
}

