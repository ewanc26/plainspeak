#include <catch2/catch_test_macros.hpp>
#include "../../src/sema/type.h"

TEST_CASE("legacy PlainSpeak types have stable structural representations", "[types]") {
    CHECK(Type::number() == Type::integer(IntegerRank::Long, false));
    CHECK(Type::decimal() == Type::floating(FloatingRank::Double));
    CHECK(Type::string() == Type::string());

    Type numbers = Type::listOf(Type::number());
    REQUIRE(numbers.isList());
    REQUIRE(numbers.elementType);
    CHECK(*numbers.elementType == Type::number());
}

TEST_CASE("integer rank and signedness are part of type identity", "[types][c99]") {
    CHECK(Type::integer(IntegerRank::Int, false) != Type::integer(IntegerRank::Long, false));
    CHECK(Type::integer(IntegerRank::Long, false) != Type::integer(IntegerRank::Long, true));
    CHECK(Type::integer(IntegerRank::LongLong, false).isInteger());
}

TEST_CASE("plain char is distinct from signed and unsigned char", "[types][c99]") {
    Type plain = Type::character();
    Type signedChar = Type::integer(IntegerRank::Char, false);
    Type unsignedChar = Type::integer(IntegerRank::Char, true);

    CHECK(plain.charSignedness == CharSignedness::Plain);
    CHECK(signedChar.charSignedness == CharSignedness::Signed);
    CHECK(unsignedChar.charSignedness == CharSignedness::Unsigned);
    CHECK(plain != signedChar);
    CHECK(plain != unsignedChar);
    CHECK(signedChar != unsignedChar);
}

TEST_CASE("qualifiers are represented structurally", "[types][c99][c11]") {
    TypeQualifiers q;
    q.isConst = true;
    q.isVolatile = true;
    q.isRestrict = true;
    q.isAtomic = true;

    Type qualified = Type::pointerTo(Type::integer(IntegerRank::Int), q);
    Type plain = Type::pointerTo(Type::integer(IntegerRank::Int));

    CHECK(qualified != plain);
    CHECK(qualified.qualifiers == q);
}

TEST_CASE("pointer and array types retain their referenced type", "[types][c99]") {
    Type pointer = Type::pointerTo(Type::integer(IntegerRank::Int, true));
    REQUIRE(pointer.isPointer());
    REQUIRE(pointer.elementType);
    CHECK(*pointer.elementType == Type::integer(IntegerRank::Int, true));

    Type array = Type::arrayOf(Type::floating(FloatingRank::Float), 8);
    REQUIRE(array.isArray());
    REQUIRE(array.elementType);
    REQUIRE(array.arrayBound);
    CHECK(*array.arrayBound == 8);
    CHECK(*array.elementType == Type::floating(FloatingRank::Float));

    Type incomplete = Type::incompleteArrayOf(Type::number());
    CHECK_FALSE(incomplete.arrayBound.has_value());
    CHECK(incomplete != Type::arrayOf(Type::number(), 1));
}

TEST_CASE("function types retain return parameters and variadic state", "[types][c99]") {
    Type fn = Type::function(
        Type::integer(IntegerRank::Int),
        {Type::pointerTo(Type::integer(IntegerRank::Char)), Type::decimal()},
        true);

    REQUIRE(fn.isFunction());
    REQUIRE(fn.returnType);
    CHECK(*fn.returnType == Type::integer(IntegerRank::Int));
    REQUIRE(fn.parameterTypes.size() == 2);
    CHECK(fn.parameterTypes[0] == Type::pointerTo(Type::integer(IntegerRank::Char)));
    CHECK(fn.parameterTypes[1] == Type::decimal());
    CHECK(fn.variadic);
}

TEST_CASE("aggregate enum bit-precise and nullptr types are representable", "[types][c11][c23]") {
    CHECK(Type::structure("packet").isAggregate());
    CHECK(Type::unionType("word").isAggregate());
    CHECK(Type::structure("packet") != Type::structure("other"));
    CHECK(Type::enumeration("mode").kind == TypeKind::Enumeration);

    Type bits = Type::bitInt(37, true);
    CHECK(bits.kind == TypeKind::BitInt);
    CHECK(bits.bitWidth == 37);
    CHECK(bits.isUnsigned);
    CHECK(bits.isInteger());

    CHECK(Type::nullptrType().kind == TypeKind::Nullptr);
}
