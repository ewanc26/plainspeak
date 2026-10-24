#include <assert.h>
#include <stdio.h>
_Static_assert(1 == 1, "arithmetic");
int main(void) {
    assert(1 == 1);
    printf("assertions passed\n");
    return 0;
}
