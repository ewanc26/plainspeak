#include "plainspeak_runtime.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void die(const char *message) {
    fprintf(stderr, "runtime error: %s\n", message);
    exit(1);
}

static const char *type_name(PsType t) {
    if (t == PS_INT) return "number";
    if (t == PS_DOUBLE) return "decimal";
    if (t == PS_STRING) return "string";
    return "list";
}

static void type_error(const char *op, PsValue a, PsValue b) {
    fprintf(stderr, "runtime error: cannot %s a %s and a %s\n",
            op, type_name(a.type), type_name(b.type));
    exit(1);
}

PsValue ps_int(long v) { PsValue r; r.type = PS_INT; r.as.i = v; return r; }
PsValue ps_double(double v) { PsValue r; r.type = PS_DOUBLE; r.as.d = v; return r; }
PsValue ps_str(const char *v) { PsValue r; r.type = PS_STRING; r.as.s = v; return r; }

static double as_double(PsValue v) {
    if (v.type == PS_INT) return (double)v.as.i;
    if (v.type == PS_DOUBLE) return v.as.d;
    die("expected a number");
    return 0.0;
}

static PsList *as_list(PsValue v) {
    if (v.type != PS_LIST || !v.as.list) die("expected a list");
    return v.as.list;
}

static size_t list_index(PsValue list, PsValue index) {
    PsList *l = as_list(list);
    if (index.type != PS_INT) die("list position must be a whole number");
    if (index.as.i < 1 || (size_t)index.as.i > l->length) {
        fprintf(stderr, "runtime error: list position %ld is out of range 1..%zu\n",
                index.as.i, l->length);
        exit(1);
    }
    return (size_t)(index.as.i - 1);
}

PsValue ps_list_from(const PsValue *items, size_t count) {
    PsList *list = (PsList *)malloc(sizeof(PsList));
    if (!list) die("out of memory");
    list->length = count;
    list->capacity = count > 4 ? count : 4;
    list->items = (PsValue *)malloc(list->capacity * sizeof(PsValue));
    if (!list->items) die("out of memory");
    if (items && count) memcpy(list->items, items, count * sizeof(PsValue));
    PsValue result;
    result.type = PS_LIST;
    result.as.list = list;
    return result;
}

PsValue ps_list_copy(PsValue list) {
    PsList *l = as_list(list);
    return ps_list_from(l->items, l->length);
}

void ps_list_append(PsValue list, PsValue item) {
    PsList *l = as_list(list);
    if (l->length == l->capacity) {
        size_t capacity = l->capacity ? l->capacity * 2 : 4;
        PsValue *items = (PsValue *)realloc(l->items, capacity * sizeof(PsValue));
        if (!items) die("out of memory");
        l->items = items;
        l->capacity = capacity;
    }
    l->items[l->length++] = item;
}

PsValue ps_list_get(PsValue list, PsValue index) {
    PsList *l = as_list(list);
    return l->items[list_index(list, index)];
}

void ps_list_set(PsValue list, PsValue index, PsValue item) {
    PsList *l = as_list(list);
    l->items[list_index(list, index)] = item;
}

void ps_list_remove(PsValue list, PsValue index) {
    PsList *l = as_list(list);
    size_t i = list_index(list, index);
    if (i + 1 < l->length) {
        memmove(&l->items[i], &l->items[i + 1], (l->length - i - 1) * sizeof(PsValue));
    }
    l->length--;
}

