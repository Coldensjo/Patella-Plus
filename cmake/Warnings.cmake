# The language and warning settings of the product targets, in one place. The presets windows-x64 and
# linux-x64-gcc apply them to their targets:
#
#	include(cmake/Warnings.cmake)
#	patella_target_language(<target>)   C++23 and the conformance switches
#	patella_target_warnings(<target>)   the warning level, file by file, and warnings as errors

# DIRECTORY, not GLOBAL: the variables below are directory-scoped, and a directory that includes the
# file after another one did must get them too.
include_guard(DIRECTORY)

# The repository root: the baseline below names its files relative to it.
cmake_path(GET CMAKE_CURRENT_LIST_DIR PARENT_PATH PATELLA_ROOT_DIR)

# The warning baselines, one per compiler: the sources that do not build warning-free at the full
# level yet. They build one level lower (MSVC /W3; GCC and Clang -Wall), still with warnings as
# errors, so the build never stops being green. A file leaves a list when it builds clean at the full
# level, and a list only shrinks; no file is added. A file is on a list when its translation unit
# warns, wherever the warning is (a header included). Paths are relative to the repository root.
# A header on a list is one whose own warnings put its includers on it; no target compiles a header
# alone, so its entry sets no level by itself.
#
# MSVC: measured with MSVC 14.51 (cl 19.51) for x64.
set(PATELLA_WARNING_BASELINE_MSVC
	src/Communication.cpp
	src/ConsoleWindow.cpp
	src/ContentWindows.cpp
	src/ContextMenus.cpp
	src/Control.cpp
	src/Creatures.cpp
	src/DialogElements.cpp
	src/DialogWindow.cpp
	src/Dialogs.cpp
	src/GUI.cpp
	src/MainWindow.cpp
	src/MapWindow.cpp
	src/Player.cpp
	src/Preferences.cpp
	src/Status.cpp
	src/Surface.cpp
	src/TXTEA.cpp
	src/Utils.cpp
	src/vlong.cpp
)
# GCC and Clang: measured with GCC 16.2 and Clang 22.1 on Linux x64 and mingw-w64 GCC 16.2 on Windows x64.
# No preset builds with Clang or mingw-w64.
set(PATELLA_WARNING_BASELINE_GCC
	src/Container.h
	src/TXTEA.cpp
	src/Utils.cpp
	src/vlong.cpp
)
set(PATELLA_WARNING_BASELINE_CLANG
	src/Skins.cpp
	src/TXTEA.cpp
	src/Utils.cpp
	src/vlong.cpp
)
# The packets and the game state (GCC 16.2 and Clang 22.1 in the image, Release, the full level without -Werror):
# GCC Communication.cpp 69 -Wconversion; Clang Communication.cpp 69 -Wimplicit-int-conversion and 17
# -Wsign-conversion, Creatures.cpp 69, Inventory.cpp 63, Network.cpp 24, Map.cpp 18 (with Container.h's) and
# Objects.cpp 2 (with Container.h's 6 in all) -Wsign-conversion.
list(APPEND PATELLA_WARNING_BASELINE_GCC
	src/Communication.cpp)
list(APPEND PATELLA_WARNING_BASELINE_CLANG
	src/Communication.cpp
	src/Creatures.cpp
	src/Inventory.cpp
	src/Map.cpp
	src/Network.cpp
	src/Objects.cpp)

# The preferences, the client's state, the GUI and the windows (measured the same way). GCC: -Wshadow (the windows'
# SetSize(Width, Height) and constructor parameters named as their members) and -Wconversion. Clang:
# -Wsign-conversion, -Wshadow, -Wunnecessary-virtual-specifier (virtual members of final classes); the window headers
# that warn so are on it too.
list(APPEND PATELLA_WARNING_BASELINE_GCC
	src/ConsoleWindow.cpp
	src/ContentWindows.cpp
	src/ContextMenus.cpp
	src/DialogElements.cpp
	src/DialogWindow.cpp
	src/Dialogs.cpp
	src/GUI.cpp
	src/MainWindow.cpp
	src/MapWindow.cpp)
list(APPEND PATELLA_WARNING_BASELINE_CLANG
	src/Client.cpp
	src/ConsoleWindow.cpp
	src/ContentWindows.cpp
	src/ContextMenus.cpp
	src/Control.cpp
	src/DialogElements.cpp
	src/DialogWindow.cpp
	src/Dialogs.cpp
	src/GUI.cpp
	src/MainWindow.cpp
	src/MapWindow.cpp
	src/Preferences.cpp
	src/Script.cpp
	src/Status.cpp
	src/Talk.cpp
	src/graphics.cpp
	src/ConsoleWindow.h
	src/ContentWindows.h
	src/DialogElements.h
	src/DialogWindow.h
	src/Dialogs.h
	src/MapWindow.h)
