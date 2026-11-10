// Surface.h: TSurface, the platform's drawing surface. Portable: no platform header.
//
// TSurface is the original's base class of the graphics engines (Surface.cpp): the client draws
// through it (graphics.cpp, the window files, Control.cpp) and the engine implements it. It moved here from
// src/Surface.h with its slots, their order and their default bodies (still Surface.cpp's code, at its addresses),
// so that the engine is a platform implementation behind it:
//
// 	TSurfaceVulkan   the Vulkan engine (vulkan/SurfaceVulkan.h): Patella.exe's and the test build's; its surfaces are
// 	                 SDL_Vulkan_CreateSurface's (vulkan/WindowSystem.h)
// 	the golden tests' recording surfaces (tests/golden: testsurface.h, commworld.cpp's)
//
// No Win32 type in it: the window is a TWindowHandle (platform/Window.h), ClipRect's rectangles a TEdgeRect (RECT's
// four edges). What the engine shares with the client (the cache surfaces' size, the main window) is a TSurfaceShared
// that the surface's creator passes to its constructor (TGraphicsEngine, src/Surface.h), instead of the client's
// accessor (bridge::GraphicsEngine()).
//
// The names from the binary: hWindow, ViewMode, the surface and mode constants (SurfaceDX.cpp's and surfaceDX9.cpp's
// assertions), TestEngine, InitSurfaces and ExitSurfaces (the original ChangeEngine's error messages); the rest are ours.

#pragma once

#include "platform/Window.h"
#include "types/GraphicsTypes.h"
#include <cstdint>

struct TFullscreenMode;
class TSurface;

// View modes (TSurface::ViewMode, Init); 0 is none (not initialised). The int constants of the int members and
// parameters, unscoped as the other unnamed ones.
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants, above
	WINDOWED_MODE = 1,
	FULLSCREEN_MODE = 2,
};

// The surfaces of an engine, by index (the int Surface/ToSurface/FromSurface
// arguments of the drawing slots). SPRITE_SURFACE is the first of the video
// cache surfaces (graphics.cpp's Item.Surface + 6). The names are from the
// assertions of SurfaceDX.cpp and surfaceDX9.cpp ("Surface9[SPRITE_SURFACE]").
// Unscoped int constants, as the view modes'.
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants, above
	PRIMARY_SURFACE = 0,
	SCREEN_SURFACE = 1,
	TEMP_SURFACE = 2,
	STRETCH_SURFACE = 3,
	CACHE_SURFACE = 4,
	AUTOMAP_SURFACE = 5,
	SPRITE_SURFACE = 6,
};

namespace patella::platform {

// A rectangle by its edges, in pixels: Win32's RECT (left, top inside; right, bottom outside), the same members in
// the same order, for ClipRect (TRect is a position and a size).
struct TEdgeRect {
	int left = 0;
	int top = 0;
	int right = 0;
	int bottom = 0;
};

// The data the engines share with the client: Surface.cpp's (TGraphicsEngine's, which derives from this). The engine
// sets the cache surfaces' size. The main window is Surface.cpp's file-local value: TGraphicsEngine sets it
// (SurfaceInitInstance), TSurface reads it (TestEngine).
class TSurfaceShared {
public:
	TSurfaceShared(const TSurfaceShared &) = delete;
	TSurfaceShared &operator=(const TSurfaceShared &) = delete;
	TSurfaceShared(TSurfaceShared &&) = delete;
	TSurfaceShared &operator=(TSurfaceShared &&) = delete;

protected:
	TSurfaceShared() = default;
	// Never destroyed through this base (TGraphicsEngine is TGraphics' member).
	~TSurfaceShared() = default;

	friend class ::TSurface;
	TWindowHandle SurfaceWindow{}; // the main window, from SurfaceInitInstance

public:
	int CacheSurfaceWidth = 256; // the size of a video cache surface (256 at start)
	int CacheSurfaceHeight = 256;
};

} // namespace patella::platform

