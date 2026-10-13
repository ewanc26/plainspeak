#include <stdio.h>
int main(void) {
    int x = 0;
    while (x < 6) {
        x = x + 1;
        if (x == 2) {
            continue;
        }
        printf("%d\n", x);
        if (x == 4) {
            break;
        }
    }
    printf("%d\n", x);
    for (int n = 1; n <= 5; n++) {
        if (n == 3) {
            continue;
        }
        if (n == 5) {
            break;
        }
        printf("%d\n", n);
    }
    return 0;
}
