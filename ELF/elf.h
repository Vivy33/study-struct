#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <gelf.h>

//用于将一个数值 n 对齐到4字节边界。
//对齐到4字节边界是为了确保数据在内存中的存放符合某些硬件平台的对齐要求，从而提高内存访问的效率和正确性。
#define NOTE_ALIGN(n) (((n) + 3) & -4U)

struct elf_symbols* get_elf_func_symbols(const char* filename, struct elf* elf_info);
struct elf* upsert_elf(const char *filename);
void clear_symbol_gcache();

#endif // ELF_H