// Slots with _purecall in the binary are pure; the others have the default
// bodies in Surface.cpp. Slot names and parameters come from the calls
// (graphics.cpp and the window files) and from SurfaceDX.cpp's
// implementations; ToSurface, FromSurface, VMode, GMode and ViewMode are the
// binary's (assertions), the other names are ours. Surfaces 6 and up are the
// video cache surfaces.
class TSurface {
public:
	// Shared: the data the engines share with the client (above), the creator's; it outlives the surface.
	explicit TSurface(patella::platform::TSurfaceShared &Shared);
	TSurface(const TSurface &) = delete;
	TSurface &operator=(const TSurface &) = delete;
	TSurface(TSurface &&) = delete;
	TSurface &operator=(TSurface &&) = delete;
	virtual ~TSurface();
	virtual void SetPixel(int Surface, int x, int y, TRGBColor Color) = 0;                       // ContentWindows.cpp's automap cross
	virtual void FillRectangleClipped(int Surface, TRect Rect, TRGBColor Color, TRect Clip) = 0; // ContentWindows.cpp's bars
	virtual void FillRectangle(int Surface, TRect Rect, TRGBColor Color) = 0;
	virtual void DrawHorizontalLine(int Surface, int x, int y, int Width, TRGBColor Color) = 0; // ContentWindows.cpp's separator line
	virtual void DrawVerticalLine(int Surface, int x, int y, int Height, TRGBColor Color) = 0;  // DialogElements.cpp's edit line cursor
	virtual void DrawFrameClipped(int Surface, TRect Rect, int LineWidth, TRGBColor Color,
		TRect Clip) = 0;
	virtual void DrawFrame(int Surface, TRect Rect, int LineWidth, TRGBColor Color) = 0;
	virtual void WriteImage(int ToSurface, int x, int y, int Width, int Height, std::uint8_t *Data) = 0;
	virtual void WriteMaskedImage(int ToSurface, int x, int y, int Width, int Height, std::uint8_t *Data,
		std::uint8_t *Mask, TRGBColor *Colors) = 0;
	// The blits take the source part as a TRect FromRect (and StretchBlit the
	// target as ToRect): the assertions name FromRect.width/height and
	// ToRect.width/height, and every caller builds the 16-byte block on the
	// stack.
	virtual void BlitRect(int ToSurface, int x, int y, int FromSurface, TRect FromRect) = 0;
	virtual void Blit(int ToSurface, int x, int y, int FromSurface, TRect FromRect,
		TRGBColor ColorKey) = 0;
	// BlitRect's worker, a copy without colour key, clipped.
	virtual void BlitRectClipped(int ToSurface, int x, int y, int FromSurface, TRect FromRect,
		TRect Clip) = 0;
	virtual void BlitClipped(int ToSurface, int x, int y, int FromSurface, TRect FromRect,
		TRGBColor ColorKey, TRect Clip) = 0;
	virtual void StretchBlitRect(int ToSurface, TRect Destination, int FromSurface,
		TRect Source) = 0;
	virtual void StretchBlit(int ToSurface, TRect ToRect, int FromSurface, TRect FromRect,
		TRGBColor ColorKey, TRGBColor Highlight, TRect Clip) = 0;
	// Shades Count rectangles with the colours of their four corners
	// (top left, top right, bottom left, bottom right); MapWindow.cpp's light.
	virtual void ShadeRectangles(int Surface, int Count, TRect *Rects, TRGBColor *Colors) = 0;
	virtual void SetColorKey(int Surface, TRGBColor ColorKey); // calls SetSourceColorKey; graphics.cpp sets a cache surface's colour key
	virtual void SetSourceColorKey(int Surface, TRGBColor ColorKey) = 0;
	// Clips Destination to Clip and moves the edges of Source by as
	// much; false when nothing is left.
	virtual bool ClipRect(TRect Clip, patella::platform::TEdgeRect *Destination, patella::platform::TEdgeRect *Source) = 0;
	virtual void ResetFrameTimer() = 0;
	virtual void WaitForNextFrame() = 0; // the frame limiter and the FPS count
	virtual void UpdateWindowRect() = 0;
	virtual void InitDisplay(int VMode, TFullscreenMode *GMode) = 0;
	virtual void ExitDisplay() = 0;
	virtual void CreateSurfaces() = 0;
	virtual void ResetDisplay(int VMode, TFullscreenMode *GMode) = 0;
	virtual void RestoreSurfaces(int ViewMode, TFullscreenMode *Mode) = 0;
	virtual void FinishFrame() = 0;
	// Window is the main window (TGUI's WindowHandle), read during the call.
	virtual bool Init(patella::platform::TWindowHandle *Window, int ViewMode, TFullscreenMode *Resolution, bool Force) = 0;
	virtual int GetFullscreenModes(TFullscreenMode **Modes) = 0; // a new[] list; the number of modes
	virtual int GetCacheSurfaces() = 0;                          // the number of video cache surfaces
	virtual void GetSurfaceRect(int Surface, TRect *Rect) = 0;
	// The surfaces were created again since the last Init (their
	// contents are gone); MapWindow.cpp forgets its tile cache then.
	virtual bool SurfaceRecreated(int Surface) = 0;
	virtual bool IsUnavailable() = 0;
	virtual bool CanShadeRectangles() = 0; // MapWindow.cpp draws the light with ShadeRectangles only when true
	virtual void SetFramerateLimit(int Framerate) = 0;
	virtual double GetFPS() = 0;
	virtual void InitSurfaces() = 0;
	virtual void ExitSurfaces() = 0;
	virtual bool TestEngine();

	// The members the engines share.
	patella::platform::TWindowHandle hWindow; // the main window (TestEngine, SurfaceInitInstance)
	int ViewMode;                             // 0 none, WINDOWED_MODE, FULLSCREEN_MODE
	int Width;                                // the fullscreen mode
	int Height;
	int Depth;
	bool Unavailable;  // no exclusive mode (another application has it)
	bool CanStretch;   // the hardware stretches blits
	bool Flag26;       // cleared by the constructor
	int NumberOfModes; // cleared by the constructor; GetFullscreenModes' count
	int CacheSurfaces; // cleared by the constructor
	double StartTime;  // timeGetTime at ResetFrameTimer
	double NextFrame;  // the time the next frame is due
	double FrameTime;  // 1000 / Framerate
	double FPS;        // frames per second, counted each second
	int Framerate;     // the frame limit; 0xffff is none

protected:
	// The engine's shared data. After the original's members, so their order is the original's.
	patella::platform::TSurfaceShared &Shared;
};
