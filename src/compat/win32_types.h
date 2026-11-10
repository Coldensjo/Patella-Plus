// compat/win32_types.h: the types and constants of the Windows API that the client's own code names, for where there is
// no <windows.h> (Patella-Modern issue #91: the golden runner's stand-ins, tests/golden/linux/windows.h, which includes
// this file; issue #131: the Linux client's platform layer, PATELLA_WIN32_TYPES of common.h). Types and constants only: no
// function is declared here, so a Windows call that was missed fails the Linux client's build instead of linking to
// nothing. The types have the Windows x64 sizes (LONG and DWORD 4 bytes, where Linux's long has 8; the handles and the
// *_PTR types 8), the constants the Windows SDK's values.

#pragma once

#if defined(_WIN32)
#error "compat/win32_types.h is for systems without the Windows headers"
#endif

#include <cstdint>

// The compiler's spelling of the Windows headers (the stand-in windows.h of the golden runner defines it too).
#ifndef CALLBACK
#define CALLBACK
#endif

// --- The types (Windows x64 sizes) ---

using BOOL = int;
using DWORD = std::uint32_t;
using LONG = std::int32_t;
using UINT = unsigned int;
using INT_PTR = std::intptr_t;
using UINT_PTR = std::uintptr_t;
using LONG_PTR = std::intptr_t;
using ULONG_PTR = std::uintptr_t;
using DWORD_PTR = ULONG_PTR;
using WPARAM = UINT_PTR;
using LPARAM = LONG_PTR;
using LRESULT = LONG_PTR;

// NOLINTBEGIN(bugprone-macro-parentheses): Name is a declared name and a type here, which parentheses cannot enclose.
#define DECLARE_HANDLE(Name) \
	struct Name##__ { \
		int unused; \
	}; \
	using Name = Name##__ *
// NOLINTEND(bugprone-macro-parentheses)
DECLARE_HANDLE(HWND);
DECLARE_HANDLE(HDC);

#define TRUE 1
#define FALSE 0

struct tagPOINT {
	LONG x;
	LONG y;
};
using POINT = tagPOINT;

// --- The constants (the Windows SDK's values) ---

// Extended window styles.
#define WS_EX_CLIENTEDGE 0x00000200L
#define WS_EX_APPWINDOW 0x00040000L
