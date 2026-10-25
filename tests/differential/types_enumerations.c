#include <stdio.h>
enum color { red, green = 5, blue };
int main(void) {
    enum color current = blue;
    printf("%d\n", current);
    return 0;
}
