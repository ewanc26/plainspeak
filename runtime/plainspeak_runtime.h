#pragma once
#include <stddef.h>
#include <setjmp.h>

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

PsValue ps_list_from(const PsValue *items, size_t count);
PsValue ps_list_copy(PsValue list);
void ps_list_append(PsValue list, PsValue item);
PsValue ps_list_get(PsValue list, PsValue index);
void ps_list_set(PsValue list, PsValue index, PsValue item);
void ps_list_remove(PsValue list, PsValue index);

PsValue ps_add(PsValue a, PsValue b);
PsValue ps_sub(PsValue a, PsValue b);
PsValue ps_mul(PsValue a, PsValue b);
PsValue ps_div(PsValue a, PsValue b);
PsValue ps_mod(PsValue a, PsValue b);

PsValue ps_and(PsValue a, PsValue b);
PsValue ps_or(PsValue a, PsValue b);
PsValue ps_not(PsValue v);
PsValue ps_neg(PsValue v);

long ps_length(PsValue v);
long ps_strlen(PsValue v);
PsValue ps_read(void);
PsValue ps_read_double(void);

PsValue ps_gt(PsValue a, PsValue b);
PsValue ps_lt(PsValue a, PsValue b);
PsValue ps_eq(PsValue a, PsValue b);
PsValue ps_ne(PsValue a, PsValue b);
PsValue ps_ge(PsValue a, PsValue b);
PsValue ps_le(PsValue a, PsValue b);

long ps_as_int(PsValue v);
static inline double ps_as_double(PsValue v) {
    return v.type == PS_DOUBLE ? v.as.d : (double)ps_as_int(v);
}
int ps_truthy(PsValue v);

void ps_say(PsValue v);
void ps_say_many(size_t count, const PsValue *items);

PsValue ps_sin(PsValue v);
PsValue ps_cos(PsValue v);
PsValue ps_tan(PsValue v);
PsValue ps_sqrt(PsValue v);
PsValue ps_log(PsValue v);
PsValue ps_abs(PsValue v);
PsValue ps_floor(PsValue v);
PsValue ps_ceil(PsValue v);
PsValue ps_pow(PsValue a, PsValue b);

/* C11 synchronisation handles. Each create call returns an opaque nonzero
 * handle (0 on failure) that PlainSpeak programs keep in an integer object. */
long ps_mutex_create(void);
int ps_mutex_lock(long handle);
int ps_mutex_unlock(long handle);
int ps_mutex_destroy(long handle);
long ps_cond_create(void);
int ps_cond_wait(long cond, long mutex);
int ps_cond_signal(long cond);
int ps_cond_broadcast(long cond);
int ps_cond_destroy(long cond);

/* <setjmp.h> access through numbered jump points (0..PS_JUMP_POINTS-1).
 * ps_jump_mark(n) is setjmp on point n and must be used directly as the
 * controlling value of a condition or assignment, exactly as C requires;
 * ps_jump(n, value) is longjmp to point n and never returns. */
#define PS_JUMP_POINTS 16
extern jmp_buf ps_jump_table[PS_JUMP_POINTS];
#define ps_jump_mark(n) setjmp(ps_jump_table[(n)])
void ps_jump(int point, int value);

/* More C11 threading helpers: sleeping, recursive/timed mutexes, timed condition
 * waits and call-once flags. Return values follow <threads.h> (thrd_success = 0,
 * thrd_timedout = 3 on common targets; see PS_THRD_* below). */
#define PS_THRD_SUCCESS 0
#define PS_THRD_TIMEDOUT 1
#define PS_THRD_ERROR 2
int ps_sleep_ms(long milliseconds);
long ps_mutex_create_recursive(void);
int ps_mutex_lock_ms(long handle, long milliseconds);
int ps_cond_wait_ms(long cond, long mutex, long milliseconds);
long ps_once_create(void);
int ps_once_run(long handle, void (*routine)(void));