PsValue ps_add(PsValue a, PsValue b) {
    if (a.type == PS_INT && b.type == PS_INT) return ps_int(a.as.i + b.as.i);
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE))
        return ps_double(as_double(a) + as_double(b));
    if (a.type == PS_STRING || b.type == PS_STRING) {
        char left_num[64], right_num[64];
        const char *left = NULL, *right = NULL;
        if (a.type == PS_STRING) left = a.as.s;
        else if (a.type == PS_INT) { snprintf(left_num, sizeof(left_num), "%ld", a.as.i); left = left_num; }
        else if (a.type == PS_DOUBLE) { snprintf(left_num, sizeof(left_num), "%g", a.as.d); left = left_num; }
        if (b.type == PS_STRING) right = b.as.s;
        else if (b.type == PS_INT) { snprintf(right_num, sizeof(right_num), "%ld", b.as.i); right = right_num; }
        else if (b.type == PS_DOUBLE) { snprintf(right_num, sizeof(right_num), "%g", b.as.d); right = right_num; }
        if (left && right) {
            size_t n = strlen(left) + strlen(right) + 1;
            char *text = (char *)malloc(n);
            if (!text) die("out of memory");
            snprintf(text, n, "%s%s", left, right);
            return ps_str(text);
        }
    }
    type_error("add", a, b);
    return ps_int(0);
}

PsValue ps_sub(PsValue a, PsValue b) {
    if (a.type == PS_INT && b.type == PS_INT) return ps_int(a.as.i - b.as.i);
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE))
        return ps_double(as_double(a) - as_double(b));
    type_error("subtract", a, b); return ps_int(0);
}

PsValue ps_mul(PsValue a, PsValue b) {
    if (a.type == PS_INT && b.type == PS_INT) return ps_int(a.as.i * b.as.i);
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE))
        return ps_double(as_double(a) * as_double(b));
    type_error("multiply", a, b); return ps_int(0);
}

PsValue ps_div(PsValue a, PsValue b) {
    if (a.type == PS_INT && b.type == PS_INT) {
        if (b.as.i == 0) die("division by zero");
        return ps_int(a.as.i / b.as.i);
    }
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) {
        double d = as_double(b);
        if (d == 0.0) die("division by zero");
        return ps_double(as_double(a) / d);
    }
    type_error("divide", a, b); return ps_int(0);
}

PsValue ps_mod(PsValue a, PsValue b) {
    if (a.type == PS_INT && b.type == PS_INT) {
        if (b.as.i == 0) die("modulo by zero");
        return ps_int(a.as.i % b.as.i);
    }
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) {
        double d = as_double(b);
        if (d == 0.0) die("modulo by zero");
        return ps_double(fmod(as_double(a), d));
    }
    type_error("modulo", a, b); return ps_int(0);
}

PsValue ps_and(PsValue a, PsValue b) {
    if (a.type != PS_INT || b.type != PS_INT) type_error("and", a, b);
    return ps_int(ps_truthy(a) && ps_truthy(b));
}
PsValue ps_or(PsValue a, PsValue b) {
    if (a.type != PS_INT || b.type != PS_INT) type_error("or", a, b);
    return ps_int(ps_truthy(a) || ps_truthy(b));
}
PsValue ps_not(PsValue v) { if (v.type != PS_INT) die("expected a number for not"); return ps_int(!ps_truthy(v)); }
PsValue ps_neg(PsValue v) {
    if (v.type == PS_INT) return ps_int(-v.as.i);
    if (v.type == PS_DOUBLE) return ps_double(-v.as.d);
    die("cannot negate a non-number"); return ps_int(0);
}

long ps_strlen(PsValue v) { if (v.type != PS_STRING) die("expected a string for length"); return (long)strlen(v.as.s); }
long ps_length(PsValue v) {
    if (v.type == PS_STRING) return (long)strlen(v.as.s);
    if (v.type == PS_LIST) return (long)as_list(v)->length;
    die("expected a string or list for length"); return 0;
}

PsValue ps_read(void) { long v; if (scanf("%ld", &v) != 1) die("failed to read a number"); return ps_int(v); }
PsValue ps_read_double(void) { double v; if (scanf("%lf", &v) != 1) die("failed to read a number"); return ps_double(v); }

