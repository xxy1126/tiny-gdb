#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <elf.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>


#define DEBUG 0
#define numOfCommand 0x10

struct Symbol{
    size_t address; 
    struct Symbol* next; 
    char name[0x20]; 
}; 
struct Breakpoint {
    size_t address; 
    size_t code; 
    size_t idx; 
    struct Breakpoint* next; 
    char name[0x20]; 
}; 
struct Operate {
    char name[0x10]; 
    int isbreak; 
    void (*run)(char *command); 
};

typedef struct Symbol Symbol; 
typedef struct Breakpoint Breakpoint;
typedef struct Operate Operate; 

Operate instrution[numOfCommand];

Symbol* symbol_head;
Breakpoint* breakpoint_head; 
size_t entry = 0; 
size_t numofBreakpoint = 0;
int child; 

void quit_command(char *command); 
void breakpoint_command(char *command); 
void continue_command(); 
void info_command(char *command);
void delete_command(char *arg);
void delete_breakpoint(int idx); 
void info_breakpoint(); 
void info_register();
void set_breakpoint(Symbol *target); 
Breakpoint* find_breakpoint(struct user_regs_struct user_reg); 


void message(char *content) {
    printf("\e[31;1m1motfl>\e[0m %s\n", content);  
}
void error(char *error_message) {
    printf("\e[31;1m1motfl:\e[0m %s\n", error_message); 
    exit(-1); 
}
void init_gdb_var() {
    message("init gdb variable");
    symbol_head = (Symbol*)malloc(sizeof(Symbol)); 
    breakpoint_head = (Breakpoint*)malloc(sizeof(Breakpoint)); 
};
void init_command() {
    message("init gdb command");

    strcpy(instrution[0].name, "quit"); 
    instrution[0].run = quit_command; 

    strcpy(instrution[1].name, "breakpoint"); 
    instrution[1].run = breakpoint_command; 
    instrution[1].isbreak = 0; 

    strcpy(instrution[2].name, "continue"); 
    instrution[2].run = continue_command; 
    instrution[2].isbreak = 1; 

    strcpy(instrution[3].name, "info"); 
    instrution[3].run = info_command; 
    instrution[3].isbreak = 0; 

    strcpy(instrution[4].name, "delete"); 
    instrution[4].run = delete_command; 
    instrution[4].isbreak = 0;  
}
void continue_command() {
    ptrace(PTRACE_CONT, child, 0, 0);
}
void info_command(char* command) {
    // printf("%s\n", command);
    if(!strcmp(command, "breakpoint")) {
        info_breakpoint(); 
    } else if(!strcmp(command, "register")) {
        info_register(); 
    } else {
        error("gg");
    }
}
void info_breakpoint() {
    Breakpoint *target = breakpoint_head->next; 
    printf("%5s%20s%10s\n", "id", "address", "where");
    while(target != NULL) {
        printf("%5lu  0x%016lxx%10s\n", target->idx, target->address, target->name);
        target = target->next;
    }
}
void info_register() {
    struct user_regs_struct user_reg; 
    ptrace(PTRACE_GETREGS, child, 0, &user_reg); 
    printf("%7s 0x%016llx\n", "rax", user_reg.rax); 
    printf("%7s 0x%016llx\n", "rdi", user_reg.rdi); 
    printf("%7s 0x%016llx\n", "rsi", user_reg.rsi); 
    printf("%7s 0x%016llx\n", "rdx", user_reg.rdx); 
    printf("%7s 0x%016llx\n", "rcx", user_reg.rcx); 
    printf("%7s 0x%016llx\n", "rsp", user_reg.rsp); 
    printf("%7s 0x%016llx\n", "rbp", user_reg.rbp); 
    printf("%7s 0x%016llx\n", "rip", user_reg.rip); 
}
void quit_command(char *commmand) {
    exit(0);
}
void breakpoint_command(char *arg) {
#if DEBUG 
    printf("set breakpoint at %s\n", arg);
#endif 
    int isAddress = (arg[0] == '*'); 
    size_t address; 
    if(isAddress) {
        address = strtoll(arg[1], NULL, 16); 
    }

    Symbol* now = symbol_head->next; 
    while(now != NULL) {
        if((!isAddress) && (!memcmp(now->name, arg, strlen(now->name)))) {
            break;
        } else if(isAddress && address == now->address) {
            break;
        }
        now = now->next;
    }
    if(now!=NULL) 
        set_breakpoint(now); 
    else 
        error("not found breakpoint");
}
void delete_command(char *arg) {
    int idx = atoi(arg);
    // printf("idx is %d\n", idx);
    delete_breakpoint(idx); 
}
void delete_breakpoint(int idx) {
    Breakpoint *target = breakpoint_head; 
    while(target != NULL) {
        Breakpoint *next = target->next; 
        if(next->idx == idx) {
            target->next = next->next; 
            break; 
        }
        target = target->next;
    }
}
Breakpoint* find_breakpoint(struct user_regs_struct user_reg) {
    Breakpoint* target = breakpoint_head->next; 
    while(target != NULL) {
        if(target->address == user_reg.rip-1) {
            return target; 
        }
        target = target->next; 
    }
    return NULL; 
}
void set_breakpoint(Symbol *target) {
    printf("set_breakpoint %s 0x%lu\n", target->name, target->address);
    Breakpoint* new_breakpoint = (Breakpoint*)malloc(sizeof(Breakpoint)); 

    new_breakpoint->address = target->address; 
    strcpy(new_breakpoint->name, target->name); 
    new_breakpoint->idx =  ++numofBreakpoint;  
    new_breakpoint->code = ptrace(PTRACE_PEEKTEXT, child, new_breakpoint->address, 0);
    ptrace(PTRACE_POKETEXT, child, new_breakpoint->address, (new_breakpoint->code& 0xFFFFFFFFFFFFFF00) | 0xCC);

    new_breakpoint->next = breakpoint_head->next; 
    breakpoint_head->next = new_breakpoint; 
#if DEBUG
    size_t a = ptrace(PTRACE_PEEKDATA, child, new_breakpoint->address, 0);
    printf("%lx %s\n", a, new_breakpoint->name);
#endif 
}
void parse_elf(char *filename) {
    message("parsing elf");
    FILE *fp = fopen(filename, "r"); 
    if(!fp) {
        error("file not found"); 
    }

    Elf64_Ehdr header; 
    Elf64_Shdr section_header; 

    fread(&header, sizeof(Elf64_Ehdr), 1, fp); 
    fseek(fp, header.e_shoff, SEEK_SET); 
    entry = header.e_entry;
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


int parse_command(char *command) {
    char *opt = strtok(command, " ");  
    for(int i = 0; i < numOfCommand; i++) {
        if(!strcmp(opt, instrution[i].name)) {              
            char *arg = strtok(NULL, " "); 
            instrution[i].run(arg);
            return instrution[i].isbreak;  
        }
    }
#if DEBUG 
    printf("%s\n%s\n", command, opt);
#endif 
    return 0; 
}

int main(int argc, char **argv) {
    if(argc < 2) {
        error("wrong argument"); 
    }
    
    init_gdb_var(); 
    init_command(); 
    parse_elf(argv[1]); 
    
    child = fork(); 
    if(!child) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL); 
        execl(argv[1], argv[1], NULL); 
        error("exec failed"); 
    }

    wait(NULL);
    char* breakmain = malloc(0x20); 
    strcpy(breakmain, "breakpoint main");
    parse_command(breakmain);

    ptrace(PTRACE_CONT, child, 0, 0);

    char* command = malloc(0x20); 
    int status = 0; 
    struct user_regs_struct user_reg;
    while(1) {
        waitpid(child,&status, 0);

        if(WIFEXITED(status)) {
            error("program exit"); 
        }
        if(WIFSTOPPED(status)) {
            if(WSTOPSIG(status) == SIGTRAP) {
                ptrace(PTRACE_GETREGS, child, 0, &user_reg); 
            
                Breakpoint *target = find_breakpoint(user_reg); 

                printf("breakpoint at %s\n", target->name);

                user_reg.rip = user_reg.rip - 1; 
                ptrace(PTRACE_SETREGS, child, 0, &user_reg); 

                ptrace(PTRACE_POKETEXT, child, target->address, target->code); 
                ptrace(PTRACE_SINGLESTEP, child, 0, 0); 
                wait(NULL); 
                ptrace(PTRACE_POKETEXT, child, target->address, target->code&0xFFFFFFFFFFFFFF00 + 0xCC); 
            }
        }
        while(1) {
            printf("\e[31;1m1motfl> \e[0m");
            fgets(command, 0x20, stdin);
            if(command[strlen(command)] == '\n') {
                command[strlen(command)] = '\x00';
            }
            int isbreak = parse_command(command); 
            if(isbreak) {
                break; 
            }
        }
    }
    free(command); 
    return 0;
}