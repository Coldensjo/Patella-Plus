// Surface.h: the graphics engine choice and its data (TGraphicsEngine, Surface.cpp's). TSurface, the engines' base
// class, is the platform's interface (platform/Surface.h, exported here); its default bodies are Surface.cpp's.

#pragma once

#include "platform/Surface.h" // IWYU pragma: export
#include "platform/Window.h"

namespace patella {

// Surface.cpp's data: the command line's engine number, the engine's surface object, and the data the engines share
// (the blit statistics, the screen, the cache surfaces' size, the main window: platform::TSurfaceShared, the base,
// which the engine's surface gets at its construction), with the same initial values. A part of TGraphics (graphics.h,
// its member Engine), declared here and defined in Surface.cpp, as TSession is Status.cpp's inside TCommunication.
// Surface.cpp's functions are its members of the same names; the free functions forward to them. The data the other
// files read or write was extern and is public (Control.cpp, Dialogs.cpp, MainFrm.cpp, Player.cpp and the golden tests;
// the Vulkan engine through its TSurfaceShared); the surface object is private, and the main window is the base's, read
// by TSurface's TestEngine.
class TGraphicsEngine final : public platform::TSurfaceShared {
public:
	TGraphicsEngine() = default;
	TGraphicsEngine(const TGraphicsEngine &) = delete;
	TGraphicsEngine &operator=(const TGraphicsEngine &) = delete;
	TGraphicsEngine(TGraphicsEngine &&) = delete;
	TGraphicsEngine &operator=(TGraphicsEngine &&) = delete;
	~TGraphicsEngine() = default;

	TSurface *GetSurface(); // the x64 client's
	void SurfaceExitInstance();
	void SurfaceInitInstance(platform::TWindowHandle Window); // the x64 client's

	// .data (the base's after the cache surfaces' size)
	int CommandLineEngine = -1; // "engine N" on the command line (Player.cpp); -1 = none
	// .bss (the base's, but for these two)
	bool Initialised{}; // SurfaceInitInstance has run: only from then on does GetSurface make the surface

private:
	TSurface *Surface{}; // the engine's surface; raw, as the original's (deleted by SurfaceExitInstance)
};

// The transition's access path: the one TClient's TGraphicsEngine (Client.cpp). It goes when no free function needs it.
namespace bridge {
TGraphicsEngine &GraphicsEngine();
} // namespace bridge

} // namespace patella

TSurface *GetSurface();
void SurfaceExitInstance();
void SurfaceInitInstance(patella::platform::TWindowHandle Window);
