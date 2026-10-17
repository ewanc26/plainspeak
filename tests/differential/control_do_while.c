#include <stdio.h>
int main(void) {
    int x = 0;
    do {
        x = x + 1;
        if (x < 3) {
            continue;
        }
        printf("%d\n", x);
    } while (x < 4);
    printf("%d\n", x);
    return 0;
}
