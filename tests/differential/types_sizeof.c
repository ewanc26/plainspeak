#include <stdio.h>
int main(void) {
    int x = 1;
    printf("%d\n", (int)sizeof(int));
    printf("%d\n", (int)sizeof(int *));
    printf("%d\n", (int)sizeof x);
    printf("%d\n", (int)_Alignof(long double));
    return 0;
}