# C++23 with the conformance switches. MSVC has no /std:c++23 yet, so /std:c++latest; GCC and Clang
# compile as C++26, whose features the code reaches only through src/compat/cxx26.h. The targets do
# not set CXX_STANDARD, which would add a second standard flag. The C sources (the regexp,
# src/Extern/regexp) are C17 with every compiler: MSVC's default is its C89 dialect, GCC 16's C23.
function(patella_target_language target)
	if(MSVC)
		target_compile_options(${target} PRIVATE
			$<$<COMPILE_LANGUAGE:CXX>:/std:c++latest /permissive- /Zc:__cplusplus>
			$<$<COMPILE_LANGUAGE:C>:/std:c17>
			$<$<COMPILE_LANGUAGE:C,CXX>:/utf-8 /Zc:preprocessor /Zc:inline>)
	elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++26>
			$<$<COMPILE_LANGUAGE:C>:-std=c17>)
		# GCC 16 keeps the default contract violation handler, which contract_assert calls
		# (src/compat/cxx26.h), in libstdc++exp.
		if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16)
			target_link_libraries(${target} PRIVATE $<$<LINK_LANGUAGE:CXX>:stdc++exp>)
		endif()
		# Clang with libstdc++ (in the image GCC 16's library) keeps std::stacktrace there too (the developer
		# log's stack trace, src/log/Log.h); with a library that has no libstdc++exp the check fails and
		# std::stacktrace is not used (the header has no __cpp_lib_stacktrace there).
		if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT MSVC)
			if(NOT DEFINED PATELLA_CLANG_STDCXXEXP)
				include(CheckCXXSourceCompiles)
				set(CMAKE_REQUIRED_FLAGS "-std=c++26")
				set(CMAKE_REQUIRED_LIBRARIES stdc++exp)
				check_cxx_source_compiles("#include <stacktrace>
int main() { return static_cast<int>(std::stacktrace::current().size()); }"
					PATELLA_CLANG_STDCXXEXP)
				unset(CMAKE_REQUIRED_FLAGS)
				unset(CMAKE_REQUIRED_LIBRARIES)
			endif()
			if(PATELLA_CLANG_STDCXXEXP)
				target_link_libraries(${target} PRIVATE $<$<LINK_LANGUAGE:CXX>:stdc++exp>)
			endif()
		endif()
		# mingw-w64: the compiler's runtime DLLs are copied next to the executable, where Windows
		# looks first, so it never loads another GCC's libstdc++-6.dll that comes earlier on PATH (a
		# loader error dialog at start). Linking them in (-static) is not possible with GCC 16.2: in
		# C++26 an unoptimised object defines std::logic_error's copy constructor, which the static
		# libstdc++.a defines too (multiple definition).
		get_target_property(type ${target} TYPE)
		if(MINGW AND type STREQUAL "EXECUTABLE")
			cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH compiler_dir)
			set(runtime "")
			foreach(dll libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll)
				if(EXISTS "${compiler_dir}/${dll}")
					list(APPEND runtime "${compiler_dir}/${dll}")
				endif()
			endforeach()
			if(runtime)
				add_custom_command(TARGET ${target} POST_BUILD
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${runtime} "$<TARGET_FILE_DIR:${target}>"
					VERBATIM)
			endif()
		endif()
	else()
		message(FATAL_ERROR "patella_target_language: no settings for ${CMAKE_CXX_COMPILER_ID}")
	endif()
endfunction()

# The full warning level for every source of the target, the baseline level for the files on the
# compiler's PATELLA_WARNING_BASELINE_<MSVC|GCC|CLANG>, and warnings as errors
# (COMPILE_WARNING_AS_ERROR: /WX or -Werror; `cmake --compile-no-warning-as-error` turns it off to
# measure). The level is set per source, so no command line has two levels. Headers included with
# angle brackets (Windows SDK, the standard library) are external to MSVC and give no warnings
# (/external:anglebrackets /external:W0).
# Surface.cpp and the Vulkan engine's two big files with GCC and Clang: the C structures of Vulkan initialised with
# only their type (-Wextra's -Wmissing-field-initializers) and the original's member names reused as parameters
# (-Wshadow), which MSVC does not warn about. At -Wall, as the other files of the original's code.
list(APPEND PATELLA_WARNING_BASELINE_GCC src/Surface.cpp src/vulkan/Renderer.cpp src/vulkan/SurfaceVulkan.cpp)
list(APPEND PATELLA_WARNING_BASELINE_CLANG src/Surface.cpp src/vulkan/Renderer.cpp src/vulkan/SurfaceVulkan.cpp
	# The original's headers through these two (-Wsign-conversion, -Wunnecessary-virtual-specifier): Clang only.
	src/Player.cpp)

