// SurfaceVulkan.h: the Vulkan engine's surface on the client's window.
//
// TSurfaceVulkan does what surfaceDX9.cpp's TSurfaceDX9 does, call for call, the reference engine of the frames: the
// same surfaces and sizes, the same batching of quads (drawn when the texture or the target changes, when 9,600
// vertices are queued, and at the end of the frame), the same colour fills, uploads, colour key and light, and the
// same statistics. Instead of Direct3D 9 it drives the Vulkan back end (Renderer.h); the display modes come from the
// platform's list (SDL3's), in the order and with the filter DirectDraw 4 gave surfaceDX9.cpp. Windows x64 client only
// (cmake/Vulkan.cmake).
//
// The engine is the x64 client's only engine (Surface.cpp). Its device is the graphics device choice of
// DeviceSettings.h: the advanced options' buttons, Patella.ini's Device, PATELLA_VULKAN. Its window system (the client
// area, the screen, the surfaces) is SDL3's, vulkan/WindowSystem.h.

#pragma once

#include "Surface.h"
#include "compat/msvc.h"
#include "graphics.h"
#include "vulkan/Renderer.h"
#include <array>
#include <memory>
#include <vector>
#include "common.h"
#include "vulkan/Quads.h"

enum : int { // NOLINT(cppcoreguidelines-use-enum-class): an int constant, as in platform/Surface.h
	NUMBER_OF_SURFACES_VULKAN = 22
};

class TSurfaceVulkan final : public TSurface {
public:
	// Shared: the data the engine shares with the client (platform/Surface.h; TGraphicsEngine).
	explicit TSurfaceVulkan(patella::platform::TSurfaceShared &Shared);
	~TSurfaceVulkan() override;

	void SetPixel(int Surface, int x, int y, TRGBColor Color) override;
	void FillRectangleClipped(int Surface, TRect Rect, TRGBColor Color, TRect Clip) override;
	void FillRectangle(int Surface, TRect Rect, TRGBColor Color) override;
	void DrawHorizontalLine(int Surface, int x, int y, int Width, TRGBColor Color) override;
	void DrawVerticalLine(int Surface, int x, int y, int Height, TRGBColor Color) override;
	void DrawFrameClipped(int Surface, TRect Rect, int LineWidth, TRGBColor Color, TRect Clip) override;
	void DrawFrame(int Surface, TRect Rect, int LineWidth, TRGBColor Color) override;
	void WriteImage(int ToSurface, int x, int y, int Width, int Height, uint8 *Data) override;
	void WriteMaskedImage(int ToSurface, int x, int y, int Width, int Height, uint8 *Data, uint8 *Mask,
		TRGBColor *Colors) override;
	void BlitRect(int ToSurface, int x, int y, int FromSurface, TRect FromRect) override;
	void Blit(int ToSurface, int x, int y, int FromSurface, TRect FromRect, TRGBColor ColorKey) override;
	void BlitRectClipped(int ToSurface, int x, int y, int FromSurface, TRect FromRect, TRect Clip) override;
	void BlitClipped(int ToSurface, int x, int y, int FromSurface, TRect FromRect, TRGBColor ColorKey,
		TRect Clip) override;
	void StretchBlitRect(int ToSurface, TRect Destination, int FromSurface, TRect Source) override;
	void StretchBlit(int ToSurface, TRect ToRect, int FromSurface, TRect FromRect, TRGBColor ColorKey,
		TRGBColor Highlight, TRect Clip) override;
	void ShadeRectangles(int Surface, int Count, TRect *Rects, TRGBColor *Colors) override;
	void SetSourceColorKey(int ToSurface, TRGBColor ColorKey) override;
	bool ClipRect(TRect Clip, patella::platform::TEdgeRect *Rect, patella::platform::TEdgeRect *Source) override;
	void ResetFrameTimer() override;
	void WaitForNextFrame() override;
	void UpdateWindowRect() override;
	void InitDisplay(int VMode, TFullscreenMode *GMode) override;
	void ExitDisplay() override;
	void CreateSurfaces() override;
	void ResetDisplay(int VMode, TFullscreenMode *GMode) override;
	void RestoreSurfaces(int ViewMode, TFullscreenMode *Resolution) override;
	void FinishFrame() override;
	bool Init(patella::platform::TWindowHandle *Window, int ViewMode, TFullscreenMode *Resolution, bool Force) override;
	int GetFullscreenModes(TFullscreenMode **Modes) override;
	int GetCacheSurfaces() override;
	void GetSurfaceRect(int Surface, TRect *Rect) override;
	bool SurfaceRecreated(int Surface) override;
	bool IsUnavailable() override;
	bool CanShadeRectangles() override;
	void SetFramerateLimit(int Limit) override;
	double GetFPS() override;
	void InitSurfaces() override;
	void ExitSurfaces() override;
	bool TestEngine() override;

