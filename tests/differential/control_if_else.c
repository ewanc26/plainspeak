#include <stdio.h>
int main(void) {
    int x = 2;
    if (x < 3) {
        printf("small\n");
    } else {
        printf("large\n");
    }
    x = 7;
    if (x < 3) {
        printf("small\n");
    } else {
        printf("large\n");
    }
    return 0;
}
