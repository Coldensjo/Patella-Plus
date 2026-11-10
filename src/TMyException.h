// TMyException: the client's exception type, the only game class with RTTI. The
// original file name is unknown ("TMyException" fits the alphabetical link
// order, which puts this file after Talk and before TRSA, TXTEA and Utils).
//
// A message and the source location of the site that threw it. The original's
// two constructors took the file, the line, the kind, two named values and a
// reason (a text, or a caught TMyException whose message became the reason and
// whose origin was kept); their formatting is ErrorText now, the message's
// text, and the origin ("<file> <line>" of the site that first raised the
// error) went with error.txt's report, its only reader. The sites build the
// exception through common.h's TMYEXCEPTION and TERROR* and
// compat/assertion.h's PATELLA_ASSERT*, which pass their own location.

#pragma once

#include "compat/assertion.h"

#include <string>
#include <string_view>

// A recoverable failure: what a TMyException would carry, returned in a patella::TExpected<T, TError>
// (compat/expected.h) instead of thrown. A function that the original exposes throws it on (TMyException(TError)); its
// callers that recover test the result instead of catching.
struct TError {
	std::string Message;
};

class TMyException {
public:
	explicit TMyException(std::string Message);
	// A returned failure thrown after all: the same message.
	explicit TMyException(TError Error);
	virtual ~TMyException();

	[[nodiscard]] std::string GetMessage() const { return Message; }

private:
	std::string Message;
};

namespace patella {

// The text of the original's constructor: "<file> <line>: <Kind>[ (<Name1> = <Value1>)][ (<Name2> =
// <Value2>)][, reason:\n<Reason>]", the file without its path, each part only when its text is not empty.
std::string ErrorText(std::string_view File, int Line, std::string_view Kind, std::string_view Name1, int Value1,
	std::string_view Name2, int Value2, std::string_view Reason);
// The text of the wrapping constructor: the same, with the caught exception's message as the reason, which
// is always appended (", reason:\n" also before an empty message).
std::string ErrorText(std::string_view File, int Line, std::string_view Kind, std::string_view Name1, int Value1,
	std::string_view Name2, int Value2, const TMyException &Inner);

// TMYEXCEPTION's exception: ErrorText's message with the site's file and line.
TMyException MakeError(const TSourceLocation &Location, std::string_view Kind, std::string_view Name1, int Value1,
	std::string_view Name2, int Value2, std::string_view Reason);
TMyException MakeError(const TSourceLocation &Location, std::string_view Kind, std::string_view Name1, int Value1,
	std::string_view Name2, int Value2, const TMyException &Inner);
// TUNEXPECTED_REASON's failure: MakeError's message as a TError.
TError MakeFailure(const TSourceLocation &Location, std::string_view Kind, std::string_view Name1, int Value1,
	std::string_view Name2, int Value2, std::string_view Reason);

// The original's handlers around InitInstance and Run (Player.cpp, after its PROGRAM_ERROR handler), in one place.
// Called inside a catch; throws the TMyException, for HandleException, that the exception being handled becomes, with
// Location as the site. Its texts are the original's. An out-of-memory error, std::bad_alloc here, says "unknown
// exception occurred": the original's operator new was MFC's (nafxcw.lib), whose handler threw a CMemoryException *,
// which only the original's catch (...) took. Its "bad_alloc_exception occurred" was never that error's.
[[noreturn]] void ThrowTopLevelError(const TSourceLocation &Location);

} // namespace patella
