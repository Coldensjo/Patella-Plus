// Win32Platform.cpp: the Win32 implementations of the platform's interfaces (platform/Win32Platform.h), the Windows
// side of the SDL3 window's input (TSdlLoopInput) and the Windows client's Platform(). Each function of the Win32
// classes is the call the original made itself (timeGetTime, time, Sleep, GetCursorPos, SetTimer, KillTimer,
// MessageBoxA, EnumWindows), with the same arguments.

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

// Win32's GetCursorPos (the owner's decision): where Windows cannot read the cursor (another desktop: the lock
// screen, UAC's secure desktop) it fails and the point stays the last one, as the original's; SDL_GetGlobalMouseState
// would report (0, 0) there.
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

// The window's title and class for Player.cpp's CountTibiaWindows, read as the original read them.
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

// The display's modes are SDL3's on every system (TSdlWindow, platform/SdlPlatform.cpp), which asks TWin32Window for
// the desktop's windows only.
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

// The data folder's files (platform/FileSystem.h), set by ConfigureDataFiles (DataLocationWin32.cpp); until then the
// working folder's. Its folders are not constant-initialised: made on first use, never destroyed (the files are open
// to the very end, as the clock is read).
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

// Patella.exe's platform (Platform.h). The input, the clipboard and the desktop are the SDL3 window's.
TPlatform &Platform()
{
	static TPlatform Instance{Win32ClockInstance, SystemFileSystem(), NetworkSocket(), WindowClipboard(),
		WindowInput(), WindowDesktop()};
	return Instance;
}

} // namespace patella::platform
