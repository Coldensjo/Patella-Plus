// EntryWin32.cpp: the Windows client's entry point. It does what MFC's WinMain (AfxWinMain) did for the original and
// the client relies on: the program's critical errors without Windows' own boxes; the command line; InitInstance, and
// Run unless InitInstance returned FALSE (the exit code is then 0, after a start refused with one of InitInstance's
// boxes).

#include "Player.h"
#include "common.h"

int WINAPI WinMain(_In_ HINSTANCE Instance, _In_opt_ HINSTANCE Previous, _In_ LPSTR CommandLine, _In_ int ShowCommand)
{
	(void)Instance;
	(void)Previous;
	(void)ShowCommand;
	// AfxWinInit's.
	SetErrorMode(SetErrorMode(0) | SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
	theApp.CommandLine = CommandLine;
	if (theApp.InitInstance() == FALSE)
		return 0;
	return theApp.Run();
}
