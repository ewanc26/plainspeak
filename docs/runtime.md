# Runtime and generated-C reference

PlainSpeak currently emits portable C11 and links against `plainspeak_runtime.h` / `plainspeak_runtime.c`. C99 remains the semantic portability baseline; C11 is the backend dialect because later-standard facilities such as `_Alignof` are intentionally used when the language exposes them.

There are now two deliberately separate generated representations:

1. **Legacy PlainSpeak values** use the tagged `PsValue` runtime.
2. **Explicit native objects** introduced by `Declare` use their real C scalar/pointer/fixed-array/structure/union/enumeration type and therefore have C address, size, alignment, storage, indirection, subscript and member semantics.

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

- Native scalar (including tagged enums), object-pointer, fixed-array, tagged-structure and tagged-union storage are first-class, including native bit-fields, C99 flexible-array tails and recursive const/volatile/restrict/_Atomic qualification. Requested alignment, allocated extended storage and the broader atomic API/memory-order surface remain pending.
- Pointer arithmetic/comparison are native, while pointer/aggregate transport through legacy untyped Procedures remains deliberately rejected; typed Procedures carry native values instead.
- `sizeof`/`_Alignof` results are currently boxed into legacy signed `number`; a native `size_t`-equivalent is pending.
- String-concatenation buffers, list allocations, and iteration snapshots live until process exit; the legacy runtime has no language-level ownership/GC.
- Runtime list storage remains intentionally untyped; homogeneity is a compiler invariant rather than duplicated metadata in the C ABI.

## Native fixed arrays

Fixed native arrays do not add a runtime container ABI. They lower directly to C arrays, and native `Element at` lowers to C subscripting. Ordinary array expressions decay to element pointers in value contexts, while retained semantic types let `Size of arrayName` preserve the full array extent. The C declarator emitter is recursive so array-of-pointer and pointer-to-array types preserve C precedence.


## Typed procedures

Legacy Procedures keep the `PsValue` ABI. Procedures with explicit parameter types and a `returns` clause instead lower to native C function declarations/definitions. Their parameters are emitted with the semantic C types retained by sema, array parameters use C's array-to-pointer adjustment, and typed return values use raw/native lowering. Generated prototypes precede all procedure definitions, so forward and mutually recursive calls are valid generated C.

Typed calls use raw arguments and native return values; when a typed arithmetic result re-enters a legacy PlainSpeak expression it is boxed through the existing numeric bridge. Pointer results stay on the native/raw path. Typed `void` procedures emit C `void` and bare `return;`.


## Native structures

Structure definitions lower directly to C `struct` definitions before native object declarations and procedure prototypes. Semantic analysis retains the ordered field types, enforces completeness for by-value fields and objects, and permits pointers to incomplete/self-referential tags. Tags and field names use the same C-safe identifier mangling as other user names.

`Member ... of ...` lowers to either C `.` or `->` according to the semantic type of its base. Scalar members cross the existing boxing bridge only when used by legacy operations such as `Say`; structure values themselves remain native. Fixed-array members remain arrays, so native `Element at` can operate on them without a runtime container.


## Native unions

Union definitions lower directly to C `union` definitions alongside structures before native object declarations and procedure prototypes. Structure and union tags share C's single tag namespace. Semantic analysis tracks union completeness, ordered members and recursive/forward pointers independently from structures while preserving that namespace collision rule.

Member access uses the same `.` / `->` lowering as structures. No active-member metadata is added to the runtime: union representation and any cross-member interpretation remain properties of generated C and the target implementation.


## Aggregate initialization

Aggregate declaration syntax does not add a runtime container. For file-scope native aggregates, C static storage supplies the initial all-zero state and generated startup code writes the selected initializer entries in source order. For automatic aggregates, codegen emits a native declaration with `= {0}` and then the validated member/element stores.

Positional structure initializers use the semantically retained field order. Positional array initializers use indexes from zero. Named structure/union designators and numeric array designators lower to ordinary native field/element assignments after zero initialization. This design deliberately supports dynamic PlainSpeak expressions while preserving omitted-member zero semantics; it is not a claim that every initializer is a C constant expression.


## Native enumerations

Enumeration definitions lower to real C `enum` definitions before aggregate definitions, objects and function prototypes. Semantic analysis computes implicit values, checks explicit values against the current C99-C17 `int` range, enforces the shared C tag namespace, and retains the completed enumeration for object/layout queries.

PlainSpeak Enumerator expressions are source-qualified, but generated C enumerator identifiers are built from both the enumeration tag and enumerator name. This keeps generated identifiers unique even when separate PlainSpeak enumerations reuse the same member name. Enum objects remain native C enum objects; only the existing boxing bridge is used when an enum value enters a legacy operation such as `Say`.


## Bit-fields and flexible array members

Aggregate semantic metadata retains each member's ordinary type plus optional bit width or flexible-array marker. Code generation emits bit-fields directly as C `type name : width` declarations and flexible tails as incomplete arrays `type name[]`; neither feature is emulated by the runtime.

Unnamed bit-fields have no PlainSpeak member identity and are omitted from positional initializer slots. Named bit-fields use ordinary native member reads/stores, but semantic analysis records bit-field expressions so `Size of` can reject them before C compilation.

A structure with a flexible tail remains a complete structure type for direct objects and `sizeof`, while the flexible member itself is an incomplete array. Sema prevents that structure from being embedded by value or used as a fixed-array element, forbids flexible union members, requires the tail to be last with another named member before it, and excludes it from aggregate initialization.


## Native type qualifiers

Type qualifiers are retained structurally at every recursive type node and emitted directly into C declarators. `constant`, `volatile`, `restricted`, and `atomic` lower to `const`, `volatile`, `restrict`, and `_Atomic`. Pointer qualifiers are emitted after the corresponding `*`, so a const pointer and a pointer to const remain distinct generated C types.

Semantic analysis performs C-style top-level lvalue conversion for reads while preserving nested pointee qualification. It allows pointer assignment to add immediate pointee qualifiers and rejects qualifier-discarding or unsafe nested-pointer conversions. Const modifiability is checked before code generation across direct assignment, compound Add/Subtract, dereference stores, array element stores, and aggregate member stores.

Const/volatile aggregate qualification propagates to member expressions. Atomic scalar objects and pointers use the backend compiler's native C11 atomic load/store behavior for ordinary expressions and assignments; PlainSpeak does not emulate those accesses in its runtime. Atomic structure/union member access is rejected because the C11 semantics make direct member access undefined, and explicit memory-order operations/fences remain future concurrency work.

Top-level native objects are still emitted as file-scope declarations with most PlainSpeak initializers executed later in `main`. Because a const object cannot be assigned after declaration, top-level const runtime initializers are intentionally rejected until constant-expression/file-scope initializer lowering lands. Current aggregate initializers are similarly post-store based and therefore reject const subobjects and atomic aggregate targets.
