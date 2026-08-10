#include "plainspeak_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PsValue ps_int(long v) {
    PsValue r;
    r.type = PS_INT;
    r.as.i = v;
    return r;
}

PsValue ps_str(const char *v) {
    PsValue r;
    r.type = PS_STRING;
    r.as.s = v;
    return r;
}

static void ps_type_error(const char *op, PsValue a, PsValue b) {
    fprintf(stderr,
            "runtime error: cannot %s a %s and a %s\n",
            op,
            a.type == PS_INT ? "number" : "string",
            b.type == PS_INT ? "number" : "string");
    exit(1);
}

PsValue ps_add(PsValue a, PsValue b) {
    if (a.type == PS_INT && b.type == PS_INT) {
        return ps_int(a.as.i + b.as.i);
    }
    if (a.type == PS_STRING && b.type == PS_STRING) {
        size_t len = strlen(a.as.s) + strlen(b.as.s) + 1;
        char *buf = (char *)malloc(len);
        if (!buf) { fprintf(stderr, "runtime error: out of memory\n"); exit(1); }
        snprintf(buf, len, "%s%s", a.as.s, b.as.s);
        return ps_str(buf); /* intentionally leaked in this scaffold */
    }
    ps_type_error("add", a, b);
    return ps_int(0); /* unreachable */
}

PsValue ps_gt(PsValue a, PsValue b) {
    if (a.type != PS_INT || b.type != PS_INT) ps_type_error("compare", a, b);
    return ps_int(a.as.i > b.as.i);
}

PsValue ps_lt(PsValue a, PsValue b) {
    if (a.type != PS_INT || b.type != PS_INT) ps_type_error("compare", a, b);
    return ps_int(a.as.i < b.as.i);
}

PsValue ps_eq(PsValue a, PsValue b) {
    if (a.type == PS_INT && b.type == PS_INT) return ps_int(a.as.i == b.as.i);
    if (a.type == PS_STRING && b.type == PS_STRING)
        return ps_int(strcmp(a.as.s, b.as.s) == 0);
    ps_type_error("compare", a, b);
    return ps_int(0); /* unreachable */
}

long ps_as_int(PsValue v) {
    if (v.type != PS_INT) {
        fprintf(stderr, "runtime error: expected a number\n");
        exit(1);
    }
    return v.as.i;
}

int ps_truthy(PsValue v) {
    return v.type == PS_INT ? (v.as.i != 0) : (v.as.s[0] != '\0');
}

void ps_say(PsValue v) {
    if (v.type == PS_INT) printf("%ld\n", v.as.i);
    else printf("%s\n", v.as.s);
}
