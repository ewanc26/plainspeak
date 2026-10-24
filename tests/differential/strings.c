#include <stdio.h>
#include <string.h>
int main(void) {
    const char *greeting = "Hello";
    printf("%s\n", greeting);
    printf("a \"quoted\" word\n");
    printf("%s, world\n", greeting);
    printf("%d\n", strcmp(greeting, "Hello") == 0);
    return 0;
}
