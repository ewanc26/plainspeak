#include <stdio.h>

#include "ffi_user_helper.c"

int main(void) {
    printf("%ld\n", scale_by(3L, 4L));
    return 0;
}
