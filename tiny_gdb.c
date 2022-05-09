#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void error(char *error_message) {
    printf("error: %s\n", error_message); 
    exit(-1); 
}

void parse_elf(char *filename) {
    printf("parsing\n");
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
    return 0;
}