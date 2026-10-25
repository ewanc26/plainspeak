#include <stdio.h>
static int double_it(int value) { return value * 2; }
int main(void) {
    int (*cb)(int) = double_it;
    printf("%d\n", cb(21));
    return 0;
}
