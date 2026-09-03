#include <assert.h>
#include "../../runtime/plainspeak_runtime.h"

int main(void) {
    PsValue initial[] = {ps_int(2), ps_int(3), ps_int(5)};
    PsValue list = ps_list_from(initial, 3);

    assert(ps_length(list) == 3);
    assert(ps_list_get(list, ps_int(2)).as.i == 3);

    ps_list_append(list, ps_int(7));
    assert(ps_length(list) == 4);
    assert(ps_list_get(list, ps_int(4)).as.i == 7);

    ps_list_set(list, ps_int(2), ps_int(11));
    assert(ps_list_get(list, ps_int(2)).as.i == 11);

    ps_list_remove(list, ps_int(1));
    assert(ps_length(list) == 3);
    assert(ps_list_get(list, ps_int(1)).as.i == 11);

    return 0;
}
