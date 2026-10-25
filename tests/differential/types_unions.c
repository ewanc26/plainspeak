#include <stdio.h>
union value { int whole; double fraction; };
int main(void) {
    union value v;
    v.whole = 42;
    printf("%d\n", v.whole);
    return 0;
}
