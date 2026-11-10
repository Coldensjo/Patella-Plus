// Surface.cpp: reconstructed from Tibia.exe 7.72.
//
// The first part of the Status/SurfaceDX gap: the surface object of the
// graphics engine, the engine's start-up, the base class TSurface, and the
// data the engines share. The file name is
// inferred (see Surface.h). No function has line evidence, so they are in
// binary order.

// The Windows x64 client (PATELLA_VULKAN, cmake/WindowsX64.cmake) has one engine since Patella-Modern
// issue #20: the Vulkan engine (src/vulkan/SurfaceVulkan.h), without SurfaceDX.cpp,
// SurfaceGL.cpp and surfaceDX9.cpp; GetSurface and SurfaceInitInstance have their own versions for it
// below. The original's versions and the three engines left the fork with the 32-bit oracle (issue #94).
#include "Surface.h"
#include "types/GraphicsTypes.h"
#include "Preferences.h"
#include "vulkan/DeviceSettings.h"
#include "common.h"
#include "vulkan/DeviceChoice.h"
#include "platform/Window.h"
bool IsVulkanInstalled();
TSurface *CreateSurfaceVulkan(patella::platform::TSurfaceShared &Shared);

// Data: TGraphicsEngine's members since issue #116 (Surface.h), in the original's .data and .bss order.

// The surface is made on the first call after SurfaceInitInstance, which names its window and loads the device
// choice. Before that there is none: Control.cpp's ExitControl relies on it after a failed InitControl.
TSurface *patella::TGraphicsEngine::GetSurface()
{
	if (Initialised && Surface == nullptr)
		Surface = CreateSurfaceVulkan(*this);
	return Surface;
}

void patella::TGraphicsEngine::SurfaceExitInstance()
{
	if (Surface != nullptr)
		delete Surface;
	Surface = nullptr;
}

// The default test only takes the main window (the engine's shared data since issue #138).
bool TSurface::TestEngine()
{
	hWindow = Shared.SurfaceWindow;
	return true;
}

// Shared: issue #138's (platform/Surface.h).
TSurface::TSurface(patella::platform::TSurfaceShared &Shared) : Shared(Shared)
{
	NumberOfModes = 0;
	CacheSurfaces = 0;
	Flag26 = false;
}

void TSurface::SetColorKey(int Surface, TRGBColor ColorKey)
{
	SetSourceColorKey(Surface, ColorKey);
}

TSurface::~TSurface() = default;

// The Vulkan engine is the only engine, whatever number the preferences or the command line hold. The
// preferences' Engine keeps one of the original's three numbers, which only this function reads: the
// command line's "engine N" (0-2) and a number 0-2 from the preferences are stored as they are, and no
// number (-1) or one out of range becomes 2, the original's choice on a machine with Direct3D 9. As in
// the original, the command line's number skips the test. The engine's device is the graphics device choice
// (issue #31; vulkan/DeviceSettings.h: PATELLA_VULKAN, else Patella.ini, else automatic), read first;
// the advanced options' buttons choose it (Control.cpp ChangeEngine). When the chosen device fails the
// test (a GPU that is gone, a CPU driver that is missing), the engine runs on the automatic choice, as
// the original falls back to another engine; Patella.ini keeps the choice. The automatic choice is
// tested too, though nothing asks how that ends: without a Vulkan device there is no engine to fall
// back to, and the engine's InitSurfaces or CreateSurfaces fails with PROGRAM_ERROR 12, as the
// original's Direct3D 9 device would.
void patella::TGraphicsEngine::SurfaceInitInstance(platform::TWindowHandle Window)
{
	SurfaceWindow = Window;
	Initialised = true;
	LoadDeviceChoice();
	if (CommandLineEngine >= 0 && CommandLineEngine < 3) {
		SetEngine(CommandLineEngine);
		return;
	}
	int Engine = GetEngine();
	if (Engine < 0 || Engine >= 3)
		Engine = ENGINE_DIRECTX9;
	try {
		if (IsVulkanInstalled()) {
			GetSurface()->hWindow = Window;
			if (!GetSurface()->TestEngine() && GetDeviceChoice().Kind != patella::vulkan::TDeviceKind::Automatic) {
				SetDeviceChoice(patella::vulkan::TDeviceChoice{});
				GetSurface()->TestEngine();
			}
			if (Surface != nullptr)
				delete Surface;
			Surface = nullptr;
		}
	} catch (...) {
		throw TProgramError{PROGRAM_ERROR::SURFACE};
	}
	SetEngine(Engine);
}

// The original's free functions (issue #116): each calls its member on the one TClient's TGraphicsEngine. The other
// files call them, and the golden tests fake GetSurface and SurfaceExitInstance by these symbols.
TSurface *GetSurface()
{
	return patella::bridge::GraphicsEngine().GetSurface();
}

void SurfaceExitInstance()
{
	patella::bridge::GraphicsEngine().SurfaceExitInstance();
}

void SurfaceInitInstance(patella::platform::TWindowHandle Window)
{
	patella::bridge::GraphicsEngine().SurfaceInitInstance(Window);
}
