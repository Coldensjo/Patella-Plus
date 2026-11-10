// Win32Platform.h: the Win32 implementations of the platform's interfaces (platform/Platform.h). Not part of the
// original source.
//
// The code behind each is what the client called itself before, call for call: timeGetTime, time and Sleep (the
// clock). The file system is the portable TDataFileSystem (platform/FileSystem.h); the network is NetworkSocket(),
// below. Also the input (GetCursorPos, SetTimer, KillTimer: MainFrm.cpp's and Player.cpp's) and the desktop's windows
// (MessageBoxA, EnumWindows: Player.cpp's). The SDL3 window's clipboard and desktop are SDL3's
// (platform/SdlPlatform.h), and these the golden runner's; the display's modes are SDL3's alone.

#pragma once

#include "common.h"
#include "platform/Platform.h"
#include "platform/Process.h" // IWYU pragma: export

#include <cstdint>

namespace patella::platform {

// timeGetTime, time, Sleep.
class TWin32Clock final : public IClock {
public:
	[[nodiscard]] std::uint32_t Milliseconds() override;
	[[nodiscard]] std::int64_t WallSeconds() override;
	void FrameWait(std::uint32_t Duration) override;
};

// GetCursorPos, SetTimer, KillTimer; nothing at the idle step, every capture change the client's.
class TWin32Input final : public IInput {
public:
	bool CursorPosition(TPoint &Point) override;
	void StartTimer(TWindowHandle Window, unsigned int Id, unsigned int Milliseconds) override;
	void StopTimer(TWindowHandle Window, unsigned int Id) override;
	void BeginIdle() override {}
	[[nodiscard]] bool IgnoreCaptureChange() override { return false; }
	[[nodiscard]] bool IgnoreDesktopInput() override { return false; }
};

// MessageBoxA, EnumWindows (GetWindowTextA, GetClassNameA). No display modes: the golden runner asks none.
class TWin32Window final : public IWindow {
public:
	void ShowMessageBox(TWindowHandle Owner, const char *Text, const char *Caption, TMessageBoxIcon Icon) override;
	void VisitWindows(TWindowVisitor Visit) override;
	[[nodiscard]] bool DisplayMode(unsigned int Index, TDisplayMode &Mode) override;
	[[nodiscard]] bool DesktopMode(TDisplayMode &Mode) override;
	bool SetFullscreenMode(TWindowHandle Window, const TDisplayMode *Mode) override;
	// Patella.exe always has a desktop.
	[[nodiscard]] bool Headless() override { return false; }
};

// The Win32 implementations, made on first use and never destroyed: Patella.exe's Platform(), and the parts of the
// test build's (TestModeWin32.cpp) that are not the test mode's.
TWin32Input &Win32Input();
TWin32Window &Win32Window();
} // namespace patella::platform
