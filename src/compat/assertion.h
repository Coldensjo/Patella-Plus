// assertion.h: PATELLA_ASSERT, the client's assertions.
//
// - PATELLA_ASSERT(Expression): when the expression is false, throws a TMyException:
//   "<file> <line>: assertion failed, reason:\n<Expression>", with the expression's source text.
// - PATELLA_ASSERT_VALUES(Expression, Value1[, Value2]): the same with one or two values in the message,
//   "<file> <line>: assertion failed (<Value1's text> = <value>)[ (<Value2's text> = <value>)], reason:\n...", each
//   value converted to int (static_cast).
// - PATELLA_DIVISOR(Value): an integer divisor from input; zero throws a TMyException instead of trapping (at the end
//   of this file).
//
// The expression is evaluated exactly once, the values only when it is false (after it, in an unspecified order).
// The file and line are the assertion's own (std::source_location::current()). Each is an expression of type void,
// so it is one statement anywhere a statement fits (an if without braces, an else: no dangling else), and nothing is
// compiled out in any build: assertions on packets and files stay checked.
//
// Not contract_assert, on any compiler: the outcome of a failed assertion must stay a TMyException (the client's
// handlers catch that), and a contract assertion gives it only through a throwing replaceable violation handler, one
// for the whole program (CONTRACT_ASSERT's, compat/cxx26.h, would throw too), whose semantic a compiler flag changes
// (ignore compiles the check out), which may evaluate the predicate more than once, and in which GCC 16.2 cannot tell
// a predicate that threw from a false one.
//
// The failure functions are TMyException.cpp's.

#pragma once

#include <source_location> // IWYU pragma: export

namespace patella {

using TSourceLocation = std::source_location;

#define PATELLA_SOURCE_LOCATION() ::std::source_location::current()

// Throw the assertion's TMyException. Names is the values' source text as the preprocessor stringized it
// ("Position, NumberOfBuddies-1"): it is split at its top-level commas.
[[noreturn]] void AssertionFailed(const char *Expression, const TSourceLocation &Location);
[[noreturn]] void AssertionFailed(const char *Expression, const TSourceLocation &Location, const char *Names, int Value1);
[[noreturn]] void AssertionFailed(const char *Expression, const TSourceLocation &Location, const char *Names, int Value1,
	int Value2);

// PATELLA_ASSERT_VALUES's call: the values converted to int.
template <class... T>
[[noreturn]] void AssertionFailedValues(const char *Expression, const TSourceLocation &Location, const char *Names,
	const T &...Values)
{
	static_assert(sizeof...(T) == 1 || sizeof...(T) == 2, "PATELLA_ASSERT_VALUES carries one or two values");
	AssertionFailed(Expression, Location, Names, static_cast<int>(Values)...);
}

// PATELLA_DIVISOR's failure: the TMyException "<file> <line>: integer division by zero, reason:\n
// <Expression>".
[[noreturn]] void DivisionByZero(const char *Expression, const TSourceLocation &Location);

// PATELLA_DIVISOR's call: the divisor itself, or the failure when it is zero.
template <class T> T CheckedDivisor(T Value, const char *Expression, const TSourceLocation &Location)
{
	if (Value == 0)
		DivisionByZero(Expression, Location);
	return Value;
}

} // namespace patella

#define PATELLA_ASSERT(Expression) \
	(!(Expression) ? ::patella::AssertionFailed(#Expression, PATELLA_SOURCE_LOCATION()) : static_cast<void>(0))

#define PATELLA_ASSERT_VALUES(Expression, ...) \
	(!(Expression) ? ::patella::AssertionFailedValues(#Expression, PATELLA_SOURCE_LOCATION(), #__VA_ARGS__, __VA_ARGS__) \
				   : static_cast<void>(0))

// PATELLA_DIVISOR(Value): an integer divisor that input can make zero (a packet's field, a value of Tibia.dat), as the
// division's (or the remainder's) right operand. It is the value, evaluated once; when it is zero, a TMyException is
// thrown instead of the division: the original (MSVC 7.1, whose catch-all handlers caught structured exceptions)
// trapped there and its catch wrapper turned the trap into a TMyException; the x64 client builds with /EHsc, where
// nothing catches a trap.
#define PATELLA_DIVISOR(Value) (::patella::CheckedDivisor((Value), #Value, PATELLA_SOURCE_LOCATION()))
