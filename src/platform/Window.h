// Window.h: IWindow, the platform's windows and display. Portable: no platform header.
//
// What the client asks of the desktop besides its own window's events: the message boxes (Player.cpp FatalError, the
// start's two), the other top-level windows (InitInstance's count of the Tibia clients, CountTibiaWindows) and the
// display's modes (the Graphics Options dialog's list, vulkan/SurfaceVulkan.cpp GetFullscreenModes; the desktop's
// own, CreateSurfaces' depth check; the fullscreen mode's change, CreateSurfaces and ExitDisplay). Each goes through
// the process's IWindow (platform/Platform.h), which is TSdlWindow (platform/SdlPlatform.h). The frame window itself
// is SdlFrame.cpp's SDL3 window.

#pragma once

namespace patella::platform {

// A window of the platform: an HWND on Windows, an SDL_Window on Linux. Null: none (a box without owner).
using TWindowHandle = void *;

// The message box's icon: MB_ICONHAND (FatalError's), MB_ICONEXCLAMATION (the start's two boxes).
enum class TMessageBoxIcon : unsigned char { Error,
	Warning };

// A display mode: its size in pixels and its bits per pixel (EnumDisplaySettings' dmPelsWidth, dmPelsHeight,
// dmBitsPerPel).
struct TDisplayMode {
	int Width = 0;
	int Height = 0;
	int Depth = 0;
};

// One top-level window: its title and its class name, each at most 29 characters (the original's GetWindowTextA and
// GetClassNameA with 30), and their lengths (0: none, or it could not be read; the text is then undefined).
using TWindowVisitor = void (*)(const char *Title, int TitleLength, const char *ClassName, int ClassLength);

class IWindow {
public:
	IWindow() = default;
	IWindow(const IWindow &) = delete;
	IWindow &operator=(const IWindow &) = delete;
	IWindow(IWindow &&) = delete;
	IWindow &operator=(IWindow &&) = delete;

	// A modal message box with an OK button; returns when it is closed (MessageBoxA). Text and Caption are ASCII.
	virtual void ShowMessageBox(TWindowHandle Owner, const char *Text, const char *Caption, TMessageBoxIcon Icon) = 0;
	// Visit for every top-level window of the desktop, in the platform's order (EnumWindows).
	virtual void VisitWindows(TWindowVisitor Visit) = 0;
	// The display's Index-th mode: false past the last one.
	[[nodiscard]] virtual bool DisplayMode(unsigned int Index, TDisplayMode &Mode) = 0;
	// The display's current mode, the desktop's: false when it cannot be read.
	[[nodiscard]] virtual bool DesktopMode(TDisplayMode &Mode) = 0;
	// Switches the display to Mode (width, height, bits per pixel) for the fullscreen window Window: false where the
	// display has no such mode or the change failed. With Mode null the display goes back to the desktop's mode.
	virtual bool SetFullscreenMode(TWindowHandle Window, const TDisplayMode *Mode) = 0;
	// True when the client runs without a desktop (the test build's --test-headless; Patella.exe's false): its main
	// window is never shown (it exists, hidden, for its messages), the Vulkan engine renders without a surface and
	// presents nothing, and fullscreen mode leaves the display's mode alone.
	[[nodiscard]] virtual bool Headless() = 0;

protected:
	// Never destroyed through the interface (Clock.h's IClock).
	~IWindow() = default;
};

} // namespace patella::platform
