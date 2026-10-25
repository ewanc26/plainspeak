#include <stdio.h>
int main(void) {
    int x = 5;
    volatile int v = 7;
    const int *p = &x;
    printf("%d\n", *p);
    printf("%d\n", v);
    return 0;
}
