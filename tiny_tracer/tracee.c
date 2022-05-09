#include <stdio.h>

int func1() {
    printf("function1\n");
}
void func2() {
    printf("function2\n"); 
}
void func3() {
    printf("function3\n");
}

int main() {
    func1(); 
    func3(); 
    func2();
    func2();
    func3(); 
    return 0;
}