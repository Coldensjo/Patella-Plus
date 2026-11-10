// PresentWindow.h: the windows the Vulkan engine presents in.
//
// Every device the engine creates for the client's window draws into a child window of its own, which
// covers the main window's client area, lets every mouse message through to it (WM_NCHITTEST:
// HTTRANSPARENT) and paints nothing itself; the main window never gets a Vulkan surface. When the device
// goes (a device switch, a change of the view mode, the end), its window is retired, and destroyed once the
// next device has shown its first frame, or at once when it never showed one. Destroying the window takes
// with it whatever the system or a driver still keeps bound to it for presentation: on the main window, the
// AMD Radeon iGPU's cross-adapter presentation stayed bound to it after its device, swapchain, surface and
// instance were destroyed, and the desktop compositor showed its last frame for good over every later
// device's swapchain. Until the next device's first frame, the retired window keeps showing the old
// device's last one.
//
// The main window's client area that no device window covers is filled black whenever the main window
// erases its background: while Windows runs its modal size loop the client draws no frame, and the device
// window keeps its size until the next one (Fit), so the part a larger window uncovers would show whatever
// the compositor had left there, stripes of old pixels, until the mouse was released. The main window's
// class has no background brush (Player.cpp InitInstance, as the original's); TPresentWindows subclasses it
// from the first Open to Close or the main window's end.
//
// On Windows everything runs on the window's thread. On Linux SDL3 has no child windows, so the device's
// window is the main window itself, and these functions keep only that state (a retired device has no
// window to outlive); the device test's window is a hidden SDL window of its own.

#pragma once

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include "compat/win32_types.h" // the Linux client's HWND
#endif

namespace patella::vulkan {

class TPresentWindows {
public:
	TPresentWindows() = default;
	~TPresentWindows();
	TPresentWindows(const TPresentWindows &) = delete;
	TPresentWindows &operator=(const TPresentWindows &) = delete;

	// A new window for a new device, shown over Parent's client area and above the retired one; a current
	// window is retired first. nullptr when it cannot be created.
	HWND Open(HWND Parent);
	// The current window takes Parent's client area again (its size, from the top left corner).
	void Fit();
	// The current window has shown a frame: the retired one is destroyed.
	void Presented();
	// The current window's device is gone: the window is kept until the next one has shown a frame, or
	// destroyed now when it never showed one (a retired window that did is then kept instead).
	void Retire();
	// Both windows destroyed; the main window no longer subclassed.
	void Close();
	// Fills the subclassed main window's client area outside the device windows black in Context (its erase).
	void FillUncovered(HDC Context) const;

private:
	static LRESULT CALLBACK ParentProc(
		HWND Window, UINT Message, WPARAM WParam, LPARAM LParam, UINT_PTR Id, DWORD_PTR Self);
	void Subclass(HWND Window);
	void Unsubclass();

	HWND CurrentWindow = nullptr;
	HWND RetiredWindow = nullptr;
	HWND SubclassedWindow = nullptr; // the main window, subclassed
	bool CurrentPresented = false;
};

// A hidden child window for a device test (Surface.cpp's TestEngine): the test's surface never touches
// the main window or a window that shows frames; destroyed with the object.
class TTestWindow {
public:
	explicit TTestWindow(HWND Parent);
	~TTestWindow();
	TTestWindow(const TTestWindow &) = delete;
	TTestWindow &operator=(const TTestWindow &) = delete;

	[[nodiscard]] HWND Handle() const { return Window; }

private:
	HWND Window = nullptr;
};

} // namespace patella::vulkan
