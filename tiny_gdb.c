#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <elf.h>

#define DEBUG 1 
struct Symbol{
    size_t address; 
    char name[0x20]; 
    struct Symbol* next; 
}; 
struct Breakpoint {
    size_t address; 
    size_t code; 
    size_t idx; 
    char name[0x20]; 
    struct breakpoint* next; 
}; 
typedef struct Symbol Symbol; 
typedef struct Breakpoint Breakpoint;

Symbol* symbol_head;
Breakpoint* breakpoint_head; 

void error(char *error_message) {
    printf("\e[31;1m1motfl:\e[0m %s\n", error_message); 
    exit(-1); 
}
void init_gdb() {
    symbol_head = (Symbol*)malloc(sizeof(Symbol)); 
    breakpoint_head = (Breakpoint*)malloc(sizeof(Breakpoint)); 
};

void parse_elf(char *filename) {
    printf("parsing\n");
    FILE *fp = fopen(filename, "r"); 
    if(!fp) {
        error("file not found"); 
    }

    Elf64_Ehdr header; 
    Elf64_Shdr section_header; 

    fread(&header, sizeof(Elf64_Ehdr), 1, fp); 
    fseek(fp, header.e_shoff, SEEK_SET); 

    for(int i = 0; i < header.e_shnum; i++) {
        fread(&section_header, sizeof(Elf64_Shdr), 1, fp);

        if(section_header.sh_type == SHT_SYMTAB) {
            Elf64_Shdr str_section_header; 
            int sym_entry_count = 0; 
            size_t str_table_offset = header.e_shoff + section_header.sh_link*sizeof(Elf64_Shdr); 
            fseek(fp,str_table_offset, SEEK_SET);  
            fread(&str_section_header, sizeof(Elf64_Shdr), 1, fp); 

            fseek(fp, section_header.sh_offset, SEEK_SET); 
            
            sym_entry_count = section_header.sh_size / section_header.sh_entsize; 

            for(int k = 0; k < sym_entry_count; k++) {
                Elf64_Sym sym; 
                fread(&sym, sizeof(Elf64_Sym), 1, fp); 
                if(ELF64_ST_TYPE(sym.st_info) == STT_FUNC && sym.st_name !=0 && sym.st_value !=0) {
                    Symbol *new_symbol = malloc(sizeof(Symbol)); 
                    new_symbol->address = sym.st_value; 
                    
                    long fileops = ftell(fp); 
                    fseek(fp, str_section_header.sh_offset+sym.st_name, SEEK_SET); 
                    fread(new_symbol->name, 0x20, sizeof(char), fp); 
                    fseek(fp, fileops, SEEK_SET); 

                    new_symbol->next = symbol_head->next; 
                    symbol_head->next = new_symbol; 
                }
            }
        }
    }

#if DEBUG
    Symbol* now = symbol_head->next; 
    while(now != NULL) {
        printf("%s 0x%llx\n", now->name, now->address); 
        now = now->next;
    }
#endif 
}

int check_command(char *command) {
    return -1; 
}

void parse_command() {

}

void init_command() {
    
}
int main(int argc, char **argv) {
    if(argc < 2) {
        error("wrong argument"); 
    }
    
    init_gdb(); 
    init_command(); 
    parse_elf(argv[1]); 

    char* command = malloc(0x20); 
    while(1) {
        printf("\e[31;1m1motfl> \e[0m");
        scanf("%32s", command); 
        int opt = check_command(command); 
        if(opt == -1) {
            printf("command error\n"); 
        } else {
            parse_command(command); 
        }
    }
    free(command); 
    return 0;
}