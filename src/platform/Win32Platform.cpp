// Win32Platform.cpp: the Win32 implementations of the platform's interfaces (Patella-Modern issue #109;
// platform/Win32Platform.h). Not part of the original source: each function is the call the client made itself
// before #109 (sysinc.cpp's timeGetTime, the wall clock's time, the frame timer's Sleep; Network.cpp's CAsyncSocket
// and WinSock calls until issue #125, now platform/PortableSocket.cpp's and, in the golden runner,
// tests/golden/network/MfcSocket.cpp's) or before #119 (sysinc.cpp's clipboard, MainFrm.cpp's and Player.cpp's GetCursorPos, SetTimer,
// KillTimer, MessageBoxA and EnumWindows), with the same arguments.

#include "platform/Win32Platform.h"

#include "common.h"
#include "platform/Platform.h"
#include "types/FixedString.h"

#include <mmsystem.h>

#include "platform/SdlPlatform.h"

#include <cstdint>
#include <ctime>

#if defined(_MSC_VER)
#pragma comment(lib, "winmm.lib")
#endif

namespace patella::platform {

std::uint32_t TWin32Clock::Milliseconds()
{
	return timeGetTime();
}

std::int64_t TWin32Clock::WallSeconds()
{
	return std::time(nullptr);
}

void TWin32Clock::FrameWait(std::uint32_t Duration)
{
	Sleep(Duration);
}

bool TWin32Input::CursorPosition(TPoint &Point)
{
	POINT Cursor;
	if (GetCursorPos(&Cursor) == FALSE)
		return false;
	Point.x = Cursor.x;
	Point.y = Cursor.y;
	return true;
}

void TWin32Input::StartTimer(TWindowHandle Window, unsigned int Id, unsigned int Milliseconds)
{
	::SetTimer(static_cast<HWND>(Window), Id, Milliseconds, nullptr);
}

void TWin32Input::StopTimer(TWindowHandle Window, unsigned int Id)
{
	::KillTimer(static_cast<HWND>(Window), Id);
}

// Win32's GetCursorPos (the owner's decision on #123): where Windows cannot read the cursor (another
// desktop: the lock screen, UAC's secure desktop) it fails and the point stays the last one, as the original's;
// SDL_GetGlobalMouseState would report (0, 0) there.
bool TSdlLoopInput::CursorPosition(TPoint &Point)
{
	return Win32Input().CursorPosition(Point);
}

void TSdlLoopInput::StartTimer([[maybe_unused]] TWindowHandle Window, [[maybe_unused]] unsigned int Id,
	unsigned int Milliseconds)
{
	Timer.Start(Milliseconds, Platform().Clock.Milliseconds());
}

void TSdlLoopInput::StopTimer([[maybe_unused]] TWindowHandle Window, [[maybe_unused]] unsigned int Id)
{
	Timer.Stop();
}

void TWin32Window::ShowMessageBox(TWindowHandle Owner, const char *Text, const char *Caption, TMessageBoxIcon Icon)
{
	MessageBoxA(static_cast<HWND>(Owner), Text, Caption, Icon == TMessageBoxIcon::Error ? MB_ICONHAND : MB_ICONEXCLAMATION);
}

namespace {

// Player.cpp's CountTibiaWindows until issue #119, its first half: the window's title and class as it read them.
BOOL CALLBACK VisitWindow(HWND Window, LPARAM Parameter)
{
	const auto Visit = reinterpret_cast<TWindowVisitor>(Parameter);
	TFixedString<32> WindowText;
	TFixedString<32> WindowClass;
	const int TextLength = GetWindowTextA(Window, WindowText, 30);
	const int ClassLength = GetClassNameA(Window, WindowClass, 30);
	Visit(WindowText, TextLength, WindowClass, ClassLength);
	return TRUE;
}

} // namespace

void TWin32Window::VisitWindows(TWindowVisitor Visit)
{
	EnumWindows(VisitWindow, reinterpret_cast<LPARAM>(Visit));
}

// The display's modes are SDL3's on every system since issue #152 (TSdlWindow, platform/SdlPlatform.cpp). TWin32Window
// is the golden runner's, whose code asks nothing of the display.
bool TWin32Window::DisplayMode(unsigned int, TDisplayMode &)
{
	return false;
}

bool TWin32Window::DesktopMode(TDisplayMode &)
{
	return false;
}

bool TWin32Window::SetFullscreenMode(TWindowHandle, const TDisplayMode *)
{
	return false;
}

// Stateless, with trivial destructors: constant-initialised, so they are there before any static initialiser and
// after every static destructor.
namespace {
constinit TWin32Clock Win32ClockInstance;
constinit TWin32Input Win32InputInstance;
constinit TWin32Window Win32WindowInstance;
constinit TSdlLoopInput SdlLoopInputInstance;
} // namespace

// Issue #130: the data folder's files (platform/FileSystem.h), set by ConfigureDataFiles (DataLocationWin32.cpp); until
// then, and in the golden runner, the working folder's. Its folders are not constant-initialised: made on first use,
// never destroyed (the files are open to the very end, as the clock is read).
TDataFileSystem &SystemFileSystem()
{
	static TDataFileSystem &Instance = *new TDataFileSystem; // NOLINT(cppcoreguidelines-owning-memory): never destroyed
	return Instance;
}

TWin32Input &Win32Input()
{
	return Win32InputInstance;
}

TWin32Window &Win32Window()
{
	return Win32WindowInstance;
}

TSdlLoopInput &SdlLoopInput()
{
	return SdlLoopInputInstance;
}

IInput &WindowInput()
{
	return SdlLoopInputInstance;
}

IClipboard &WindowClipboard()
{
	return SdlClipboard();
}

IWindow &WindowDesktop()
{
	return SdlWindow();
}

// Patella.exe's and the golden runner's platform (Platform.h). Issue #122: the
// input is the window's (WindowInput); issue #124: the clipboard and the desktop too.
TPlatform &Platform()
{
	static TPlatform Instance{Win32ClockInstance, SystemFileSystem(), NetworkSocket(), WindowClipboard(),
		WindowInput(), WindowDesktop()};
	return Instance;
}

} // namespace patella::platform
