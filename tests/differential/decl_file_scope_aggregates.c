#include <stdio.h>
struct point { int x; int y; };
const int sparse[2][3] = {[1] = {[2] = 7}};
const struct point p = {1, 4};
int main(void) {
    printf("%d\n", sparse[1][2]);
    printf("%d\n", p.y);
    return 0;
}
