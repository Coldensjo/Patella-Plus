// DeviceSettings.h: the client's graphics device choice.
//
// The advanced graphics options' three buttons choose the Vulkan engine's device: "Automatic", "GPU"
// (the next GPU on every click when there are several) and "CPU" (the shipped CPU driver). The choice is
// Patella.ini's Device (src/settings/Settings.h); the environment variable PATELLA_VULKAN (the same
// words: auto, gpu, gpuN, cpu) overrides it at start-up and is never written. Surface.cpp
// (SurfaceInitInstance), Control.cpp (ChangeDevice), Dialogs.cpp (TAdvancedGraphicsDialog) and
// SurfaceVulkan.cpp use it. No Vulkan header.

#pragma once

#include "vulkan/DeviceChoice.h"

#include <string>

// At start-up (SurfaceInitInstance), before any device exists: the choice (PATELLA_VULKAN when it is
// set and not empty, a value it does not know meaning automatic; else Patella.ini's Device; else
// automatic), the GPUs (ListGpus) and the CPU driver next to the executable.
void LoadDeviceChoice();

patella::vulkan::TDeviceChoice GetDeviceChoice();
void SetDeviceChoice(const patella::vulkan::TDeviceChoice &Choice);

// Writes the choice as Patella.ini's Device; the file's other lines stay as they are, a new file gets a
// comment line first. False when the file cannot be written, or is there but cannot be read (it is then left
// alone); the choice still holds for this run.
bool SaveDeviceChoice();

// The GPUs found at start-up (ListGpus' order, the one GpuIndex counts in), the one "the best GPU" is,
// and a GPU's name ("" for an index out of range).
int GetNumberOfGpus();
int GetBestGpu();
const char *GetGpuName(int Index);

// The CPU driver's manifest next to the executable (lvp_icd.x86_64.json); empty when it is not there.
const std::string &GetCpuDriverManifest();

// The numbers of the preferences' Engine: Patella.ini's Engine, Tibia.cfg's on its one import and the command
// line's "engine N". They were the original's three engines, 0 DirectX 5, 1 OpenGL and 2 DirectX 9, whose
// buttons stood where the device buttons stand, and a click on a device button still records its place's
// number (Control.cpp's ChangeDevice). The number chooses nothing: the device is Patella.ini's Device. A
// missing number, or one that is none of the three, becomes 2 at start-up (Surface.cpp's SurfaceInitInstance).
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants
	ENGINE_AUTOMATIC = 0,
	ENGINE_CPU = 1,
	ENGINE_GPU = 2,
};

// The number a click on Button records as the preferences' Engine.
int EngineNumberOfButton(patella::vulkan::TDeviceButton Button);

// SurfaceVulkan.cpp: the name of the device the engine runs on ("" while it has none).
const char *GetDeviceName();
