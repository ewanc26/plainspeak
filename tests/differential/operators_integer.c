#include <stdio.h>
int main(void) {
    int a = 13;
    int b = 4;
    printf("%d\n", a + b);
    printf("%d\n", a - b);
    printf("%d\n", a * b);
    printf("%d\n", a / b);
    printf("%d\n", a % b);
    printf("%d\n", a & b);
    printf("%d\n", a | b);
    printf("%d\n", a ^ b);
    printf("%d\n", ~a);
    printf("%d\n", a << 2);
    printf("%d\n", a >> 1);
    printf("%d\n", a > b);
    printf("%d\n", a == 13);
    printf("%d\n", a < b);
    printf("%d\n", (a > b) && (b < a));
    printf("%d\n", !(a == b));
    printf("%d\n", -a + b);
    return 0;
}
