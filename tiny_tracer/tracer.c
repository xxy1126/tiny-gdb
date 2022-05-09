#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <elf.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>


struct BreakPoint{
    size_t address; 
    size_t oricode; 
    char name[0x20];
}breakpoint[10000]; 

int numbp = 0 ; 
void parse_elf(char *filename) {
    printf("parsing %s\n", filename); 
    Elf64_Ehdr header; 
    Elf64_Shdr section_header; 
    FILE *fp = fopen(filename, "r"); 
    if(!fp) {
        printf("fopen error\n"); 
        exit(-1); 
    }

    fread(&header, sizeof(Elf64_Ehdr), 1, fp); 
    fseek(fp, header.e_shoff, SEEK_SET); 

    for(int i = 0; i < header.e_shnum; i++) {
        fread(&section_header, sizeof(Elf64_Shdr), 1 ,fp); 
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
                if(ELF64_ST_TYPE(sym.st_info) == STT_FUNC && sym.st_name !=0 && sym.st_value!=0) {
                    long file_ops = ftell(fp); 
                    breakpoint[numbp].address = sym.st_value; 
                    
                    fseek(fp, str_section_header.sh_offset+sym.st_name, SEEK_SET); 
                    fread(breakpoint[numbp].name, 0x20, sizeof(char), fp); 

                    numbp = numbp+1; 

                    fseek(fp, file_ops, SEEK_SET); 
                }
            }
        }
    }

}

void set_breakpoint(int child) {
    for(int i = 0; i < numbp; i++) {
        breakpoint[i].oricode = ptrace(PTRACE_PEEKTEXT, child, breakpoint[i].address, 0); 
        // printf("%llx\n", breakpoint[i].oricode); 
        ptrace(PTRACE_POKETEXT, child, breakpoint[i].address, (breakpoint[i].oricode& 0xFFFFFFFFFFFFFF00) | 0xCC); 
        // size_t test_code = ptrace(PTRACE_PEEKTEXT, child, breakpoint[i].address, 0); 
        // printf("%llx\n", test_code);
    }
}

int find_breakpoint(struct user_regs_struct user_reg) {
    for(int i = 0; i < numbp; i++) {
        if(breakpoint[i].address == user_reg.rip-1) { 
            return i; 
        }
    }
    return -1; 
}

int main(int argc, char* argv[]) {
    printf("1motfl's tracer, %s\n", argv[1]); 

    int child = fork(); 
    if(!child) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl(argv[1], argv[1], NULL); 
        printf("exec error"); 
    } else {
        wait(NULL); 
        parse_elf(argv[1]); 

        // for(int i = 0; i < numbp; i++) {
        //     printf("%p %s\n", breakpoint[i].address, breakpoint[i].name);
        // }

        set_breakpoint(child); 
        ptrace(PTRACE_CONT, child, 0, 0); 

        int status = 0; 
        struct user_regs_struct user_reg; 
        while(1) {
            waitpid(child, &status, 0); 

            if(WIFEXITED(status)) {
                printf("exit!\n"); 
                return 0; 
            }
            if(WIFSTOPPED(status)) {
                if(WSTOPSIG(status) == SIGTRAP) {
                    ptrace(PTRACE_GETREGS, child, 0, &user_reg); 

                    int idx = find_breakpoint(user_reg); 
                    if(idx == -1) {
                        printf("No breakpoint\n"); 
                        exit(-1); 
                    } 


                    printf("%s\n", breakpoint[idx].name); 

                    user_reg.rip = user_reg.rip - 1; 
                    ptrace(PTRACE_SETREGS, child, 0, &user_reg); 
                    ptrace(PTRACE_POKETEXT, child, breakpoint[idx].address, breakpoint[idx].oricode); 

                    ptrace(PTRACE_SINGLESTEP, child, 0, 0); 
                    wait(NULL); 
                    ptrace(PTRACE_POKETEXT, child, breakpoint[idx].address, (breakpoint[idx].oricode& 0xFFFFFFFFFFFFFF00) | 0xCC); 

                }
            }
            ptrace(PTRACE_CONT, child, 0, 0);
        }
    }
    return 0; 
}