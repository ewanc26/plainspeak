#include <stdio.h>
int main(void) {
    int a = 1;
    printf("%d\n", (++a, a));
    return 0;
}
