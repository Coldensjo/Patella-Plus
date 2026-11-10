// Input.h: IInput, the platform's input outside the window's events. Portable: no platform header.
//
// The keys, the buttons and the wheel come to the client as SDL's events (SdlFrame.cpp). What it reads or starts
// besides goes through the process's IInput (platform/Platform.h): the mouse position, polled (Player.cpp InitInstance
// and OnIdle, SdlFrame.cpp); the main frame's 100 ms timer (SdlFrame.cpp), whose ticks are input to the client
// (ActionNumber's); the idle step before the client reads them (CPlayerApp::OnIdle); whether a loss of the mouse
// capture is the client's input; and whether the desktop's keyboard and mouse events are. The client's IInput is
// TSdlLoopInput (platform/Process.h), for which every capture change and desktop event is input.

#pragma once

#include "platform/Window.h"

namespace patella::platform {

// A point on the screen, in pixels.
struct TPoint {
	int x = 0;
	int y = 0;
};

class IInput {
public:
	IInput() = default;
	IInput(const IInput &) = delete;
	IInput &operator=(const IInput &) = delete;
	IInput(IInput &&) = delete;
	IInput &operator=(IInput &&) = delete;

	// The mouse cursor's position on the screen into Point (GetCursorPos): false, Point unchanged, where it cannot
	// be read.
	virtual bool CursorPosition(TPoint &Point) = 0;
	// The window's timer Id: a tick every Milliseconds (SetTimer; WM_TIMER with Id), until StopTimer (KillTimer).
	virtual void StartTimer(TWindowHandle Window, unsigned int Id, unsigned int Milliseconds) = 0;
	virtual void StopTimer(TWindowHandle Window, unsigned int Id) = 0;
	// The start of an idle call (CPlayerApp::OnIdle), before the client polls the cursor and handles its events.
	virtual void BeginIdle() = 0;
	// True where the window's loss of the mouse capture (WM_CAPTURECHANGED) is not the client's input, so that it
	// does not release the held buttons.
	[[nodiscard]] virtual bool IgnoreCaptureChange() = 0;
	// True where the desktop's keyboard and mouse events (SDL3's) are not the client's input: the SDL3 window then
	// hands them to nobody.
	[[nodiscard]] virtual bool IgnoreDesktopInput() = 0;

protected:
	// Never destroyed through the interface (Clock.h's IClock).
	~IInput() = default;
};

} // namespace patella::platform
