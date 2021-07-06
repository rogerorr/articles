#include <stdlib.h>

#include <stdio.h>
#include <setjmp.h>
 
static jmp_buf buf;
 
void third(void) {
    printf("second\n");         // prints
    longjmp(buf,1);             // jumps back to where setjmp was called - making setjmp now return 1
}
 
void second(void) {
    __try
    {
        third();
    }
    __finally
    {
    puts("finally");
    }
}
 
void first(void) {
    struct X
    {
    X() { puts("Hi"); }
    ~X() { puts("Bye"); }
    } anX;

    second();
    printf("first\n");          // does not print
}
 
int main() {   
    if ( ! setjmp(buf) ) {
        first();                // when executed, setjmp returns 0
    } else {                    // when longjmp jumps back, setjmp returns 1
        printf("main\n");       // prints
    }
 
    return 0;
}