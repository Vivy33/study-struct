#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <libelf.h>
#include <gelf.h>
#include "elf.h"

void print_symbol_table(int fd, Elf64_Shdr *shdrs, uint16_t shnum, char *shstrtab) {
    Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
    if (elf == NULL) {
        fprintf(stderr, "Error: elf_begin() failed: %s\n", elf_errmsg(-1));
        return;
    }

    for (int i = 0; i < shnum; i++) {
        //检查是否为符号表
        if (shdrs[i].sh_type == SHT_SYMTAB) {
            GElf_Shdr sym_shdr;
            gelf_getshdr(elf_getscn(elf, i), &sym_shdr);
            Elf_Data *data = elf_getdata(elf_getscn(elf, i), NULL);

            int num_symbols = sym_shdr.sh_size / sym_shdr.sh_entsize;
            for (int j = 0; j < num_symbols; j++) {
                GElf_Sym sym;
                gelf_getsym(data, j, &sym);

                const char *name = elf_strptr(elf, sym_shdr.sh_link, sym.st_name);
                printf("Symbol: %s\n", name);
                printf("  Value: 0x%lx\n", sym.st_value);
                printf("  Size: %lu\n", sym.st_size);
                printf("  Info: %u\n", sym.st_info);
                printf("  Other: %u\n", sym.st_other);
                printf("  Section index: %u\n", sym.st_shndx);
            }
        }
    }

    elf_end(elf);
}
