#include <stdio.h>

struct record {
    int id;
    int score;
};

static void show(struct record row) {
    printf("Student %d scored %d\n", row.id, row.score);
}

int main(void) {
    struct record table[4] = {
        {101, 72},
        {102, 95},
        {103, 88},
        {104, 60},
    };

    for (int i = 0; i <= 2; i++) {
        int best = i;
        for (int j = i + 1; j <= 3; j++) {
            if (table[j].score > table[best].score) {
                best = j;
            }
        }
        struct record hold = table[i];
        table[i] = table[best];
        table[best] = hold;
    }

    for (int k = 0; k <= 3; k++) {
        show(table[k]);
    }
    return 0;
}
