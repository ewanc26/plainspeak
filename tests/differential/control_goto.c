#include <stdio.h>
int main(void) {
    int attempts = 0;
try_again:
    attempts = attempts + 1;
    if (attempts < 3) {
        goto try_again;
    }
    printf("%d\n", attempts);
    goto wrapped_up;
    printf("never printed\n");
wrapped_up:
    printf("done\n");
    return 0;
}
