#include <stdio.h>
int main(void) {
    char c = 65;
    signed char s = 100;
    unsigned char u = 200;
    short h = 30000;
    unsigned int w = 4000000000u;
    printf("%d\n", c);
    printf("%d\n", s + 27);
    printf("%d\n", u + 100);
    printf("%d\n", h + h);
    printf("%u\n", w);
    return 0;
}
