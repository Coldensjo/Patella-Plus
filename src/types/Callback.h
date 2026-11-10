// types/Callback.h: TCallback, a receiver and an event number (GUI.h's until Patella-Modern issue #110). GUI.h
// exports it, so the window files see it as before. Its two functions are still GUI.cpp's.

#pragma once

class TComponent;

// A receiver and an event number (passed by value): Send() calls
// Receiver->HandleEvent(Event). Buttons, timers and hotkeys keep one.
struct TCallback {
	TCallback() : Receiver(nullptr), Event(0) {} // inlined (vector<THotkey>, TTimer)
	TCallback(TComponent *Receiver, int Event);
	void Send() const;
	TComponent *Receiver;
	int Event;
};
