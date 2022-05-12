#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <elf.h>
#include <sys/ptrace.h>

#define DEBUG 1 
#define numOfCommand 0x10

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
struct Operate {
    char name[0x10]; 
    void (*run)(char *command); 
};

typedef struct Symbol Symbol; 
typedef struct Breakpoint Breakpoint;
typedef struct Operate Operate; 

Operate instrution[numOfCommand];

Symbol* symbol_head;
Breakpoint* breakpoint_head; 


void quit_command(char *command); 
void breakpoint_command(char *command); 
void set_breakpoint(Symbol *target); 


void error(char *error_message) {
    printf("\e[31;1m1motfl:\e[0m %s\n", error_message); 
    exit(-1); 
}
void init_gdb() {
    symbol_head = (Symbol*)malloc(sizeof(Symbol)); 
    breakpoint_head = (Breakpoint*)malloc(sizeof(Breakpoint)); 
};
void init_command() {
    strcpy(instrution[0].name, "quit"); 
    instrution[0].run = quit_command; 

    strcpy(instrution[1].name, "breakpoint"); 
    instrution[1].run = breakpoint_command; 
}

void quit_command(char *commmand) {
    exit(0);
}
void breakpoint_command(char *arg) {
#if DEBUG 
    printf("set breakpoint at %s\n", arg);
#endif 
    bool isAddress = arg[0] == '*'; 
    size_t address; 
    if(isAddress) {
        address = strtoll(arg[1]); 
    }

    Symbol* now = symbol_head->next; 
    while(now != NULL) {
        if((!isAddress) && (!strcmp(now->name, arg))) {
            break;
        } else if(isAddress && address == now->address) {
            break;
        }
    }
    set_breakpoint(now); 
}
void set_breakpoint(Symbol *target) {
    Breakpoint* now = breakpoint_head->next; 

}
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


void parse_command(char *command) {
    char *opt = strtok(command, " ");  
    for(int i = 0; i < numOfCommand; i++) {
        if(!strcmp(opt, instrution[i].name)) {
            char *arg = strtok(NULL, " "); 
            instrution[i].run(arg); 
        }
    }
    return ; 
}
void init_gbd_run(char *filename) {
    int child = fork(); 
    if(!child) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL); 
        execl(argv[1], argv[1], NULL); 
        error("exec failed"); 
    } else {
        wait(NULL); 
        return ; 
    }
}
int main(int argc, char **argv) {
    if(argc < 2) {
        error("wrong argument"); 
    }
    
    init_gdb_var(); 
    init_command(); 
    parse_elf(argv[1]); 
    init_gdb_run(argv[1]); 


    char* command = malloc(0x20); 
    while(1) {
        printf("\e[31;1m1motfl> \e[0m");
        fgets(command, 0x20, stdin);
        parse_command(command); 
    }
    free(command); 
    return 0;
}