	// The device's name ("" without one; DeviceSettings.h GetDeviceName).
	[[nodiscard]] const char *DeviceName() const;

private:
	using TEdgeRect = patella::platform::TEdgeRect; // the rectangles (RECT's edges)

	void AddShadeQuad(int Left, int Top, int Right, int Bottom, uint32 TopLeft, uint32 TopRight, uint32 BottomRight,
		uint32 BottomLeft);
	void FlushVertices();
	void SetTexture(int Surface);
	void SetRenderTarget(int Surface);
	void ApplyColorKey(int Surface, TRGBColor ColorKey);
	void FlushShade();
	void AddQuad(int Left, int Top, int Right, int Bottom, double Left_u, double Right_u, double Top_v,
		double Bottom_v, uint32 TopLeft, uint32 TopRight, uint32 BottomRight, uint32 BottomLeft);
	void ColorFill(int Image, const patella::platform::TEdgeRect *Rect, uint32 Color);
	// The main window as Win32's, for the devices' own windows (PresentWindow.h; hWindow is a TWindowHandle).
	[[nodiscard]] HWND MainWindow() const { return static_cast<HWND>(hWindow); }
	patella::vulkan::TDeviceRequest Request(patella::platform::TWindowHandle Window);
	void Fail();

	bool Ready;                                            // CreateSurfaces has finished (SurfaceRecreated)
	std::array<TEdgeRect, NUMBER_OF_SURFACES_VULKAN> Rect; // the surfaces' rectangles, as surfaceDX9.cpp's
	TEdgeRect WindowRect;                                  // the client area (from 0, 0: only its size)
	bool Initialized;                                      // between InitSurfaces and ExitSurfaces (surfaceDX9.cpp: DirectDraw exists)
	std::unique_ptr<patella::vulkan::TRenderer> Renderer;  // surfaceDX9.cpp's Direct3D and Device
	std::array<int, NUMBER_OF_SURFACES_VULKAN> Image;      // the renderer's images (SCREEN_SURFACE: the canvas)
	int ScreenTexture;                                     // the 106x106 texture of SCREEN_SURFACE
	int WhiteTexture;                                      // Direct3D 9's stage without a texture: opaque white
	int DeviceTarget;                                      // the image the device draws quads into
	int DeviceTexture;                                     // the image the device samples (-1 none)
	int CurrentTarget;                                     // surfaceDX9.cpp's CurrentTarget: ColorFill's image
	patella::vulkan::TFilter Filter;                       // the sampler state (D3DSAMP_MAGFILTER/MINFILTER)
	std::vector<patella::vulkan::TQuad> Shade;             // ShadeVertices
	bool DisplayModeSet;                                   // fullscreen changed the display mode
};

// Surface.cpp's engine selection.
bool IsVulkanInstalled();
TSurface *CreateSurfaceVulkan(patella::platform::TSurfaceShared &Shared);