PsValue ps_gt(PsValue a, PsValue b) {
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) return ps_int(as_double(a) > as_double(b));
    type_error("compare", a, b); return ps_int(0);
}
PsValue ps_lt(PsValue a, PsValue b) {
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) return ps_int(as_double(a) < as_double(b));
    type_error("compare", a, b); return ps_int(0);
}
PsValue ps_ge(PsValue a, PsValue b) {
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) return ps_int(as_double(a) >= as_double(b));
    type_error("compare", a, b); return ps_int(0);
}
PsValue ps_le(PsValue a, PsValue b) {
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) return ps_int(as_double(a) <= as_double(b));
    type_error("compare", a, b); return ps_int(0);
}
PsValue ps_eq(PsValue a, PsValue b) {
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) return ps_int(as_double(a) == as_double(b));
    if (a.type == PS_STRING && b.type == PS_STRING) return ps_int(strcmp(a.as.s, b.as.s) == 0);
    type_error("compare", a, b); return ps_int(0);
}
PsValue ps_ne(PsValue a, PsValue b) {
    PsValue equal = ps_eq(a, b);
    return ps_int(!equal.as.i);
}

long ps_as_int(PsValue v) {
    if (v.type == PS_INT) return v.as.i;
    if (v.type == PS_DOUBLE) return (long)v.as.d;
    die("expected a number"); return 0;
}

int ps_truthy(PsValue v) {
    if (v.type == PS_INT) return v.as.i != 0;
    if (v.type == PS_DOUBLE) return v.as.d != 0.0;
    if (v.type == PS_STRING) return v.as.s[0] != '\0';
    return as_list(v)->length != 0;
}

static void print_inline(PsValue v) {
    if (v.type == PS_INT) printf("%ld", v.as.i);
    else if (v.type == PS_DOUBLE) printf("%g", v.as.d);
    else if (v.type == PS_STRING) printf("%s", v.as.s);
    else {
        PsList *l = as_list(v);
        printf("[");
        for (size_t i = 0; i < l->length; ++i) {
            if (i) printf(", ");
            print_inline(l->items[i]);
        }
        printf("]");
    }
}
void ps_say(PsValue v) { print_inline(v); printf("\n"); }
void ps_say_many(size_t count, const PsValue *items) {
    for (size_t i = 0; i < count; ++i) {
        if (i) printf(" ");
        print_inline(items[i]);
    }
    printf("\n");
}

static PsValue unary_math(PsValue v, double (*fn)(double)) {
    if (v.type != PS_INT && v.type != PS_DOUBLE) die("expected a number for a math function");
    return ps_double(fn(as_double(v)));
}
PsValue ps_sin(PsValue v) { return unary_math(v, sin); }
PsValue ps_cos(PsValue v) { return unary_math(v, cos); }
PsValue ps_tan(PsValue v) { return unary_math(v, tan); }
PsValue ps_sqrt(PsValue v) { return unary_math(v, sqrt); }
PsValue ps_log(PsValue v) { return unary_math(v, log); }
PsValue ps_abs(PsValue v) {
    if (v.type == PS_INT) return ps_int(labs(v.as.i));
    if (v.type == PS_DOUBLE) return ps_double(fabs(v.as.d));
    die("expected a number for absolute value");
    return ps_int(0);
}
PsValue ps_floor(PsValue v) {
    if (v.type == PS_INT) return ps_int(v.as.i);
    if (v.type == PS_DOUBLE) return ps_double(floor(v.as.d));
    die("expected a number for floor");
    return ps_int(0);
}
PsValue ps_ceil(PsValue v) {
    if (v.type == PS_INT) return ps_int(v.as.i);
    if (v.type == PS_DOUBLE) return ps_double(ceil(v.as.d));
    die("expected a number for ceiling");
    return ps_int(0);
}
PsValue ps_pow(PsValue a, PsValue b) {
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) return ps_double(pow(as_double(a), as_double(b)));
    die("cannot compute power of a non-number"); return ps_int(0);
}
