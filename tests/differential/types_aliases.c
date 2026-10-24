#include <stdio.h>
typedef unsigned int Count;
typedef unsigned int *IntPtr;
typedef const unsigned int *ConstIntPtr;
int main(void) {
    Count total = 7;
    printf("%u\n", total);
    ConstIntPtr p = &total;
    printf("%u\n", *p);
    return 0;
}
