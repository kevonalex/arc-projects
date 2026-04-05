#include <stdio.h>

int x = 42;
int *p = &x;

int main() {   
    printf("%d\n", x);
    printf("%p\n", (void*)p);
    printf("%d\n", *p);
    printf("\n\n\n");

    int y = 42;
    int *o = &y;
    printf("y value: %d\n", y);
    printf("y address: %p\n", (void*)&y);
    printf("o value: %p\n", (void*)o);
    printf("o type: %zu bytes\n", sizeof(p));
    printf("*o value: %d\n", *p);
    return 0;
}