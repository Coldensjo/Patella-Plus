// Surface.cpp: the surface object of the graphics engine, the engine's start-up, the default bodies of the base class
// TSurface, and the data the engines share (TGraphicsEngine, Surface.h).
//
// The client has one engine, the Vulkan engine (src/vulkan/SurfaceVulkan.h): GetSurface makes its surface, and
// SurfaceInitInstance names its window and chooses its device.
#include "Surface.h"
#include "types/GraphicsTypes.h"
#include "Preferences.h"
#include "vulkan/DeviceSettings.h"
#include "common.h"
#include "vulkan/DeviceChoice.h"
#include "platform/Window.h"
bool IsVulkanInstalled();
TSurface *CreateSurfaceVulkan(patella::platform::TSurfaceShared &Shared);

// The data is TGraphicsEngine's (Surface.h).

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

// The default test only takes the main window (the engine's shared data).
bool TSurface::TestEngine()
{
	hWindow = Shared.SurfaceWindow;
	return true;
}

// Shared: the data the engines share (platform/Surface.h).
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

// The Vulkan engine is the only engine. Its device is the graphics device choice (vulkan/DeviceSettings.h:
// PATELLA_VULKAN, else Patella.ini's Device, else automatic), read first; the advanced options' buttons
// choose it (Control.cpp ChangeDevice). When the chosen device fails the test (a GPU that is gone, a CPU
// driver that is missing), the engine runs on the automatic choice, as the original falls back to another
// engine; Patella.ini keeps the choice. The automatic choice is tested too, though nothing asks how that
// ends: without a Vulkan device there is no engine to fall back to, and the engine's InitSurfaces or
// CreateSurfaces fails with PROGRAM_ERROR 12, as the original's Direct3D 9 device would.
//
// The preferences' Engine (vulkan/DeviceSettings.h) chooses nothing and is only kept in order here: the
// command line's "engine N" (0-2) and a number 0-2 from the preferences are stored as they are, and no
// number (-1) or one out of range becomes 2, the number the original stored on a machine with Direct3D 9.
// The command line's number also skips the device test, as the original's skipped the engine test.
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
		Engine = ENGINE_GPU;
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
