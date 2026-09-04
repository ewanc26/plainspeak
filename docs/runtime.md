# Runtime and generated-C reference

PlainSpeak currently emits portable C11 and links against `plainspeak_runtime.h` / `plainspeak_runtime.c`. C99 remains the semantic portability baseline; C11 is the backend dialect because later-standard facilities such as `_Alignof` are intentionally used when the language exposes them.

There are now two deliberately separate generated representations:

1. **Legacy PlainSpeak values** use the tagged `PsValue` runtime.
2. **Explicit native objects** introduced by `Declare` use their real C scalar/pointer/fixed-array type and therefore have C address, size, alignment, storage, indirection and subscript semantics.

The compiler's semantic analysis records which representation every relevant expression/object uses. Code generation consumes that result rather than re-inferring types.

## Legacy value type

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

Lists own a growable C array of `PsValue` entries. Semantic analysis enforces homogeneous element types; the runtime supplies bounds checking and mutation. List variables are reference values. `ps_list_copy` is the deliberate exception used by generated `For each` loops to create an iteration snapshot.

## Native object lowering

For example:

```text
Declare x as integer with value 41. Declare p as pointer to integer with value Address of x. Set value at p to 42.
```

lowers conceptually to real C objects and addresses:

```c
int ps_x;
int *ps_p;

int main(void) {
    ps_x = 41;
    ps_p = &ps_x;
    *ps_p = 42;
}
```

The exact generated expressions may include numeric bridge calls when an initializer originates in the legacy boxed expression system. Direct top-level native objects are file-scope C objects; their PlainSpeak initializers run in `main` in source order. Native declarations inside procedures or blocks are automatic C declarations at the source position.

The language does not expose the address of a boxed `PsValue`; `Address of` is restricted to explicit native objects so C address semantics cannot accidentally refer to a compiler/runtime wrapper.

## Core runtime functions

| Function | Description |
|---|---|
| `ps_int`, `ps_double`, `ps_str` | Wrap legacy scalar values. |
| `ps_add`, `ps_sub`, `ps_mul`, `ps_div`, `ps_mod` | Legacy scalar arithmetic and supported string concatenation. Whole-number inputs preserve `PS_INT`; mixed numeric inputs promote to `PS_DOUBLE`. |
| `ps_length` | Return the length of a string or list. |
| `ps_as_int` | Coerce a boxed numeric value to C `long`; used as one bridge into native integer objects. |
| `ps_as_double` | Coerce a boxed numeric value to C `double`; used as the bridge into native floating objects. |
| `ps_truthy` | Test legacy scalar truthiness or whether a list is non-empty. |
| `ps_say` | Print a legacy scalar or list followed by a newline. Native arithmetic objects are boxed at the call boundary before printing. |

Whole-number division uses C integer division, truncating toward zero, and whole-number modulo uses the corresponding integer remainder. Decimal or mixed numeric operands use floating-point division and `fmod`.

## List functions

| Function | Signature | Description |
|---|---|---|
| `ps_list_from` | `PsValue ps_list_from(const PsValue *items, size_t count)` | Allocate a mutable list and copy the supplied values. |
| `ps_list_copy` | `PsValue ps_list_copy(PsValue list)` | Allocate an independent shallow snapshot of the list's current values. |
| `ps_list_append` | `void ps_list_append(PsValue list, PsValue item)` | Append an item, growing capacity when needed. |
| `ps_list_get` | `PsValue ps_list_get(PsValue list, PsValue index)` | Read a one-based position with bounds checking. |
| `ps_list_set` | `void ps_list_set(PsValue list, PsValue index, PsValue item)` | Replace a one-based position. |
| `ps_list_remove` | `void ps_list_remove(PsValue list, PsValue index)` | Remove a one-based position and compact the tail. |

The snapshot is shallow because nested lists are rejected by semantic analysis and supported list scalars are copied directly.

## Name mangling

User identifiers are mangled in `src/codegen/mangling.cpp`:

- Default: `ps_<name>`
- If that would collide with a C keyword or runtime symbol, the escaped form `_ps_<name>` is used instead.

The keyword set includes C89/C90, C99, C11, and C23 spellings. Runtime bridge names such as `ps_as_int` and `ps_as_double` are reserved as well.

## Current object-model boundaries

- Native scalar and object-pointer storage is first-class, but arrays, aggregates, qualifiers, atomics, requested alignment and allocated storage have not landed yet.
- Pointer arithmetic/comparison and pointer-valued legacy procedure transport are deliberately rejected rather than lowered incorrectly.
- `sizeof`/`_Alignof` results are currently boxed into legacy signed `number`; a native `size_t`-equivalent is pending.
- String-concatenation buffers, list allocations, and iteration snapshots live until process exit; the legacy runtime has no language-level ownership/GC.
- Runtime list storage remains intentionally untyped; homogeneity is a compiler invariant rather than duplicated metadata in the C ABI.

## Native fixed arrays

Fixed native arrays do not add a runtime container ABI. They lower directly to C arrays, and native `Element at` lowers to C subscripting. Ordinary array expressions decay to element pointers in value contexts, while retained semantic types let `Size of arrayName` preserve the full array extent. The C declarator emitter is recursive so array-of-pointer and pointer-to-array types preserve C precedence.
