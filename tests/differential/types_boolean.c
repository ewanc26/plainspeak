#include <stdbool.h>
#include <stdio.h>
int main(void) {
    bool flag = true;
    bool other = false;
    printf("%d\n", flag);
    printf("%d\n", other);
    printf("%d\n", flag && other);
    printf("%d\n", flag || other);
    bool big = 5;
    printf("%d\n", big);
    return 0;
}
