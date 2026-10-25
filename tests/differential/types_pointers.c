#include <stdio.h>
int main(void) {
    int x = 7;
    int *p = &x;
    *p = 42;
    printf("%d\n", *p);
    printf("%d\n", x);
    return 0;
}
