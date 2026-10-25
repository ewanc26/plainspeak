#include <stdio.h>
int main(void) {
    int grid[2][3] = {{1, 5, 9}, {2, 6}};
    int sparse[2][3] = {[1] = {[2] = 7}};
    printf("%d\n", grid[0][2]);
    printf("%d\n", grid[1][2]);
    printf("%d\n", sparse[1][0]);
    printf("%d\n", sparse[1][2]);
    return 0;
}
