#include "plainspeak_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

PsValue ps_int(long v) {
    PsValue r;
    r.type = PS_INT;
    r.as.i = v;
    return r;
}

PsValue ps_double(double v) {
    PsValue r;
    r.type = PS_DOUBLE;
    r.as.d = v;
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
            a.type == PS_INT ? "number" : (a.type == PS_DOUBLE ? "decimal" : "string"),
            b.type == PS_INT ? "number" : (b.type == PS_DOUBLE ? "decimal" : "string"));
    exit(1);
}

static double ps_as_double(PsValue v) {
    if (v.type == PS_INT) return (double)v.as.i;
    if (v.type == PS_DOUBLE) return v.as.d;
    fprintf(stderr, "runtime error: expected a number\n");
    exit(1);
}

PsValue ps_add(PsValue a, PsValue b) {
    if (a.type == PS_INT && b.type == PS_INT) {
        return ps_int(a.as.i + b.as.i);
    }
    if (a.type == PS_DOUBLE && b.type == PS_DOUBLE) {
        return ps_double(a.as.d + b.as.d);
    }
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) {
        return ps_double(ps_as_double(a) + ps_as_double(b));
    }
    if (a.type == PS_STRING && b.type == PS_STRING) {
        size_t len = strlen(a.as.s) + strlen(b.as.s) + 1;
        char *buf = (char *)malloc(len);
        if (!buf) { fprintf(stderr, "runtime error: out of memory\n"); exit(1); }
        snprintf(buf, len, "%s%s", a.as.s, b.as.s);
        return ps_str(buf); /* intentionally leaked in this scaffold */
    }
    if (a.type == PS_STRING && (b.type == PS_INT || b.type == PS_DOUBLE)) {
        char buf[64];
        if (b.type == PS_INT) snprintf(buf, sizeof(buf), "%ld", b.as.i);
        else snprintf(buf, sizeof(buf), "%g", b.as.d);
        size_t len = strlen(a.as.s) + strlen(buf) + 1;
        char *result = (char *)malloc(len);
        if (!result) { fprintf(stderr, "runtime error: out of memory\n"); exit(1); }
        snprintf(result, len, "%s%s", a.as.s, buf);
        return ps_str(result);
    }
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && b.type == PS_STRING) {
        char buf[64];
        if (a.type == PS_INT) snprintf(buf, sizeof(buf), "%ld", a.as.i);
        else snprintf(buf, sizeof(buf), "%g", a.as.d);
        size_t len = strlen(buf) + strlen(b.as.s) + 1;
        char *result = (char *)malloc(len);
        if (!result) { fprintf(stderr, "runtime error: out of memory\n"); exit(1); }
        snprintf(result, len, "%s%s", buf, b.as.s);
        return ps_str(result);
    }
    ps_type_error("add", a, b);
    return ps_int(0); /* unreachable */
}

PsValue ps_sub(PsValue a, PsValue b) {
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) {
        return ps_double(ps_as_double(a) - ps_as_double(b));
    }
    ps_type_error("subtract", a, b);
    return ps_int(0);
}

PsValue ps_mul(PsValue a, PsValue b) {
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) {
        return ps_double(ps_as_double(a) * ps_as_double(b));
    }
    ps_type_error("multiply", a, b);
    return ps_int(0);
}

PsValue ps_div(PsValue a, PsValue b) {
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) {
        double db = ps_as_double(b);
        if (db == 0.0) {
            fprintf(stderr, "runtime error: division by zero\n");
            exit(1);
        }
        return ps_double(ps_as_double(a) / db);
    }
    ps_type_error("divide", a, b);
    return ps_int(0);
}

PsValue ps_mod(PsValue a, PsValue b) {
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) {
        double db = ps_as_double(b);
        if (db == 0.0) {
            fprintf(stderr, "runtime error: modulo by zero\n");
            exit(1);
        }
        return ps_double(fmod(ps_as_double(a), db));
    }
    ps_type_error("modulo", a, b);
    return ps_int(0);
}

PsValue ps_and(PsValue a, PsValue b) {
    if (a.type != PS_INT || b.type != PS_INT) ps_type_error("and", a, b);
    return ps_int(ps_truthy(a) && ps_truthy(b));
}

PsValue ps_or(PsValue a, PsValue b) {
    if (a.type != PS_INT || b.type != PS_INT) ps_type_error("or", a, b);
    return ps_int(ps_truthy(a) || ps_truthy(b));
}

PsValue ps_not(PsValue v) {
    if (v.type != PS_INT) {
        fprintf(stderr, "runtime error: expected a number for not\n");
        exit(1);
    }
    return ps_int(!ps_truthy(v));
}

PsValue ps_neg(PsValue v) {
    if (v.type == PS_INT) return ps_int(-v.as.i);
    if (v.type == PS_DOUBLE) return ps_double(-v.as.d);
    fprintf(stderr, "runtime error: cannot negate a non-number\n");
    exit(1);
}

long ps_strlen(PsValue v) {
    if (v.type != PS_STRING) {
        fprintf(stderr, "runtime error: expected a string for length\n");
        exit(1);
    }
    return strlen(v.as.s);
}

PsValue ps_read(void) {
    long val;
    if (scanf("%ld", &val) != 1) {
        fprintf(stderr, "runtime error: failed to read a number\n");
        exit(1);
    }
    return ps_int(val);
}

PsValue ps_read_double(void) {
    double val;
    if (scanf("%lf", &val) != 1) {
        fprintf(stderr, "runtime error: failed to read a number\n");
        exit(1);
    }
    return ps_double(val);
}

