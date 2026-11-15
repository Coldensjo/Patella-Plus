// Player.h: the application class. CPlayerApp is the original's name, MFC's wizard naming for the project
// "tibiaplayer". It derives from nothing: the process's entry (platform/EntryWin32.cpp, platform/EntryLinux.cpp) sets
// CommandLine and calls InitInstance and Run.

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

void GetWindowTitle(char *Text);     // BuildConfig.h's WindowTitle ("Tibia   "); 30 bytes
void GetWindowClassName(char *Text); // BuildConfig.h's WindowClassName ("TibiaClient"); 30 bytes
