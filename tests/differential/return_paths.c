#include <stdio.h>
static int classify(int n) {
    if (n < 0) return -1;
    if (n > 0) return 1;
    return 0;
}
static int modulus(int a, int b) {
    switch (b) {
        case 0: return -1;
        default: return a % b;
    }
}
static int pick(int n) {
    for (int i = 1; i <= 3; i++) {
        if (i == n) return i;
    }
    return 0;
}
static int looped(int n) {
    while (n < 10) {
        if (n > 5) return n;
        n += 1;
    }
    return -1;
}
static int jumped(int n) {
    if (n == 0) goto zero;
    return 1;
zero:
    return 0;
}
static int repeatable(int n) {
    return n;
}
int main(void) {
    printf("%d\n", classify(-3));
    printf("%d\n", classify(0));
    printf("%d\n", classify(4));
    printf("%d\n", modulus(10, 5));
    printf("%d\n", modulus(10, 0));
    printf("%d\n", pick(2));
    printf("%d\n", pick(9));
    printf("%d\n", looped(3));
    printf("%d\n", looped(8));
    printf("%d\n", jumped(0));
    printf("%d\n", jumped(3));
    printf("%d\n", repeatable(42));
    return 0;
}
