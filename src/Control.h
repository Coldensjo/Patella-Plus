// Control.h: the client's main control flow: graphics device changes, redrawing the screen, mouse and keyboard input,
// the timer, the event queue that opens dialogs and starts connections, the network callbacks with the packet
// dispatcher, and start-up and shutdown of all modules.
//
// The functions are TGUI's members (GUI.h), defined in Control.cpp; these free functions of the same names forward to
// them for the callers outside TGUI (SdlFrame.cpp, Player.cpp) and for InitControl's callbacks.

#pragma once

#include "common.h"
#include "platform/Window.h"

#include <array>

void Resize(int Width, int Height);

void Redraw(bool Force);

void MouseMove(int x, int y);
void LeftButtonDown(int x, int y, int Flags);
void LeftButtonUp(int x, int y, int Flags);
void LeftButtonDoubleClick(int x, int y, int Flags);
void RightButtonDown(int x, int y, int Flags);
void RightButtonUp(int x, int y, int Flags);
void MouseWheel(int x, int y, int Direction);
void KeyPress(int Key);
bool CloseRequest();

void LinkCopyWarning();
void ConnectionEstablished();
void ReceivePackets();
void ConnectionClosed();
void TimerEvent();
void ProcessEvents();

void InitWindow(patella::platform::TWindowHandle Window, void (*ExitProgram)(), void (*SetDisplayMode)(int Mode), void (*OpenURL)(const char *URL));
void InitControl(bool Gamemaster);
void ExitControl();

// The last ten packet types received, the newest first (0 for none), for the developer log's diagnostics (Player.cpp
// HandleException).
std::array<uint8, 10> GetLastPacketTypes();
