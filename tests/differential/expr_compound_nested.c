#include <stdio.h>
struct point { int x; int y; };
struct box { struct point corner; int size; };
int main(void) {
    struct box b = {{1, 2}, 9};
    printf("%d\n", b.corner.x);
    printf("%d\n", b.size);
    printf("%d\n", ((int[2][3]){{4, 5}})[0][2]);
    printf("%d\n", ((int[3]){[1] = 6})[1]);
    printf("%d\n", ((struct point){.y = 8}).y);
    return 0;
}
