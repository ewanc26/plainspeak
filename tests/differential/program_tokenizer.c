#include <stdio.h>

static int classify(int c) {
    if (c == ' ') {
        return 3;
    }
    if (c >= 'a' && c <= 'z') {
        return 1;
    }
    if (c >= '0' && c <= '9') {
        return 2;
    }
    return 4;
}

int main(void) {
    int text[10] = {
        '1', '2', ' ', 'a', 'b', '+', 'c', '3', ' ', '4',
    };
    int state = 0;
    int words = 0;
    int numbers = 0;
    int symbols = 0;

    for (int i = 0; i <= 9; i++) {
        int kind = classify(text[i]);
        switch (state) {
        case 0:
            if (kind == 1) {
                state = 1;
                words += 1;
            }
            if (kind == 2) {
                state = 2;
                numbers += 1;
            }
            if (kind == 4) {
                symbols += 1;
            }
            break;
        case 1:
            if (kind == 3) {
                state = 0;
            }
            if (kind == 4) {
                state = 0;
                symbols += 1;
            }
            break;
        case 2:
            if (kind == 1) {
                state = 1;
                words += 1;
            }
            if (kind == 3) {
                state = 0;
            }
            if (kind == 4) {
                state = 0;
                symbols += 1;
            }
            break;
        default:
            break;
        }
    }

    printf("Words: %d\n", words);
    printf("Numbers: %d\n", numbers);
    printf("Symbols: %d\n", symbols);
    return 0;
}
