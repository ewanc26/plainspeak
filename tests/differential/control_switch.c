#include <stdio.h>
int main(void) {
    int level = 2;
    switch (level) {
    case 1: printf("low\n"); break;
    case 2: printf("medium\n"); break;
    default: printf("high\n"); break;
    }
    int code = 9;
    switch (code) {
    case 2: printf("second\n"); break;
    default: printf("other\n"); break;
    }
    return 0;
}
