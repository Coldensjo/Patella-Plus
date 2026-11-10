// Player.h: the application class (c:\mud\tibiaplayer, so Player.cpp).
//
// The class name CPlayerApp is the original's, MFC's wizard naming for the
// project "tibiaplayer"; the member names are ours. It derives from nothing:
// the process's entry is WinMain (platform/EntryWin32.cpp), which does what
// MFC's did and calls InitInstance and Run.

#pragma once

#include "common.h"

class CPlayerApp final {
public:
	CPlayerApp();
	BOOL InitInstance();
	int Run();
	void OnIdle();

	void FatalError(const char *Text, const char *Caption);
	void HandleProgramError(PROGRAM_ERROR Error);
	void HandleException(TMyException &Exception);

	bool Terminating;             // a fatal error is being shown; later errors return at once
	const char *CommandLine = ""; // WinMain's
};


extern CPlayerApp theApp;

void GetWindowTitle(char *Text);     // "Tibia   "
void GetWindowClassName(char *Text); // "TibiaClient"
