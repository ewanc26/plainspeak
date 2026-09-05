#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../runtime/plainspeak_runtime.h"

int main(void) {
    PsValue initial[] = {ps_int(2), ps_int(3), ps_int(5)};
    PsValue list = ps_list_from(initial, 3);
    PsValue snapshot = ps_list_copy(list);

    assert(ps_length(list) == 3);
    assert(ps_list_get(list, ps_int(2)).as.i == 3);
    assert(ps_length(snapshot) == 3);

    ps_list_set(list, ps_int(1), ps_int(13));
    assert(ps_list_get(list, ps_int(1)).as.i == 13);
    assert(ps_list_get(snapshot, ps_int(1)).as.i == 2);
    ps_list_set(list, ps_int(1), ps_int(2));

    PsValue alias = list;
    ps_list_append(alias, ps_int(7));
    assert(ps_length(list) == 4);
    assert(ps_list_get(list, ps_int(4)).as.i == 7);
    assert(ps_length(snapshot) == 3);

    ps_list_set(list, ps_int(2), ps_int(11));
    assert(ps_list_get(list, ps_int(2)).as.i == 11);
    assert(ps_list_get(snapshot, ps_int(2)).as.i == 3);

    ps_list_remove(list, ps_int(1));
    assert(ps_length(list) == 3);
    assert(ps_list_get(list, ps_int(1)).as.i == 11);
    assert(ps_length(snapshot) == 3);

    PsValue sub = ps_sub(ps_int(9), ps_int(4));
    assert(sub.type == PS_INT && sub.as.i == 5);

    PsValue mul = ps_mul(ps_int(3), ps_int(4));
    assert(mul.type == PS_INT && mul.as.i == 12);

    PsValue div = ps_div(ps_int(7), ps_int(2));
    assert(div.type == PS_INT && div.as.i == 3);

    PsValue mod = ps_mod(ps_int(7), ps_int(4));
    assert(mod.type == PS_INT && mod.as.i == 3);

    PsValue mixed = ps_div(ps_int(7), ps_double(2.0));
    assert(mixed.type == PS_DOUBLE && mixed.as.d == 3.5);

    assert(ps_as_double(ps_int(7)) == 7.0);
    assert(ps_as_double(ps_double(2.5)) == 2.5);

    FILE *old_stdout = stdout;
    FILE *capture = tmpfile();
    assert(capture);
    stdout = capture;
    ps_say_many(3, (PsValue[]){ps_int(7), ps_str("x"), ps_double(2.5)});
    fflush(capture);
    stdout = old_stdout;
    rewind(capture);
    char line[64];
    size_t n = fread(line, 1, sizeof(line) - 1, capture);
    line[n] = '\0';
    assert(strcmp(line, "7 x 2.5\n") == 0);
    fclose(capture);

    return 0;
}
