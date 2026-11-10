// Platform.h: TPlatform, the platform's services of the process. Portable: no platform header.
//
// The interfaces: IClock (Clock.h), IFileSystem (FileSystem.h), ISocket (Socket.h), IClipboard (Clipboard.h), IInput
// (Input.h) and IWindow (Window.h). TClient's constructor (Client.cpp) takes the process's TPlatform and passes its
// parts on: TNetwork ISocket, TPreferences IFileSystem. The files that are not an object's members, or whose objects
// TClient does not make (sysinc.cpp's clock, Script.cpp's files, the wall clock of Talk.cpp, Creatures.cpp,
// ConsoleWindow.cpp and ContentWindows.cpp, the Vulkan engine's frame timer), reach them through Platform().
//
// Platform() is defined once per system: platform/Win32Platform.cpp on Windows, platform/LinuxPlatform.cpp on Linux.
// Each has the system's clock, TDataFileSystem, NetworkSocket() (TPortableSocket) and the SDL3 window's input,
// clipboard and desktop (Process.h, SdlPlatform.h). Each is made on first use (a static initialiser, sysinc.cpp's
// StartTime, reads the clock before main) and never destroyed (the client reads the clocks until its very end, static
// destructors included).

#pragma once

#include "platform/Clipboard.h"  // IWYU pragma: export
#include "platform/Clock.h"      // IWYU pragma: export
#include "platform/FileSystem.h" // IWYU pragma: export
#include "platform/Input.h"      // IWYU pragma: export
#include "platform/Socket.h"     // IWYU pragma: export
#include "platform/Window.h"     // IWYU pragma: export

namespace patella::platform {

struct TPlatform {
	IClock &Clock;
	IFileSystem &FileSystem;
	ISocket &Socket;
	IClipboard &Clipboard;
	IInput &Input;
	IWindow &Window;
};

// The process's platform (above).
TPlatform &Platform();

} // namespace patella::platform
