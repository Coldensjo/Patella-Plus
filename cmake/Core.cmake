# patella_core: the static library of the portable code, built by the presets windows-x64 and linux-x64-gcc. It is
# everything but the platform layer:
#
#	vlong.cpp, TRSA.cpp   big numbers and the login packet's RSA
#	TXTEA.cpp             the session's XTEA cipher
#	TMyException.cpp      the exception type of PATELLA_ASSERT (compat/assertion.h) and TERROR (common.h)
#	Utils.cpp             the Latin-1 string helpers, outfit colours and packet buffers
#	Skins.cpp             the rectangles of the GUI graphics in Tibia.pic
#	settings/Settings.cpp Patella.ini, the settings file
#	settings/PreferenceKeys.cpp  the options of Tibia.cfg as Patella.ini's keys
#	log/LogSettings.cpp   the developer log's keys in Patella.ini (the log itself, log/Log.h, is header only)
#	Extern/regexp         Henry Spencer's regexp (C17; types/Regexp.h, TRegexp, owns a compiled one)
#	Container.h           the container templates (header only; their mdspan views)
#	AutomapLayer.h        an automap block's layer, 256 x 256 bytes and their mdspan view (header only)
#	compat/mdspan.h       std::mdspan, or its stand-in where the library has none (header only)
#	types/IndexedVector.h TIndexedVector, the growable vector over std::vector (header only)
#	types/Cp1252.h       the client's code page (Windows-1252) to and from UTF-8 and UTF-16 (header only:
#	                      8-bit inside, Unicode at the platform boundary)
#	Network.cpp           the connection's buffers, the frames and the XTEA, the servers (TNetwork)
#	Communication.cpp     the packets: the senders and the receive handlers (TCommunication)
#	Objects.cpp, Map.cpp, Creatures.cpp, Inventory.cpp
#	                      the game state (TMapState's TObjectTypes, TMap, TCreatures and TPlayerState, TInventory)
#
#	Preferences.cpp, Status.cpp, Talk.cpp, Help.cpp, Script.cpp, sysinc.cpp, graphics.cpp, Client.cpp
#	                      the preferences, the session and the messages, the consoles, the hints, the script and
#	                      binary files, the error texts, the sprite and picture caches, the one TClient
#	GUI.cpp, DialogWindow.cpp, DialogElements.cpp, Dialogs.cpp, ContentWindows.cpp, ContextMenus.cpp,
#	ConsoleWindow.cpp, MapWindow.cpp, MainWindow.cpp, Control.cpp
#	                      the GUI's logic, the windows and the dialogs, and the main loop's control
#
# The platform layer is the rest: SdlFrame.cpp (the SDL3 window and event loop), Player.cpp (the application),
# Surface.cpp (which makes the Vulkan engine's surface), src/platform and src/vulkan. Drawing goes through TSurface
# (platform/Surface.h); the library's objects call the platform through platform::Platform(), GetSurface and
# SurfaceExitInstance (Surface.h), and the graphics device choice of vulkan/DeviceSettings.h and DeviceChoice.h
# (Dialogs.cpp's advanced options and Control.cpp's ChangeDevice): what links the library's GUI objects links those.
#
# PATELLA_CORE (common.h) leaves out the Windows headers, so no core source can use them. The clients
# (cmake/WindowsX64.cmake, cmake/LinuxX64.cmake) compile these sources themselves; the library is not linked into
# them.

include("${PROJECT_SOURCE_DIR}/cmake/Warnings.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/Hardening.cmake")

set(patella_core_sources
	src/vlong.cpp
	src/TRSA.cpp
	src/TXTEA.cpp
	src/TMyException.cpp
	src/Utils.cpp
	src/Skins.cpp
	src/settings/Settings.cpp
	src/settings/PreferenceKeys.cpp
	src/log/LogSettings.cpp
	src/Extern/regexp/regexp.c)
# Packet parsing and building, and the game state.
list(APPEND patella_core_sources
	src/Network.cpp
	src/Communication.cpp
	src/Objects.cpp
	src/Map.cpp
	src/Creatures.cpp
	src/Inventory.cpp)
# The preferences, the client's state, the GUI's logic, the windows and the dialogs.
list(APPEND patella_core_sources
	src/Preferences.cpp
	src/Status.cpp
	src/Talk.cpp
	src/Help.cpp
	src/Script.cpp
	src/sysinc.cpp
	src/graphics.cpp
	src/Client.cpp
	src/GUI.cpp
	src/DialogWindow.cpp
	src/DialogElements.cpp
	src/Dialogs.cpp
	src/ContentWindows.cpp
	src/ContextMenus.cpp
	src/ConsoleWindow.cpp
	src/MapWindow.cpp
	src/MainWindow.cpp
	src/Control.cpp)
list(TRANSFORM patella_core_sources PREPEND "${PROJECT_SOURCE_DIR}/")

add_library(patella_core STATIC ${patella_core_sources})
target_include_directories(patella_core PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_compile_definitions(patella_core PUBLIC PATELLA_CORE)
patella_target_language(patella_core)
patella_target_warnings(patella_core)
patella_target_hardening(patella_core)

# Static analysis of the core library: MSVC's code analysis (/analyze; not the headers in angle brackets,
# /analyze:external-) and GCC's static analyzer (-fanalyzer) compile every source of patella_core, so the build
# fails on a finding (/WX, -Werror) like on a warning: the hosted CI's windows-x64 job (MSVC) and linux-x64-gcc job
# run them in Release, and every local build of those presets does, Release and Debug. Clang has none here. The
# findings that stay are suppressed in the code with their reasons. The code analysis does not change the objects;
# the clients compile these sources themselves, without it. PATELLA_STATIC_ANALYSIS=OFF leaves it out (a quicker
# local build).
option(PATELLA_STATIC_ANALYSIS "MSVC /analyze and GCC -fanalyzer on patella_core" ON)
if(PATELLA_STATIC_ANALYSIS)
	if(MSVC)
		target_compile_options(patella_core PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:/analyze /analyze:external->)
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_compile_options(patella_core PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:-fanalyzer>)
	endif()
endif()

# patella_platform: the platform layer's portable part, built by the same presets. TPortableSocket
# (src/platform/PortableSocket.cpp): the network over Winsock (ws2_32) or POSIX sockets, non-blocking and polled from
# the loop, the name lookup on a std::jthread. The clients compile the source themselves (cmake/WindowsX64.cmake, with
# its Windows glue platform/NetworkWin32.cpp; cmake/LinuxX64.cmake, with platform/NetworkLinux.cpp), as they do
# patella_core's. The same static analysis as patella_core's.
add_library(patella_platform STATIC "${PROJECT_SOURCE_DIR}/src/platform/PortableSocket.cpp")
target_include_directories(patella_platform PUBLIC "${PROJECT_SOURCE_DIR}/src")
patella_target_language(patella_platform)
patella_target_warnings(patella_platform)
patella_target_hardening(patella_platform)
if(WIN32)
	target_link_libraries(patella_platform PUBLIC ws2_32)
else()
	find_package(Threads REQUIRED)
	target_link_libraries(patella_platform PUBLIC Threads::Threads)
endif()
if(PATELLA_STATIC_ANALYSIS)
	if(MSVC)
		target_compile_options(patella_platform PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:/analyze /analyze:external->)
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_compile_options(patella_platform PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:-fanalyzer>)
	endif()
endif()
