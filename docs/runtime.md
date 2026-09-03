# Runtime API reference

Generated C99 links against `plainspeak_runtime.h` / `plainspeak_runtime.c`. Built-in language operations emit calls into this runtime rather than open-coding their behaviour.

## Value type

`PsValue` is a tagged scalar-or-list value:

```c
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
```

Lists own a growable C array of `PsValue` entries. The compiler's semantic pass enforces homogeneous element types; the runtime supplies bounds checking and mutation. List variables are reference values, so copying a `PsValue` that contains a list aliases the same collection.

## Core functions

| Function | Description |
|---|---|
| `ps_int`, `ps_double`, `ps_str` | Wrap scalar literals. |
| `ps_add`, `ps_sub`, `ps_mul`, `ps_div`, `ps_mod` | Scalar arithmetic and supported string concatenation. |
| `ps_length` | Return the length of a string or list. |
| `ps_as_int` | Coerce a numeric value to `long`. |
| `ps_truthy` | Test scalar truthiness or whether a list is non-empty. |
| `ps_say` | Print a scalar or list followed by a newline. |

## List functions

| Function | Signature | Description |
|---|---|---|
| `ps_list_from` | `PsValue ps_list_from(const PsValue *items, size_t count)` | Allocate a mutable list and copy the supplied values. |
| `ps_list_append` | `void ps_list_append(PsValue list, PsValue item)` | Append an item, growing capacity when needed. |
| `ps_list_get` | `PsValue ps_list_get(PsValue list, PsValue index)` | Read a one-based position with bounds checking. |
| `ps_list_set` | `void ps_list_set(PsValue list, PsValue index, PsValue item)` | Replace a one-based position. |
| `ps_list_remove` | `void ps_list_remove(PsValue list, PsValue index)` | Remove a one-based position and compact the tail. |

The runtime rejects non-list operands and out-of-range positions even though valid generated programs should have had their static type errors caught earlier.

## Name mangling

User identifiers are mangled in `src/codegen/mangling.cpp`:

- Default: `ps_<name>`
- If that would collide with a C keyword or runtime symbol, the escaped form `_ps_<name>` is used instead.

The C keyword set covered by `mangling.cpp` includes C89/C90, C99, C11, and C23 spellings.

## Memory model and known limitations

- String-concatenation buffers and list allocations live until process exit; v1 has no language-level ownership or garbage collection.
- Lists cannot contain lists, as enforced by semantic analysis.
- Runtime list storage is intentionally untyped; homogeneity is a compiler invariant rather than duplicated metadata in the C ABI.
