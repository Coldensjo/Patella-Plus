// common.h: what every file of the client shares: the platform's headers and types, the integer types, the range
// helpers, the PROGRAM_ERROR codes and the error macros.
//
// The error macros make the client's error messages. Their texts (the expression, "assertion failed", "exception
// occurred", "unknown exception occurred") are the original's; the file and line are the site's own.

#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
// PATELLA_CORE: the portable code of the patella_core library (cmake/Core.cmake), built without the Windows headers.
// The rest of the client's code: the Windows headers (WinSock 2 first, it must come before windows.h).
#if !defined(PATELLA_CORE)
#include <winsock2.h>
#include <windows.h>
#elif defined(PATELLA_WIN32_TYPES)
// The Linux client (cmake/LinuxX64.cmake): the Windows types and constants its platform layer still names (POINT, UINT,
// WM_*, VK_*), without any Windows function.
#include "compat/win32_types.h" // IWYU pragma: export
#endif
// <string>, <cstring>, <cstdio> and <cstdlib>: nothing of them is used here, and every file that uses them includes
// them itself. Here they come before compat/msvc.h's C headers, which keeps the order in which every translation unit
// declares their names.
#include <cstdint>
#include <string> // IWYU pragma: keep
#include <type_traits>
#include <cstring> // IWYU pragma: keep
#include <cstdio>  // IWYU pragma: keep
#include <cstdlib> // IWYU pragma: keep
// The Microsoft CRT's and compiler's spellings behind portable names.
#include "compat/msvc.h" // IWYU pragma: export

// The original's unsigned integer types, as aliases of the std types. They are the same types as before, not only the
// same sizes: std::uint8_t, std::uint16_t and std::uint32_t are unsigned char, unsigned short and unsigned int with
// MSVC (x86 and x64), mingw-w64 GCC, and GCC and Clang on Linux, so mangled names, overloads and the PDB's type names
// stay as they were.
using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
static_assert(sizeof(uint8) == 1 && std::is_same_v<uint8, unsigned char>, "uint8 is unsigned char");
static_assert(sizeof(uint16) == 2 && std::is_same_v<uint16, unsigned short>, "uint16 is unsigned short");
static_assert(sizeof(uint32) == 4 && std::is_same_v<uint32, unsigned int>, "uint32 is unsigned int");

// Range check used in many assertions, e.g. "In(CommandType,0,255)".
inline bool In(int Value, int Min, int Max)
{
	return Value >= Min && Value <= Max;
}

// Keeps *Value between Min and Max, in either order. The name is ours.
inline void Limit(int *Value, int Min, int Max)
{
	if (Min > Max) {
		if (*Value < Max)
			*Value = Max;
		if (*Value > Min)
			*Value = Min;
	} else {
		if (*Value < Min)
			*Value = Min;
		if (*Value > Max)
			*Value = Max;
	}
}

// Thrown by value.
#include "TMyException.h" // IWYU pragma: export

// The client's second exception: an error code, PROGRAM_ERROR, that
// CPlayerApp::HandleProgramError shows with sysinc.cpp's texts. The original
// throws the enum by value; here it is an enum class carried by its own
// exception type, TProgramError. The enumerator names are ours, after
// GetErrorText's texts and the sites that throw them.
enum class PROGRAM_ERROR : int {
	OBJECTS_FILE = 1,  // "Cannot find file "Tibia.dat"." (TObjectTypes::Init)
	SPRITES_FILE = 2,  // "Cannot find file "Tibia.spr"." (the sprite offsets)
	PICTURES_FILE = 3, // "Cannot find file "Tibia.pic"."
	SPRITES_DATA = 4,  // "Cannot open file "Tibia.spr"." (a sprite's pixels)
	PICTURES_DATA = 5, // "Cannot open file "Tibia.pic"."
	SOCKET_INIT = 6,   // "Cannot initialize Windows Sockets." (InitNetwork)
	DIRECTX_INIT = 7,  // "Cannot initialize DirectX."
	COLOR_DEPTH = 8,   // "Too low color depth detected."
	CLIPPER = 9,       // "Cannot initialize video graphics mode." (9-11)
	COOPERATIVE_LEVEL = 10,
	DISPLAY_MODE = 11,
	SURFACE = 12, // "Video surface allocation failed." (12-18): the primary surface, and the generic code
	SCREEN_SURFACE = 13,
	TEMP_SURFACE = 14,
	STRETCH_SURFACE = 15,
	CACHE_SURFACE = 16,
	AUTOMAP_SURFACE = 17,
	SPRITE_SURFACE = 18,
};

// A thrown PROGRAM_ERROR. Caught by reference: catch (const TProgramError &Error), Error.Code.
struct TProgramError {
	PROGRAM_ERROR Code;
};

// The assertions: PATELLA_ASSERT and PATELLA_ASSERT_VALUES.
#include "compat/assertion.h" // IWYU pragma: export

// The error sites' exception: a TMyException with ErrorText's message
// ("<file> <line>: <Kind>[ (<V1's text> = <V1>)]...[, reason:\n<Reason>]")
// and the site's own source location, as PATELLA_ASSERT has. TMYEXCEPTION's
// Reason is a text or a caught TMyException, whose message becomes the
// reason (the original's wrapping constructor).
#define TMYEXCEPTION(Kind, N1, V1, N2, V2, Reason) \
	::patella::MakeError(PATELLA_SOURCE_LOCATION(), Kind, N1, (int)(V1), N2, (int)(V2), Reason)

#define TERROR(Message) \
	throw TMYEXCEPTION(Message, "", 0, "", 0, "")
#define TERROR1(Message, V1) \
	throw TMYEXCEPTION(Message, #V1, V1, "", 0, "")
// An error with a text as its reason (Script.cpp: the file name), in the
// place of an assertion's expression: "<Message>, reason:\n<Reason>".
#define TERROR_REASON(Message, Reason) \
	throw TMYEXCEPTION(Message, "", 0, "", 0, Reason)
#define TERROR1_REASON(Message, V1, Reason) \
	throw TMYEXCEPTION(Message, #V1, V1, "", 0, Reason)
// TERROR_REASON's failure returned instead of thrown: a
// patella::TUnexpected<TError> for a function that returns a
// patella::TExpected<T, TError> (compat/expected.h).
#define TUNEXPECTED_REASON(Message, Reason) \
	::patella::TUnexpected<TError>(::patella::MakeFailure(PATELLA_SOURCE_LOCATION(), Message, "", 0, "", 0, Reason))
