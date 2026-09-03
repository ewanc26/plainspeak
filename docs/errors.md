# Diagnostic codes

Every diagnostic emitted by the compiler has a stable `E00xx` code. Error messages read like the literal-minded listener the language is: state what was expected, what was found, and where.

## Compile-time errors (sema pass)

| Code | Description |
|---|---|
| E0001 | Use of undeclared variable |
| E0002 | Invalid operand type for arithmetic/logical/length operations |
| E0003 | Type mismatch in mutation of a scalar variable |
| E0004 | Type mismatch in comparison |
| E0005 | Repeat/While value has the wrong type |
| E0006 | Variable redeclaration in the same scope |
| E0007 | Call to undefined procedure |
| E0008 | Wrong number of arguments to procedure call |
| E0009 | List element type mismatch or nested list |
| E0010 | List operation applied to a non-list value |
| E0011 | List position is not a whole number |

## Parse-time errors

| Code | Description |
|---|---|
| E0031 | Unrecognised verb |

Parse errors are currently rendered as literal messages rather than being assigned the stable code in the CLI; `E0031` remains reserved for the structured diagnostic pass.
