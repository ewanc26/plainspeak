#pragma once
/* plainspeak runtime: the tagged-value type and built-in verbs that
 * generated C code calls into. Kept deliberately small — see
 * docs/runtime.md for the API contract generated code relies on. */

typedef enum { PS_INT, PS_STRING } PsType;

typedef struct {
    PsType type;
    union {
        long i;
        const char *s; /* owned by the runtime for literals; leaked for
                           concatenation results in this scaffold — see
                           docs/runtime.md "Known limitations". */
    } as;
} PsValue;

PsValue ps_int(long v);
PsValue ps_str(const char *v);

/* "Add X to Y" / "X plus Y" */
PsValue ps_add(PsValue a, PsValue b);
PsValue ps_sub(PsValue a, PsValue b);
PsValue ps_mul(PsValue a, PsValue b);
PsValue ps_div(PsValue a, PsValue b);

/* logical operators */
PsValue ps_and(PsValue a, PsValue b);
PsValue ps_or(PsValue a, PsValue b);
PsValue ps_not(PsValue v);

PsValue ps_read(void);

/* comparisons: result is PS_INT holding 0 or 1 */
PsValue ps_gt(PsValue a, PsValue b);
PsValue ps_lt(PsValue a, PsValue b);
PsValue ps_eq(PsValue a, PsValue b);

long ps_as_int(PsValue v);
int  ps_truthy(PsValue v);

void ps_say(PsValue v);
