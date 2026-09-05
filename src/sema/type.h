#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// PlainSpeak's semantic type model is deliberately richer than the set of
// types the parser can spell today. The compiler is growing toward the union
// of C99-C23 capabilities, so semantic analysis needs to be able to represent
// C object, pointer, array, function, aggregate and qualified types before the
// corresponding prose syntax lands.
enum class TypeKind {
    Void,
    Boolean,
    Integer,
    Floating,
    String,      // PlainSpeak text value; currently a runtime-managed C string.
    List,        // PlainSpeak extension; homogeneous mutable reference value.
    Pointer,
    Array,
    Function,
    Structure,
    Union,
    Enumeration,
    BitInt,
    Nullptr,
    Complex
};

enum class IntegerRank { Char, Short, Int, Long, LongLong };
enum class CharSignedness { Plain, Signed, Unsigned };
enum class FloatingRank { Float, Double, LongDouble };

struct TypeQualifiers {
    bool isConst = false;
    bool isVolatile = false;
    bool isRestrict = false;
    bool isAtomic = false;

    bool operator==(const TypeQualifiers &) const = default;
};

struct Type {
    TypeKind kind = TypeKind::Integer;
    TypeQualifiers qualifiers{};

    // Scalar metadata. Unused fields remain at their defaults for other kinds.
    IntegerRank integerRank = IntegerRank::Long;
    CharSignedness charSignedness = CharSignedness::Plain;
    FloatingRank floatingRank = FloatingRank::Double;
    bool isUnsigned = false;
    std::size_t bitWidth = 0;

    // Recursive type metadata. Lists, pointers and arrays use elementType;
    // functions use returnType/parameterTypes; aggregates/enums use tag.
    std::shared_ptr<Type> elementType;
    std::optional<std::size_t> arrayBound;
    std::shared_ptr<Type> returnType;
    std::vector<Type> parameterTypes;
    bool variadic = false;
    std::string tag;

    static Type voidType() {
        Type t;
        t.kind = TypeKind::Void;
        return t;
    }

    static Type boolean() {
        Type t;
        t.kind = TypeKind::Boolean;
        return t;
    }

    static Type integer(IntegerRank rank = IntegerRank::Long, bool unsignedValue = false) {
        Type t;
        t.kind = TypeKind::Integer;
        t.integerRank = rank;
        t.isUnsigned = unsignedValue;
        if (rank == IntegerRank::Char) {
            t.charSignedness = unsignedValue ? CharSignedness::Unsigned : CharSignedness::Signed;
        }
        return t;
    }

    // In C, plain char is a distinct type from both signed char and unsigned
    // char even though its range matches one of them on a given target.
    static Type character() {
        Type t;
        t.kind = TypeKind::Integer;
        t.integerRank = IntegerRank::Char;
        t.charSignedness = CharSignedness::Plain;
        t.isUnsigned = false;
        return t;
    }

    // Existing PlainSpeak `number` values are represented by C long today.
    static Type number() { return integer(IntegerRank::Long, false); }

    static Type floating(FloatingRank rank = FloatingRank::Double) {
        Type t;
        t.kind = TypeKind::Floating;
        t.floatingRank = rank;
        return t;
    }

    static Type decimal() { return floating(FloatingRank::Double); }

    static Type complex() { Type t; t.kind = TypeKind::Complex; return t; }

    static Type string() {
        Type t;
        t.kind = TypeKind::String;
        return t;
    }

    static Type listOf(Type element) {
        Type t;
        t.kind = TypeKind::List;
        t.elementType = std::make_shared<Type>(std::move(element));
        return t;
    }

    static Type pointerTo(Type pointee, TypeQualifiers qualifiers = {}) {
        Type t;
        t.kind = TypeKind::Pointer;
        t.qualifiers = qualifiers;
        t.elementType = std::make_shared<Type>(std::move(pointee));
        return t;
    }

    static Type arrayOf(Type element, std::size_t bound) {
        Type t;
        t.kind = TypeKind::Array;
        t.elementType = std::make_shared<Type>(std::move(element));
        t.arrayBound = bound;
        return t;
    }

    static Type incompleteArrayOf(Type element) {
        Type t;
        t.kind = TypeKind::Array;
        t.elementType = std::make_shared<Type>(std::move(element));
        return t;
    }

    static Type function(Type result, std::vector<Type> parameters, bool isVariadic = false) {
        Type t;
        t.kind = TypeKind::Function;
        t.returnType = std::make_shared<Type>(std::move(result));
        t.parameterTypes = std::move(parameters);
        t.variadic = isVariadic;
        return t;
    }

    static Type structure(std::string name) {
        Type t;
        t.kind = TypeKind::Structure;
        t.tag = std::move(name);
        return t;
    }

    static Type unionType(std::string name) {
        Type t;
        t.kind = TypeKind::Union;
        t.tag = std::move(name);
        return t;
    }

    static Type enumeration(std::string name) {
        Type t;
        t.kind = TypeKind::Enumeration;
        t.tag = std::move(name);
        return t;
    }

    static Type bitInt(std::size_t width, bool unsignedValue = false) {
        Type t;
        t.kind = TypeKind::BitInt;
        t.bitWidth = width;
        t.isUnsigned = unsignedValue;
        return t;
    }

    static Type nullptrType() {
        Type t;
        t.kind = TypeKind::Nullptr;
        return t;
    }

    bool isInteger() const { return kind == TypeKind::Integer || kind == TypeKind::BitInt; }
    bool isFloating() const { return kind == TypeKind::Floating; }
    bool isNumeric() const { return isInteger() || isFloating() || kind == TypeKind::Complex; }
    bool isList() const { return kind == TypeKind::List; }
    bool isPointer() const { return kind == TypeKind::Pointer; }
    bool isArray() const { return kind == TypeKind::Array; }
    bool isFunction() const { return kind == TypeKind::Function; }
    bool isAggregate() const { return kind == TypeKind::Structure || kind == TypeKind::Union; }

    bool operator==(const Type &other) const {
        if (kind != other.kind || qualifiers != other.qualifiers ||
            integerRank != other.integerRank || charSignedness != other.charSignedness ||
            floatingRank != other.floatingRank || isUnsigned != other.isUnsigned ||
            bitWidth != other.bitWidth || arrayBound != other.arrayBound ||
            variadic != other.variadic || tag != other.tag ||
            parameterTypes != other.parameterTypes) {
            return false;
        }

        if (static_cast<bool>(elementType) != static_cast<bool>(other.elementType)) return false;
        if (elementType && *elementType != *other.elementType) return false;

        if (static_cast<bool>(returnType) != static_cast<bool>(other.returnType)) return false;
        if (returnType && *returnType != *other.returnType) return false;

        return true;
    }

    bool operator!=(const Type &other) const { return !(*this == other); }
};
