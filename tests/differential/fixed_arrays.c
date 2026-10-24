#include <stdbool.h>
#include <stdio.h>
int main(void) {
    int values[4] = {3, 1, 4, 1};
    int total = 0;
    for (int i = 0; i < 4; i++) {
        total = total + values[i];
    }
    values[2] = 9;
    printf("%d\n", total);
    printf("%d\n", values[2]);
    bool flags[2] = {true, false};
    printf("%d\n", flags[1]);
    return 0;
}
