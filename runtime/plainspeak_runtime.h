#pragma once
#include <stddef.h>

/* plainspeak runtime: the tagged-value type and built-in verbs that
 * generated C code calls into. Kept deliberately small — see
 * docs/runtime.md for the API contract generated code relies on. */

typedef enum { PS_INT, PS_DOUBLE, PS_STRING, PS_LIST } PsType;

typedef struct PsValue PsValue;
typedef struct PsList PsList;

struct PsValue {
    PsType type;
    union {
        long i;
        double d;
        const char *s;
        PsList *list;
    } as;
};

struct PsList {
    PsValue *items;
    size_t length;
    size_t capacity;
};

PsValue ps_int(long v);
PsValue ps_double(double v);
PsValue ps_str(const char *v);

/* Lists are mutable reference values. Source positions are one-based. */
PsValue ps_list_from(const PsValue *items, size_t count);
void ps_list_append(PsValue list, PsValue item);
PsValue ps_list_get(PsValue list, PsValue index);
void ps_list_set(PsValue list, PsValue index, PsValue item);
void ps_list_remove(PsValue list, PsValue index);

/* "Add X to Y" / "X plus Y" */
PsValue ps_add(PsValue a, PsValue b);
PsValue ps_sub(PsValue a, PsValue b);
PsValue ps_mul(PsValue a, PsValue b);
PsValue ps_div(PsValue a, PsValue b);
PsValue ps_mod(PsValue a, PsValue b);

/* logical operators */
PsValue ps_and(PsValue a, PsValue b);
PsValue ps_or(PsValue a, PsValue b);
PsValue ps_not(PsValue v);
PsValue ps_neg(PsValue v);

long ps_length(PsValue v);
long ps_strlen(PsValue v);
PsValue ps_read(void);
PsValue ps_read_double(void);

/* comparisons: result is PS_INT holding 0 or 1 */
PsValue ps_gt(PsValue a, PsValue b);
PsValue ps_lt(PsValue a, PsValue b);
PsValue ps_eq(PsValue a, PsValue b);
PsValue ps_ne(PsValue a, PsValue b);
PsValue ps_ge(PsValue a, PsValue b);
PsValue ps_le(PsValue a, PsValue b);

long ps_as_int(PsValue v);
int ps_truthy(PsValue v);

void ps_say(PsValue v);

/* scientific functions */
PsValue ps_sin(PsValue v);
PsValue ps_cos(PsValue v);
PsValue ps_tan(PsValue v);
PsValue ps_sqrt(PsValue v);
PsValue ps_log(PsValue v);
PsValue ps_abs(PsValue v);
PsValue ps_floor(PsValue v);
PsValue ps_ceil(PsValue v);
PsValue ps_pow(PsValue a, PsValue b);
