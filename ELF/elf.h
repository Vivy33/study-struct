#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <gelf.h>

// Define the ELF header structure
#define EI_NIDENT 16
//用于将一个数值 n 对齐到4字节边界。
//对齐到4字节边界是为了确保数据在内存中的存放符合某些硬件平台的对齐要求，从而提高内存访问的效率和正确性。
#define NOTE_ALIGN(n) (((n) + 3) & -4U)

void print_elf_header(const Elf64_Ehdr *header);
void print_program_headers(const Elf64_Phdr *phdrs, uint16_t phnum);
void print_section_headers(const Elf64_Shdr *shdrs, uint16_t shnum);
void print_symbol_table(int fd, Elf64_Shdr *shdrs, uint16_t shnum, char *shstrtab);
void print_symbol_table(int fd, Elf64_Shdr *shdrs, uint16_t shnum, char *shstrtab);
void find_symbol_by_address(int fd, Elf64_Shdr *shdrs, uint16_t shnum, char *shstrtab, uint64_t address);

#endif // ELF_H