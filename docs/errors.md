# Diagnostic codes

Every diagnostic emitted by the compiler has a stable `E00xx` code. Error messages read like the literal-minded listener the language is: state what was expected, what it found, and where.

## Compile-time errors (sema pass)

| Code | Description |
|---|---|
| E0001 | Use of undeclared variable |
| E0002 | Invalid operand type for arithmetic/logical/length operations |
| E0003 | Type mismatch in mutation of a scalar variable |
| E0004 | Type mismatch in comparison |
| E0005 | Repeat/While/For value has the wrong type |
| E0006 | Variable redeclaration in the same scope |
| E0007 | Call to undefined procedure |
| E0008 | Wrong number of arguments to procedure call |
| E0009 | List element type mismatch or nested list |
| E0010 | List operation applied to a non-list value |
| E0011 | List position is not a whole number |
| E0012 | Size/alignment query applied to a non-object or unsupported-layout type |
| E0013 | Invalid explicit native declaration, initializer, assignment, or store type |
| E0014 | `Address of` applied to a legacy boxed PlainSpeak value rather than a native object |
| E0015 | Dereference/store-through applied to a non-pointer or `void *` without a concrete pointee |
| E0016 | Invalid or still-unsupported native pointer operation |
| E0017 | Invalid fixed native array declaration, subscript, element store, or whole-array operation |
| E0018 | Invalid typed Procedure signature, argument, return, prototype, or return placement |
| E0019 | Invalid structure definition, completeness, member access, or member store |
| E0020 | Invalid union definition, shared tag namespace, completeness, member access, or member store |
| E0021 | Invalid positional or designated aggregate initializer |
| E0022 | Invalid enumeration definition, tag, enumerator, value, or use |
| E0023 | Invalid bit-field or flexible-array-member layout/use |
| E0024 | Invalid native type qualifier, forbidden const mutation, or unsupported qualified-object lowering |
| E0025 | Invalid bitwise or shift operand |
| E0026 | Invalid explicit C conversion/cast |
| E0027 | Invalid prefix/postfix increment or decrement operand |
| E0028 | Invalid C conditional expression condition or branch types |
| E0029 | Invalid Break/Continue placement outside an allowed control-flow context |
| E0030 | Invalid Switch condition, When label, duplicate/default structure, or clause body |
| E0032 | Invalid Go to / Label: undefined target, duplicate label, or cross-function label use |

## Parse-time errors

| Code | Description |
|---|---|
| E0031 | Unrecognised verb |

Parse errors are currently rendered as literal messages rather than being assigned the stable code in the CLI; `E0031` remains reserved for the structured diagnostic pass.
