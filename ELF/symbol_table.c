#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <libelf.h>
#include <gelf.h>

void print_symbol_table(int fd, Elf64_Shdr *shdrs, uint16_t shnum, char *shstrtab) {
    Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
    if (elf == NULL) {
        fprintf(stderr, "Error: elf_begin() failed: %s\n", elf_errmsg(-1));
        return;
    }

    for (int i = 0; i < shnum; i++) {
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

void find_symbol_by_address(int fd, Elf64_Shdr *shdrs, uint16_t shnum, char *shstrtab, uint64_t address) {
    Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
    if (elf == NULL) {
        fprintf(stderr, "Error: elf_begin() failed: %s\n", elf_errmsg(-1));
        return;
    }

    for (int i = 0; i < shnum; i++) {
        if (shdrs[i].sh_type == SHT_SYMTAB) {
            GElf_Shdr sym_shdr;
            gelf_getshdr(elf_getscn(elf, i), &sym_shdr);
            Elf_Data *data = elf_getdata(elf_getscn(elf, i), NULL);

            int num_symbols = sym_shdr.sh_size / sym_shdr.sh_entsize;
            for (int j = 0; j < num_symbols; j++) {
                GElf_Sym sym;
                gelf_getsym(data, j, &sym);

                if (address >= sym.st_value && address < (sym.st_value + sym.st_size)) {
                    const char *name = elf_strptr(elf, sym_shdr.sh_link, sym.st_name);
                    printf("Found symbol for address 0x%lx:\n", address);
                    printf("  Symbol: %s\n", name);
                    printf("  Value: 0x%lx\n", sym.st_value);
                    printf("  Size: %lu\n", sym.st_size);
                    printf("  Info: %u\n", sym.st_info);
                    printf("  Other: %u\n", sym.st_other);
                    printf("  Section index: %u\n", sym.st_shndx);
                    elf_end(elf);
                    return;
                }
            }
        }
    }

    printf("No symbol found for address 0x%lx\n", address);
    elf_end(elf);
}
