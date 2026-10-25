#include <stdio.h>
int main(void) {
    printf("%d\n", 0x1F);
    printf("%d\n", 0XfF);
    printf("%d\n", (int)2.5e3);
    printf("%d\n", (int)5e2);
    printf("%d\n", (int)1.25E+2);
    return 0;
}
