#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/reg.h>   /* For constants  ORIG_RAX etc */
#include <stdio.h>
int main()
{  
 
    char * argv[ ]={"ls","-al","/etc/passwd",(char *)0};
    char * envp[ ]={"PATH=/bin",0};
    pid_t child;
    long orig_rax;
    child = fork();
    if(child == 0)
    {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        printf("Try to call: execl\n");
        execve("/bin/ls",argv,envp);
        printf("child exit\n");
    }
    else
    {
        wait(NULL);             //等待子进程
        orig_rax = ptrace(PTRACE_PEEKUSER,
                          child, 8 * ORIG_RAX,
                          NULL);
        printf("The child made a "
               "system call %ld\n", orig_rax);
        ptrace(PTRACE_CONT, child, NULL, NULL);
        printf("Try to call:ptrace\n");
    }
    return 0;
}
