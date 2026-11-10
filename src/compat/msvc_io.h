// msvc_io.h: the descriptor function of the Microsoft CRT's <io.h> that the client uses, behind a portable name
// (_fileno, a <cstdio> function, is types/File.h's TFile::Descriptor). On Windows the Microsoft function itself,
// elsewhere the same semantics from POSIX.

#pragma once

// Only <io.h> on Windows: the files that include this header included it at the same place before (MFC's
// headers come after it there).
#if defined(_WIN32)
#include <io.h>
#else
#include <cstdio>
#include <stdio.h> // NOLINT(modernize-deprecated-headers): off_t, which POSIX declares there, not <cstdio>
#include <unistd.h>
#endif

#if defined(_WIN32)

// On Windows the Microsoft functions' own names (compat/msvc.h says why).
#define MsvcEof _eof

#else

// _eof: 1 when the descriptor's own position (not the FILE's, whose buffer may have read ahead) is the file's
// end, 0 when it is not (the position is left where it was), -1 on an error. (UCRT lowio/eof.cpp: the position
// compared with lseek(SEEK_END), equal only.)
inline int MsvcEof(int Descriptor)
{
	const off_t Here = lseek(Descriptor, 0, SEEK_CUR);
	if (Here == -1)
		return -1;
	const off_t End = lseek(Descriptor, 0, SEEK_END);
	if (End == -1)
		return -1;
	if (Here == End)
		return 1;
	lseek(Descriptor, Here, SEEK_SET);
	return 0;
}

#endif
