# Runtime API reference

The generated C program links against `plainspeak_runtime.h` / `plainspeak_runtime.c`.
All built-in verbs emit calls into this runtime; generated code never inlines
logic for anything the runtime provides.

## Value type

```c
typedef enum { PS_INT, PS_STRING } PsType;

typedef struct {
    PsType type;
    union { long i; const char *s; } as;
} PsValue;
```

## Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `ps_int` | `PsValue ps_int(long v)` | Wrap an integer literal. |
| `ps_str` | `PsValue ps_str(const char *v)` | Wrap a string literal. |
| `ps_add` | `PsValue ps_add(PsValue a, PsValue b)` | Integer add or string concatenation. |
| `ps_gt` | `PsValue ps_gt(PsValue a, PsValue b)` | Greater-than comparison. |
| `ps_lt` | `PsValue ps_lt(PsValue a, PsValue b)` | Less-than comparison. |
| `ps_eq` | `PsValue ps_eq(PsValue a, PsValue b)` | Equality comparison. |
| `ps_as_int` | `long ps_as_int(PsValue v)` | Coerce to integer (runtime error if string). |
| `ps_truthy` | `int ps_truthy(PsValue v)` | Non-zero / non-empty is true. |
| `ps_say` | `void ps_say(PsValue v)` | Print a value to stdout. |

## Name mangling

User identifiers are mangled to C identifiers using the rule in
`src/codegen/mangling.cpp`:

- Default: `ps_<name>`
- If that would collide with a C keyword or a runtime symbol,
  the escaped form `_ps_<name>` is used instead.

The full C keyword set covered (C89/C90, C99, C11, C23) is checked in
`mangling.cpp`, so no generated identifier can shadow a keyword or a
runtime entry point.

## Known limitations

- String concatenation results are heap-allocated and never freed.
- No static type checking: type mismatches are caught at runtime by
  `ps_add` / `ps_as_int`.
