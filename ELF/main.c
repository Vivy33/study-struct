#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "elf.h"

void print_elf_header(const Elf64_Ehdr *header) {
    // Add your implementation to print ELF header details
}

void print_program_headers(const Elf64_Phdr *phdrs, int phnum) {
    // Add your implementation to print program headers
}

void print_section_headers(const Elf64_Shdr *shdrs, int shnum) {
    // Add your implementation to print section headers
}

void read_elf_file(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Read ELF header
    Elf64_Ehdr header;
    if (read(fd, &header, sizeof(header)) != sizeof(header)) {
        perror("read");
        close(fd);
        exit(EXIT_FAILURE);
    }

    print_elf_header(&header);

    // Read Program Headers
    if (lseek(fd, header.e_phoff, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        exit(EXIT_FAILURE);
    }
    Elf64_Phdr *phdrs = malloc(header.e_phnum * sizeof(Elf64_Phdr));
    if (read(fd, phdrs, header.e_phnum * sizeof(Elf64_Phdr)) != header.e_phnum * sizeof(Elf64_Phdr)) {
        perror("read");
        free(phdrs);
        close(fd);
        exit(EXIT_FAILURE);
    }

    print_program_headers(phdrs, header.e_phnum);
    free(phdrs);

    // Read Section Headers
    if (lseek(fd, header.e_shoff, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        exit(EXIT_FAILURE);
    }
    Elf64_Shdr *shdrs = malloc(header.e_shnum * sizeof(Elf64_Shdr));
    if (read(fd, shdrs, header.e_shnum * sizeof(Elf64_Shdr)) != header.e_shnum * sizeof(Elf64_Shdr)) {
        perror("read");
        free(shdrs);
        close(fd);
        exit(EXIT_FAILURE);
    }

    print_section_headers(shdrs, header.e_shnum);
    free(shdrs);

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ELF file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    read_elf_file(argv[1]);
    return EXIT_SUCCESS;
}
