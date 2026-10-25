#include <stdio.h>
int main(void) {
    int grid[2][3];
    grid[0][0] = 1;
    grid[0][1] = 5;
    grid[0][2] = 9;
    grid[1][0] = 2;
    grid[1][1] = 6;
    grid[1][2] = 10;
    printf("%d\n", grid[0][2]);
    printf("%d\n", grid[1][1]);
    return 0;
}
