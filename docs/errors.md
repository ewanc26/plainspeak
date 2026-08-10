# Diagnostic codes

Every diagnostic emitted by the compiler has a stable `E00xx` code.
Error messages read like the literal-minded listener the language is:
state what was expected, what was found, and where.

## Compile-time errors (sema pass)

| Code | Description |
|------|-------------|
| E0001 | Use of undeclared variable |
| E0002 | Type mismatch in addition (`plus`) |
| E0003 | Type mismatch in addition to variable (`Add` / `Set`) |
| E0004 | Type mismatch in comparison |
| E0005 | Repeat/While count must be a number |
| E0006 | Variable redeclaration in the same scope |
| E0007 | Call to undefined procedure |
| E0008 | Wrong number of arguments to procedure call |

## Parse-time errors

| Code | Description |
|------|-------------|
| E0031 | Unrecognized verb |