PsValue ps_gt(PsValue a, PsValue b) {
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) {
        return ps_int(ps_as_double(a) > ps_as_double(b));
    }
    ps_type_error("compare", a, b);
    return ps_int(0);
}

PsValue ps_lt(PsValue a, PsValue b) {
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) {
        return ps_int(ps_as_double(a) < ps_as_double(b));
    }
    ps_type_error("compare", a, b);
    return ps_int(0);
}

PsValue ps_eq(PsValue a, PsValue b) {
    if (a.type == PS_INT && b.type == PS_INT) return ps_int(a.as.i == b.as.i);
    if (a.type == PS_DOUBLE && b.type == PS_DOUBLE) return ps_int(a.as.d == b.as.d);
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) {
        return ps_int(ps_as_double(a) == ps_as_double(b));
    }
    if (a.type == PS_STRING && b.type == PS_STRING)
        return ps_int(strcmp(a.as.s, b.as.s) == 0);
    ps_type_error("compare", a, b);
    return ps_int(0); /* unreachable */
}

PsValue ps_ne(PsValue a, PsValue b) {
    if (a.type == PS_INT && b.type == PS_INT) return ps_int(a.as.i != b.as.i);
    if (a.type == PS_DOUBLE && b.type == PS_DOUBLE) return ps_int(a.as.d != b.as.d);
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) {
        return ps_int(ps_as_double(a) != ps_as_double(b));
    }
    if (a.type == PS_STRING && b.type == PS_STRING)
        return ps_int(strcmp(a.as.s, b.as.s) != 0);
    ps_type_error("compare", a, b);
    return ps_int(0); /* unreachable */
}

PsValue ps_ge(PsValue a, PsValue b) {
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) {
        return ps_int(ps_as_double(a) >= ps_as_double(b));
    }
    ps_type_error("compare", a, b);
    return ps_int(0);
}

PsValue ps_le(PsValue a, PsValue b) {
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) {
        return ps_int(ps_as_double(a) <= ps_as_double(b));
    }
    ps_type_error("compare", a, b);
    return ps_int(0);
}

long ps_as_int(PsValue v) {
    if (v.type == PS_INT) return v.as.i;
    if (v.type == PS_DOUBLE) return (long)v.as.d;
    fprintf(stderr, "runtime error: expected a number\n");
    exit(1);
}

int ps_truthy(PsValue v) {
    if (v.type == PS_INT) return (v.as.i != 0);
    if (v.type == PS_DOUBLE) return (v.as.d != 0.0);
    return (v.as.s[0] != '\0');
}

void ps_say(PsValue v) {
    if (v.type == PS_INT) printf("%ld\n", v.as.i);
    else if (v.type == PS_DOUBLE) printf("%g\n", v.as.d);
    else printf("%s\n", v.as.s);
}

PsValue ps_sin(PsValue v) {
    if (v.type == PS_INT) return ps_double(sin((double)v.as.i));
    if (v.type == PS_DOUBLE) return ps_double(sin(v.as.d));
    fprintf(stderr, "runtime error: expected a number for sine\n");
    exit(1);
}

PsValue ps_cos(PsValue v) {
    if (v.type == PS_INT) return ps_double(cos((double)v.as.i));
    if (v.type == PS_DOUBLE) return ps_double(cos(v.as.d));
    fprintf(stderr, "runtime error: expected a number for cosine\n");
    exit(1);
}

PsValue ps_tan(PsValue v) {
    if (v.type == PS_INT) return ps_double(tan((double)v.as.i));
    if (v.type == PS_DOUBLE) return ps_double(tan(v.as.d));
    fprintf(stderr, "runtime error: expected a number for tangent\n");
    exit(1);
}

PsValue ps_sqrt(PsValue v) {
    if (v.type == PS_INT) return ps_double(sqrt((double)v.as.i));
    if (v.type == PS_DOUBLE) return ps_double(sqrt(v.as.d));
    fprintf(stderr, "runtime error: expected a number for square root\n");
    exit(1);
}

PsValue ps_log(PsValue v) {
    if (v.type == PS_INT) return ps_double(log((double)v.as.i));
    if (v.type == PS_DOUBLE) return ps_double(log(v.as.d));
    fprintf(stderr, "runtime error: expected a number for logarithm\n");
    exit(1);
}

PsValue ps_abs(PsValue v) {
    if (v.type == PS_INT) return ps_int(labs(v.as.i));
    if (v.type == PS_DOUBLE) return ps_double(fabs(v.as.d));
    fprintf(stderr, "runtime error: expected a number for absolute value\n");
    exit(1);
}

PsValue ps_floor(PsValue v) {
    if (v.type == PS_INT) return ps_int(v.as.i);
    if (v.type == PS_DOUBLE) return ps_double(floor(v.as.d));
    fprintf(stderr, "runtime error: expected a number for floor\n");
    exit(1);
}

PsValue ps_ceil(PsValue v) {
    if (v.type == PS_INT) return ps_int(v.as.i);
    if (v.type == PS_DOUBLE) return ps_double(ceil(v.as.d));
    fprintf(stderr, "runtime error: expected a number for ceiling\n");
    exit(1);
}

PsValue ps_pow(PsValue a, PsValue b) {
    if ((a.type == PS_INT || a.type == PS_DOUBLE) && (b.type == PS_INT || b.type == PS_DOUBLE)) {
        return ps_double(pow(ps_as_double(a), ps_as_double(b)));
    }
    fprintf(stderr, "runtime error: cannot compute power of a non-number\n");
    exit(1);
}
