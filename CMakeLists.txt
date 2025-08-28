cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# CMAKE
# apt install build-essential git
# git clone https://github.com/Kitware/CMake/; cd CMake
# ./bootstrap && make && sudo make install

# VCPKG
# cmake -DCMAKE_TOOLCHAIN_FILE=/opt/workspace/vcpkg/scripts/buildsystems/vcpkg.cmake ..
# .\vcpkg --triplet x64-windows asio luajit glew physfs openal-soft libogg libvorbis zlib opengl nlohmann-json protobuf liblzma openssl
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
	set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()

set(VCPKG_FEATURE_FLAGS "versions")
set(VCPKG_BUILD_TYPE "release")

if (VCPKG_TARGET_ANDROID)
    include("cmake/vcpkg_android.cmake")
	set(Android_INCLUDES ${Android_INCLUDES} ${CMAKE_SOURCE_DIR}/android/app/libs/include)
	set(Android_LIBRARIES ${Android_LIBRARIES} ${CMAKE_SOURCE_DIR}/android/app/libs/lib/${CMAKE_ANDROID_ARCH_ABI})
	include_directories(Android_INCLUDES)
endif()

# *****************************************************************************
# Project otclient
# *****************************************************************************
project(otclient)

# *****************************************************************************
# Append cmake search path
# *****************************************************************************
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# *****************************************************************************
# Include cmake tools
# *****************************************************************************
include(MessageColors)
include(LoggingHelper)
include(GNUInstallDirs)

# *****************************************************************************
# Options
# *****************************************************************************
option(OPTIONS_ENABLE_CCACHE "Enable ccache" OFF)
option(OPTIONS_ENABLE_SCCACHE "Use sccache to speed up compilation process" OFF)
option(OPTIONS_ENABLE_IPO "Check and Enable interprocedural optimization (IPO/LTO)" ON)

# *****************************************************************************
# Options Code
# *****************************************************************************

# === CCACHE ===
if(OPTIONS_ENABLE_CCACHE)
	find_program(CCACHE ccache)
	if(CCACHE)
		log_option_enabled("ccache")
		set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
		else()
		log_option_disabled("ccache")
	endif()
endif()


# === SCCACHE ===
if(OPTIONS_ENABLE_SCCACHE)
  find_program(SCCACHE_PATH sccache)
  if(SCCACHE_PATH)
    log_option_enabled("sccache")
    set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE_PATH})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_PATH})
  else()
    log_option_disabled("sccache")
  endif()
endif()


# === IPO ===
option(OPTIONS_ENABLE_IPO "Check and Enable interprocedural optimization (IPO/LTO)" ON)
if(OPTIONS_ENABLE_IPO)
	log_option_enabled("ipo")

	include(CheckIPOSupported)
	check_ipo_supported(RESULT result OUTPUT output)
	if(result)
		set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
	else()
		log_war("IPO is not supported: ${output}")
	endif()
else()
	log_option_disabled("ipo")
endif()


# *****************************************************************************
# Add project
# *****************************************************************************
# ────────────────────────────────────────────────────────────────────────────────
# Tekst/TTF/RTL — opcje i zależności (vcpkg: freetype, harfbuzz, fribidi)
# ────────────────────────────────────────────────────────────────────────────────
option(OTC_ENABLE_TTF       "Enable TrueType fonts via FreeType" ON)
option(OTC_ENABLE_HARFBUZZ  "Enable text shaping via HarfBuzz"   ON)
option(OTC_ENABLE_FRIBIDI   "Enable bidi for RTL scripts"        ON)

# Znajdź paczki tylko gdy flaga jest włączona
if(OTC_ENABLE_TTF)
  find_package(Freetype CONFIG REQUIRED)
endif()
if(OTC_ENABLE_HARFBUZZ)
  find_package(harfbuzz CONFIG REQUIRED)
endif()
if(OTC_ENABLE_FRIBIDI)
  find_package(fribidi CONFIG REQUIRED)
endif()

# Wspólny INTERFACE target z definicjami i linkowaniem
add_library(otc_textstack INTERFACE)

if(OTC_ENABLE_TTF)
  target_link_libraries(otc_textstack INTERFACE Freetype::Freetype)
  target_compile_definitions(otc_textstack INTERFACE OTC_ENABLE_TTF)
endif()

if(OTC_ENABLE_HARFBUZZ)
  if(TARGET harfbuzz::harfbuzz)
    target_link_libraries(otc_textstack INTERFACE harfbuzz::harfbuzz)
  elseif(TARGET harfbuzz)
    target_link_libraries(otc_textstack INTERFACE harfbuzz)
  endif()
  target_compile_definitions(otc_textstack INTERFACE OTC_ENABLE_HARFBUZZ)
endif()

if(OTC_ENABLE_FRIBIDI)
  if(TARGET fribidi::fribidi)
    target_link_libraries(otc_textstack INTERFACE fribidi::fribidi)
  elseif(TARGET fribidi)
    target_link_libraries(otc_textstack INTERFACE fribidi)
  endif()
  target_compile_definitions(otc_textstack INTERFACE OTC_ENABLE_FRIBIDI)
endif()

# Protobuf
add_subdirectory(src/protobuf)
# Src
add_subdirectory(src)
