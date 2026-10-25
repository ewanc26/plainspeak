#include <stdio.h>
struct point { int x; int y; };
int main(void) {
    struct point p = {1, 2};
    printf("%d\n", p.y);
    return 0;
}