function(patella_target_warnings target)
	if(MSVC)
		set(full /W4)
		set(baseline /W3)
		set(baseline_list ${PATELLA_WARNING_BASELINE_MSVC})
		target_compile_options(${target} PRIVATE
			$<$<COMPILE_LANGUAGE:C,CXX>:/external:anglebrackets /external:W0>)
		# The unused-entity warnings on every C++ source, the baseline files included: an unreferenced
		# parameter (C4100), a local that is initialised but not used (C4189) and a removed unreferenced
		# function with internal linkage (C4505) are level 4; /w3 makes them level 3, so the /W3 files
		# get them too (C4101, an unreferenced local, is level 3 already). What is unused on purpose
		# says so with [[maybe_unused]].
		target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/w34100 /w34189 /w34505>)
	elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		set(full -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
		set(baseline -Wall)
		# A switch case that falls through says so with [[fallthrough]], on every C++ source, the
		# baseline files included. GCC's -Wextra takes a "fall through" comment as well (level 3);
		# level 5 takes only the attribute. Clang's -Wextra does not include the warning at all. MSVC has
		# no compiler warning for it (only the code analysis's C26819, /analyze). Not for C: the
		# regexp (src/Extern) is third-party code in C17, which has no [[fallthrough]].
		# The unused-entity warnings likewise: -Wall has -Wunused (variables, functions, labels,
		# typedefs), -Wextra adds the parameters; the baseline files (-Wall) get those too. What is
		# unused on purpose says so with [[maybe_unused]]. Not for C, which has no [[maybe_unused]] in C17.
		target_compile_options(${target} PRIVATE
			$<$<COMPILE_LANGUAGE:CXX>:-Wunused -Wunused-parameter -Wunused-but-set-parameter>)
		if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			set(baseline_list ${PATELLA_WARNING_BASELINE_GCC})
			target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wimplicit-fallthrough=5>)
		else()
			set(baseline_list ${PATELLA_WARNING_BASELINE_CLANG})
			target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wimplicit-fallthrough>)
		endif()
	else()
		message(FATAL_ERROR "patella_target_warnings: no settings for ${CMAKE_CXX_COMPILER_ID}")
	endif()
	set(baseline_files "")
	foreach(file IN LISTS baseline_list)
		cmake_path(ABSOLUTE_PATH file BASE_DIRECTORY "${PATELLA_ROOT_DIR}" NORMALIZE OUTPUT_VARIABLE path)
		list(APPEND baseline_files "${path}")
	endforeach()
	get_target_property(sources ${target} SOURCES)
	get_target_property(source_dir ${target} SOURCE_DIR)
	foreach(source IN LISTS sources)
		cmake_path(ABSOLUTE_PATH source BASE_DIRECTORY "${source_dir}" NORMALIZE OUTPUT_VARIABLE path)
		cmake_path(GET path EXTENSION LAST_ONLY extension)
		string(TOLOWER "${extension}" extension)
		if(NOT extension MATCHES "^\\.(c|cc|cpp|cxx)$")
			continue()
		endif()
		# A source that names a header in its PATELLA_HEADER property has that header's level; nothing sets it now.
		get_property(header SOURCE "${source}" TARGET_DIRECTORY ${target} PROPERTY PATELLA_HEADER)
		if(path IN_LIST baseline_files OR (header AND header IN_LIST baseline_files))
			set(level ${baseline})
		else()
			set(level ${full})
		endif()
		# A source property belongs to the directory, not the target: a source that several targets
		# share gets its level once.
		get_property(options SOURCE "${source}" TARGET_DIRECTORY ${target} PROPERTY COMPILE_OPTIONS)
		list(GET level 0 first)
		if(first IN_LIST options)
			continue()
		endif()
		set_property(SOURCE "${source}" TARGET_DIRECTORY ${target} APPEND PROPERTY COMPILE_OPTIONS ${level})
	endforeach()
	set_property(TARGET ${target} PROPERTY COMPILE_WARNING_AS_ERROR ON)
endfunction()
