#include <stdio.h>
struct point { int x; int y; };
int main(void) {
    struct point p;
    p.x = 10;
    p.y = 20;
    printf("%d\n", p.x);
    printf("%d\n", p.y);
    return 0;
}
