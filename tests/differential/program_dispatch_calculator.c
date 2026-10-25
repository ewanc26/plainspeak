#include <stdio.h>

static int add(int a, int b) {
    return a + b;
}

static int subtract(int a, int b) {
    return a - b;
}

static int multiply(int a, int b) {
    return a * b;
}

static int divide(int a, int b) {
    return a / b;
}

int main(void) {
    int (*ops[4])(int, int) = {add, subtract, multiply, divide};
    int codes[5] = {0, 3, 2, 3, 7};
    int lefts[5] = {12, 84, 6, 9, 1};
    int rights[5] = {4, 0, 7, 3, 1};
    int failures = 0;

    for (int i = 0; i <= 4; i++) {
        int code = codes[i];
        if (code < 0 || code > 3) {
            printf("Error: unknown operation %d.\n", code);
            failures += 1;
        } else {
            if (code == 3 && rights[i] == 0) {
                printf("Error: division by zero.\n");
                failures += 1;
            } else {
                printf("Result: %d\n", ops[code](lefts[i], rights[i]));
            }
        }
    }

    printf("Failures: %d\n", failures);
    return 0;
}
