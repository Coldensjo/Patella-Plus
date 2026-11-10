// Win32Platform.h: the Win32 implementations of the platform's interfaces (platform/Platform.h): the clock, and what
// the SDL3 window's input and desktop take from Win32 (the cursor's position on the screen, the other top-level
// windows). The rest of the Windows client's platform is portable: TDataFileSystem (platform/FileSystem.h),
// NetworkSocket() and the SDL3 window's input, clipboard and desktop (platform/Process.h, platform/SdlPlatform.h).

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

// MessageBoxA, EnumWindows (GetWindowTextA, GetClassNameA). No display modes: those are SDL3's (TSdlWindow).
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

// The Win32 implementations, there before any static initialiser and never destroyed: TSdlLoopInput reads the cursor
// through the one, TSdlWindow visits the desktop's windows through the other.
TWin32Input &Win32Input();
TWin32Window &Win32Window();
} // namespace patella::platform
