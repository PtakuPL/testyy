
project(otclient)

# *****************************************************************************
# Options
# *****************************************************************************
if(CMAKE_BASE_NAME STREQUAL "em++")
  set(WASM ON)
  message(STATUS "WASM: ON")
endif()

option(TOGGLE_FRAMEWORK_GRAPHICS "Use Graphics " ON)
option(TOGGLE_FRAMEWORK_SOUND "Use SOUND " ON)
option(TOGGLE_FRAMEWORK_XML "Use XML " ON)
option(TOGGLE_FRAMEWORK_NET "Use NET " ON)
option(TOGGLE_FRAMEWORK_EDITOR "Use Editor " OFF)
option(TOGGLE_DIRECTX "Use DX9 support" OFF)
option(TOGGLE_BIN_FOLDER "Use build/bin folder for generate compilation files" OFF)
option(DEBUG_LOG "Enable Debug Log" OFF)
option(ASAN_ENABLED "Build this target with AddressSanitizer" OFF)
option(BUILD_STATIC_LIBRARY "Build using static libraries" OFF)
option(TOGGLE_PRE_COMPILED_HEADER "Use precompiled header (speed up compile)" ON)
option(SPEED_UP_BUILD_UNITY "Compile using build unity for speed up build" ON)

# *****************************************************************************
# Cmake Features
# *****************************************************************************
set(GNUCXX_MINIMUM_VERSION 9)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


# Make will print more details
set(CMAKE_VERBOSE_MAKEFILE OFF)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# *****************************************************************************
# Client
# *****************************************************************************
if (MSVC)
  add_executable(${PROJECT_NAME} "" ../cmake/icon/otcicon.rc)
elseif(ANDROID)
  add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
else()
  add_executable(${PROJECT_NAME} "")
endif()

# *****************************************************************************
# Options Code
# *****************************************************************************

# Define Framework options for use in compilation
set(FRAMEWORK_DEFINITIONS "")
if (TOGGLE_FRAMEWORK_GRAPHICS)
  set(FRAMEWORK_DEFINITIONS ${FRAMEWORK_DEFINITIONS} -DFRAMEWORK_GRAPHICS)
endif()
if (TOGGLE_FRAMEWORK_SOUND)
  set(FRAMEWORK_DEFINITIONS ${FRAMEWORK_DEFINITIONS} -DFRAMEWORK_SOUND)
endif()
if (TOGGLE_FRAMEWORK_XML)
  set(FRAMEWORK_DEFINITIONS ${FRAMEWORK_DEFINITIONS} -DFRAMEWORK_XML)
endif()
if (TOGGLE_FRAMEWORK_NET)
  set(FRAMEWORK_DEFINITIONS ${FRAMEWORK_DEFINITIONS} -DFRAMEWORK_NET)
endif()

if (TOGGLE_FRAMEWORK_EDITOR)
  set(FRAMEWORK_DEFINITIONS ${FRAMEWORK_DEFINITIONS} -DFRAMEWORK_EDITOR)
endif()
if (ANDROID OR WASM)
  set(FRAMEWORK_DEFINITIONS ${FRAMEWORK_DEFINITIONS} -DOPENGL_ES=2)
endif()

# Set for use precompiled header
if(TOGGLE_PRE_COMPILED_HEADER)
  # === PRECOMPILED HEADER ===
  target_precompile_headers(${PROJECT_NAME} PRIVATE framework/pch.h)
  message(STATUS "Use precompiled header: ON")
else()
  message(STATUS "Use precompiled header: OFF")
endif(TOGGLE_PRE_COMPILED_HEADER)

# === UNITY BUILD (compile time reducer) ===
if(SPEED_UP_BUILD_UNITY)
  set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ON)
  log_option_enabled("Build unity for speed up compilation")
endif(SPEED_UP_BUILD_UNITY)

set(VERSION "1.0.0")

# *****************************************************************************
# Build flags
# *****************************************************************************
if (NOT MSVC)
  if (CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${PROJECT_NAME}  PRIVATE  -Wno-deprecated-declarations)
  endif()
endif()

if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(${PROJECT_NAME} PUBLIC "-pthread")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_definitions(-DNDEBUG)
endif()

# *****************************************************************************
# Definitions code
# *****************************************************************************


# === ASAN ===
if(ASAN_ENABLED)
  log_option_enabled("asan")

  if(MSVC)
    string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    target_compile_options(${PROJECT_NAME} PUBLIC /fsanitize=address)
    target_link_options(${PROJECT_NAME} PUBLIC /fsanitize=address)
  else()
    target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=address)
    target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=address)
  endif()
else()
  log_option_disabled("asan")
endif()


# === DEBUG LOG ===
# cmake -DDEBUG_LOG=ON ..
if(CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_definitions(${PROJECT_NAME} PRIVATE -DDEBUG_LOG=ON )
  log_option_enabled("DEBUG LOG")
else()
  log_option_disabled("DEBUG LOG")
endif(CMAKE_BUILD_TYPE MATCHES Debug)

# *****************************************************************************
# Sanity Check
# *****************************************************************************

# === GCC Minimum Version ===
if (CMAKE_COMPILER_IS_GNUCXX)
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS GNUCXX_MINIMUM_VERSION)
    message(FATAL_ERROR "GCC version must be at least ${GNUCXX_MINIMUM_VERSION}!")
  endif()
endif()


# Multiprocessor compiller
if(CMake_MSVC_PARALLEL)
  if(CMake_MSVC_PARALLEL GREATER 0)
    add_definitions(/MP${CMake_MSVC_PARALLEL})
  else()
    add_definitions(/MP)
  endif()
endif()


# Client options
add_definitions(-DCLIENT)
add_definitions(${FRAMEWORK_DEFINITIONS})
add_definitions(-D"VERSION=${VERSION}")

# === Build options ===
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
add_definitions(-D"BUILD_TYPE=\\\"${CMAKE_BUILD_TYPE}\\\"")
message(STATUS "Build commit: ${BUILD_COMMIT}")
add_definitions(-D"BUILD_COMMIT=\\\"${BUILD_COMMIT}\\\"")
message(STATUS "Build revision: ${BUILD_REVISION}")
add_definitions(-D"BUILD_REVISION=\\\"${BUILD_REVISION}\\\"")

# *****************************************************************************
# Packages / Libs
# *****************************************************************************
include(FetchContent)

  find_package(OpenSSL QUIET)
  find_package(PhysFS CONFIG QUIET)
  if(TARGET PhysFS::PhysFS)
    get_target_property(PHYSFS_INCLUDE_DIR PhysFS::PhysFS INTERFACE_INCLUDE_DIRECTORIES)
    set(PHYSFS_LIBRARY PhysFS::PhysFS)
  else()
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
      pkg_check_modules(PHYSFS physfs)
    endif()
    if(PHYSFS_FOUND)
      set(PHYSFS_LIBRARY ${PHYSFS_LINK_LIBRARIES})
      set(PHYSFS_INCLUDE_DIR ${PHYSFS_INCLUDE_DIRS})
      if(NOT TARGET PhysFS::PhysFS)
        add_library(PhysFS::PhysFS INTERFACE IMPORTED)
        target_include_directories(PhysFS::PhysFS INTERFACE ${PHYSFS_INCLUDE_DIRS})
        target_link_libraries(PhysFS::PhysFS INTERFACE ${PHYSFS_LINK_LIBRARIES})
      endif()
    else()
      message(FATAL_ERROR "Could not find PhysFS; install physfs or libphysfs-dev")
    endif()
  endif()
  find_package(ZLIB REQUIRED)
  find_package(absl CONFIG QUIET)
  find_package(Protobuf REQUIRED)
  find_package(LibLZMA REQUIRED)
  find_package(nlohmann_json CONFIG QUIET)
  if(TARGET nlohmann_json::nlohmann_json)
    get_target_property(NLOHMANN_JSON_INCLUDE_DIR nlohmann_json::nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
    set(NLOHMANN_JSON_LIBRARY nlohmann_json::nlohmann_json)
  else()
    find_path(NLOHMANN_JSON_INCLUDE_DIR NAMES nlohmann/json.hpp)
    if(NOT NLOHMANN_JSON_INCLUDE_DIR)
      message(FATAL_ERROR "Could not find nlohmann/json.hpp; install nlohmann-json3-dev or provide a CMake package")
    endif()
    add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
    target_include_directories(nlohmann_json::nlohmann_json INTERFACE ${NLOHMANN_JSON_INCLUDE_DIR})
    set(NLOHMANN_JSON_LIBRARY nlohmann_json::nlohmann_json)
  endif()
  find_package(asio CONFIG QUIET)
  if(TARGET asio::asio)
    get_target_property(ASIO_INCLUDE_DIR asio::asio INTERFACE_INCLUDE_DIRECTORIES)
  else()
    find_path(ASIO_INCLUDE_DIR NAMES asio.hpp)
    if(NOT ASIO_INCLUDE_DIR)
      FetchContent_Declare(asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-30-2
      )
      FetchContent_MakeAvailable(asio)
      set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include)
    endif()
    if(NOT TARGET asio::asio)
      add_library(asio::asio INTERFACE IMPORTED)
      target_include_directories(asio::asio INTERFACE ${ASIO_INCLUDE_DIR})
      target_compile_definitions(asio::asio INTERFACE ASIO_STANDALONE)
    endif()
  endif()
  find_package(Threads REQUIRED)
  find_package(pugixml CONFIG QUIET)
  if(TARGET pugixml::pugixml)
    get_target_property(PUGIXML_INCLUDE_DIR pugixml::pugixml INTERFACE_INCLUDE_DIRECTORIES)
    set(PUGIXML_TARGET pugixml::pugixml)
  else()
    find_path(PUGIXML_INCLUDE_DIR NAMES pugixml.hpp)
    find_library(PUGIXML_LIBRARY NAMES pugixml)
    if(NOT PUGIXML_INCLUDE_DIR OR NOT PUGIXML_LIBRARY)
      message(FATAL_ERROR "Could not find pugixml; install libpugixml-dev or provide a CMake package")
    endif()
    add_library(pugixml::pugixml INTERFACE IMPORTED)
    target_include_directories(pugixml::pugixml INTERFACE ${PUGIXML_INCLUDE_DIR})
    target_link_libraries(pugixml::pugixml INTERFACE ${PUGIXML_LIBRARY})
    set(PUGIXML_TARGET pugixml::pugixml)
  endif()
  find_package(ZLIB REQUIRED)
  find_package(fmt CONFIG QUIET)
  if(TARGET fmt::fmt)
    set(FMT_TARGET fmt::fmt)
  elseif(TARGET fmt::fmt-header-only)
    set(FMT_TARGET fmt::fmt-header-only)
  else()
    find_path(FMT_INCLUDE_DIR NAMES fmt/format.h)
    if(NOT FMT_INCLUDE_DIR)
      message(FATAL_ERROR "Could not find fmt headers; install libfmt-dev or provide a CMake package")
    endif()
    add_library(fmt::fmt-header-only INTERFACE IMPORTED)
    target_include_directories(fmt::fmt-header-only INTERFACE ${FMT_INCLUDE_DIR})
    add_library(fmt::fmt ALIAS fmt::fmt-header-only)
    set(FMT_TARGET fmt::fmt-header-only)
  endif()

  set(ABSL_LOG_TARGET fmt::fmt)
  if(TARGET absl::log_internal_check_op)
    set(ABSL_LOG_TARGET absl::log_internal_check_op)
  elseif(TARGET absl::raw_logging_internal)
    set(ABSL_LOG_TARGET absl::raw_logging_internal)
  elseif(TARGET absl::log_severity)
    set(ABSL_LOG_TARGET absl::log_severity)
  elseif(TARGET absl::base)
    set(ABSL_LOG_TARGET absl::base)
  endif()

# Header-only BS_thread_pool (task scheduler)
find_path(BS_THREAD_POOL_INCLUDE_DIR NAMES BS_thread_pool.hpp)
if(NOT BS_THREAD_POOL_INCLUDE_DIR)
  FetchContent_Declare(bs_thread_pool
    GIT_REPOSITORY https://github.com/bshoshany/thread-pool.git
    GIT_TAG v4.1.0
  )
  FetchContent_MakeAvailable(bs_thread_pool)
  set(BS_THREAD_POOL_INCLUDE_DIR ${bs_thread_pool_SOURCE_DIR}/include)
endif()

# phmap (parallel-hashmap) may be available via a CMake config (e.g. vcpkg)
# or only as system headers. Try the imported target first, then fall back to
# discovering the include path manually.
find_package(phmap CONFIG QUIET)
  if(TARGET phmap::phmap)
    get_target_property(PARALLEL_HASHMAP_INCLUDE_DIRS phmap::phmap INTERFACE_INCLUDE_DIRECTORIES)
  elseif(NOT PARALLEL_HASHMAP_INCLUDE_DIRS)
    find_path(PARALLEL_HASHMAP_INCLUDE_DIRS NAMES parallel_hashmap/phmap.h)
    if(NOT PARALLEL_HASHMAP_INCLUDE_DIRS)
      FetchContent_Declare(parallel_hashmap
        GIT_REPOSITORY https://github.com/greg7mdp/parallel-hashmap.git
        GIT_TAG v1.3.11
      )
      FetchContent_MakeAvailable(parallel_hashmap)
      set(PARALLEL_HASHMAP_INCLUDE_DIRS ${parallel_hashmap_SOURCE_DIR})
    endif()
    if(NOT TARGET phmap::phmap)
      add_library(phmap::phmap INTERFACE IMPORTED)
      target_include_directories(phmap::phmap INTERFACE ${PARALLEL_HASHMAP_INCLUDE_DIRS})
    endif()
  endif()

# stduuid is header-only; prefer a config package but allow system headers.
find_package(STDUUID CONFIG QUIET)
if(TARGET stduuid::stduuid)
  set(STDUUID stduuid::stduuid)
  get_target_property(STDUUID_INCLUDE_DIRS stduuid::stduuid INTERFACE_INCLUDE_DIRECTORIES)
elseif(TARGET stduuid)
  set(STDUUID stduuid)
  get_target_property(STDUUID_INCLUDE_DIRS stduuid INTERFACE_INCLUDE_DIRECTORIES)
endif()
if(NOT STDUUID)
  find_path(STDUUID_INCLUDE_DIRS NAMES stduuid/uuid.h)

  if(NOT STDUUID_INCLUDE_DIRS)
    # Fetch a header-only copy when no package is available.
    FetchContent_Declare(stduuid_fetch
      GIT_REPOSITORY https://github.com/mariusbancila/stduuid.git
      GIT_TAG v1.2.3
    )
    FetchContent_MakeAvailable(stduuid_fetch)
    set(STDUUID_INCLUDE_DIRS ${stduuid_fetch_SOURCE_DIR}/include)
  endif()

  add_library(stduuid::stduuid INTERFACE IMPORTED)
  target_include_directories(stduuid::stduuid INTERFACE ${STDUUID_INCLUDE_DIRS})
  set(STDUUID stduuid::stduuid)
endif()

# cpp-httplib (HTTP server/client) with pkg-config fallback
  find_package(httplib CONFIG QUIET)
  if(TARGET httplib::httplib)
    set(HTTPLIB_LIBRARY httplib::httplib)
  else()
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
      pkg_check_modules(HTTPLIB cpp-httplib)
    endif()
    if(HTTPLIB_FOUND)
      if(NOT TARGET httplib::httplib)
        add_library(httplib::httplib INTERFACE IMPORTED)
        target_include_directories(httplib::httplib INTERFACE ${HTTPLIB_INCLUDE_DIRS})
        target_compile_definitions(httplib::httplib INTERFACE ${HTTPLIB_CFLAGS_OTHER})
      endif()
      set(HTTPLIB_LIBRARY httplib::httplib)
    else()
      FetchContent_Declare(cpp_httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG v0.15.3
      )
      FetchContent_MakeAvailable(cpp_httplib)
      if(NOT TARGET httplib::httplib)
        add_library(httplib::httplib INTERFACE IMPORTED)
        target_include_directories(httplib::httplib INTERFACE ${cpp_httplib_SOURCE_DIR})
      endif()
      set(HTTPLIB_LIBRARY httplib::httplib)
    endif()
  endif()
if(APPLE)
  # Required for Physfs
  find_library(FOUNDATION Foundation REQUIRED)
  find_library(IOKIT IOKit REQUIRED)
endif()
if(UNIX AND NOT ANDROID AND NOT WASM)
  find_package(X11 REQUIRED)
endif()
if(WIN32)
  find_package(DbgHelp REQUIRED)
endif(WIN32)
if(NOT OPENSSL_FOUND)
  find_package(GMP REQUIRED)
endif()
if(TOGGLE_DIRECTX)
  find_package(DirectX REQUIRED)
endif()
if(TOGGLE_FRAMEWORK_SOUND)
  if(NOT WASM)
        find_package(OpenAL CONFIG QUIET)
    if(NOT TARGET OpenAL::OpenAL)
      find_package(OpenAL REQUIRED)
    endif()
  endif()
  find_package(VorbisFile REQUIRED)
  find_package(Vorbis CONFIG QUIET)
  if(NOT TARGET Vorbis::vorbis)
    find_package(Vorbis REQUIRED)
  endif()
  find_package(Ogg CONFIG QUIET)
  if(NOT TARGET Ogg::ogg)
    find_package(Ogg REQUIRED)
  endif()

  if(OGG_FOUND AND NOT TARGET Ogg::ogg)
    add_library(Ogg::ogg UNKNOWN IMPORTED)
    set_target_properties(Ogg::ogg PROPERTIES
      IMPORTED_LOCATION ${OGG_LIBRARY}
      INTERFACE_INCLUDE_DIRECTORIES ${OGG_INCLUDE_DIR})
  endif()

  if(Vorbis_FOUND AND NOT TARGET Vorbis::vorbis)
    add_library(Vorbis::vorbis UNKNOWN IMPORTED)
    set_target_properties(Vorbis::vorbis PROPERTIES
      IMPORTED_LOCATION ${VORBIS_LIBRARY}
      INTERFACE_INCLUDE_DIRECTORIES ${VORBIS_INCLUDE_DIR})
  endif()

  if(VorbisFile_FOUND AND NOT TARGET Vorbis::vorbisfile)
    add_library(Vorbis::vorbisfile UNKNOWN IMPORTED)
    set_target_properties(Vorbis::vorbisfile PROPERTIES
      IMPORTED_LOCATION ${VORBISFILE_LIBRARY}
      INTERFACE_INCLUDE_DIRECTORIES ${VORBISFILE_INCLUDE_DIR})
  endif()
endif()
if(ANDROID)
  set(LUA_LIBRARY ${LUA_LIBRARY} ${Android_LIBRARIES}/liblua.a)
  set(LUAJIT_INCLUDE_DIR ${LUAJIT_LIBRARY} ${Android_INCLUDES}/luajit)
  set(LUAJIT_LIBRARY ${LUAJIT_LIBRARY} ${Android_LIBRARIES}/libluajit.a)
  set(OpenAL_LIBRARY ${OpenAL_LIBRARY} ${Android_LIBRARIES}/libopenal.a)
  set(MINIZIP_INCLUDE_DIR ${MINIZIP_INCLUDE_DIR} ${Android_INCLUDES}/minizip)
  set(MINIZIP_LIBRARY ${MINIZIP_LIBRARY} ${Android_LIBRARIES}/libminizip.a)
  find_package(game-activity REQUIRED CONFIG)
  find_package(EGL REQUIRED)
else()
  if(NOT WASM)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(LuaJIT REQUIRED)
  else()
    set(LUA_LIBRARY ${LUA_LIBRARY} ${CMAKE_SOURCE_DIR}/browser/include/lua51/liblua.a)
    set(BROWSER_INCLUDE_DIR ${BROWSER_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/browser/include)
  endif()
endif()

# *****************************************************************************
# OTClient source files configuration
# *****************************************************************************
set(SOURCE_FILES
        framework/core/application.cpp
        framework/core/asyncdispatcher.cpp
        framework/core/binarytree.cpp
        framework/core/clock.cpp
        framework/core/config.cpp
        framework/core/configmanager.cpp
        framework/core/event.cpp
        framework/core/eventdispatcher.cpp
        framework/core/filestream.cpp
        framework/core/logger.cpp
        framework/core/module.cpp
        framework/core/modulemanager.cpp
        framework/core/resourcemanager.cpp
        framework/core/scheduledevent.cpp
        framework/core/unzipper.cpp
        framework/core/unzipper.h
        framework/core/timer.cpp
        framework/discord/discord.cpp
        framework/luaengine/luaexception.cpp
        framework/luaengine/luainterface.cpp
        framework/luaengine/luaobject.cpp
        framework/luaengine/luavaluecasts.cpp
        framework/luafunctions.cpp
        framework/net/connection.cpp
        framework/net/inputmessage.cpp
        framework/net/outputmessage.cpp
        framework/net/protocol.cpp
        framework/net/protocolhttp.cpp
        framework/net/httplogin.cpp
        framework/net/server.cpp
        framework/otml/otmldocument.cpp
        framework/otml/otmlemitter.cpp
        framework/otml/otmlexception.cpp
        framework/otml/otmlnode.cpp
        framework/otml/otmlparser.cpp
        framework/platform/androidgameactivity.cpp
        framework/platform/androidmanager.cpp
        framework/platform/androidmanager.h
        framework/platform/androidwindow.cpp
        framework/platform/androidwindow.h
        framework/platform/platform.cpp
        framework/platform/platformwindow.cpp
        framework/platform/unixcrashhandler.cpp
        framework/platform/unixplatform.cpp
        framework/platform/win32crashhandler.cpp
        framework/platform/win32platform.cpp
        framework/stdext/demangle.cpp
        framework/stdext/math.cpp
        framework/stdext/net.cpp
        framework/stdext/string.cpp
        framework/stdext/time.cpp
        framework/stdext/uri.cpp
        framework/stdext/qrcodegen.cpp
        framework/util/color.cpp
        framework/util/crypt.cpp
        framework/proxy/proxy.cpp
        framework/proxy/proxy_client.cpp
        framework/net/packet_player.cpp
        framework/net/packet_recorder.cpp

        client/animatedtext.cpp
        client/animator.cpp
        client/attachableobject.cpp
        client/attachedeffect.cpp
        client/attachedeffectmanager.cpp
        client/client.cpp
        client/container.cpp
        client/creature.cpp
        client/creatures.cpp
        client/effect.cpp
        client/game.cpp
        client/gameconfig.cpp
        client/houses.cpp
        client/item.cpp
        client/itemtype.cpp
        client/lightview.cpp
        client/localplayer.cpp
        client/luafunctions.cpp
        client/luavaluecasts_client.cpp
        client/map.cpp
        client/mapio.cpp
        client/mapview.cpp
        client/minimap.cpp
        client/missile.cpp
        client/outfit.cpp
        client/player.cpp
        client/position.cpp
        client/protocolcodes.cpp
        client/protocolgame.cpp
        client/protocolgameparse.cpp
        client/protocolgamesend.cpp
        client/spriteappearances.cpp
        client/spritemanager.cpp
        client/statictext.cpp
        client/thing.cpp
        client/thingtype.cpp
        client/thingtypemanager.cpp
        client/tile.cpp
        client/towns.cpp
        client/uicreature.cpp
        client/uigraph.cpp
        client/uiitem.cpp
        client/uieffect.cpp
        client/uimissile.cpp
        client/uimap.cpp
        client/uimapanchorlayout.cpp
        client/uiminimap.cpp
        client/uiprogressrect.cpp
        client/uisprite.cpp

        main.cpp
        androidmain.cpp
)

if (TOGGLE_FRAMEWORK_GRAPHICS)
  set(SOURCE_FILES ${SOURCE_FILES}
          framework/core/adaptativeframecounter.cpp
          framework/core/graphicalapplication.cpp
          framework/core/garbagecollection.cpp
          framework/input/mouse.cpp
          framework/graphics/animatedtexture.cpp
            framework/graphics/apngloader.cpp
            framework/graphics/bitmapfont.cpp
            framework/graphics/cachedtext.cpp
            framework/graphics/coordsbuffer.cpp
            framework/text/TTFFont.cpp
            framework/text/TextShaper.cpp
            framework/graphics/textureatlas.cpp
          framework/graphics/drawpool.cpp
          framework/graphics/drawpoolmanager.cpp
          framework/graphics/fontmanager.cpp
          framework/graphics/framebuffer.cpp
          framework/graphics/graphics.cpp
          framework/graphics/image.cpp
          framework/graphics/painter.cpp
          framework/graphics/paintershaderprogram.cpp
          framework/graphics/particle.cpp
          framework/graphics/particleaffector.cpp
          framework/graphics/particleeffect.cpp
          framework/graphics/particleemitter.cpp
          framework/graphics/particlemanager.cpp
          framework/graphics/particlesystem.cpp
          framework/graphics/particletype.cpp
          framework/graphics/shader.cpp
          framework/graphics/shaderprogram.cpp
          framework/graphics/texture.cpp
          framework/graphics/texturemanager.cpp
          framework/graphics/shadermanager.cpp
          framework/platform/win32window.cpp
          framework/platform/x11window.cpp
          framework/ui/uianchorlayout.cpp
          framework/ui/uiboxlayout.cpp
          framework/ui/uigridlayout.cpp
          framework/ui/uihorizontallayout.cpp
          framework/ui/uilayout.cpp
          framework/ui/uimanager.cpp
          framework/ui/uiparticles.cpp
          framework/ui/uitextedit.cpp
          framework/ui/uitranslator.cpp
          framework/ui/uiverticallayout.cpp
          framework/ui/uiwidget.cpp
          framework/ui/uiwidgetbasestyle.cpp
          framework/ui/uiwidgetimage.cpp
          framework/ui/uiwidgettext.cpp
          framework/ui/uiqrcode.cpp
  )
endif()

if (TOGGLE_FRAMEWORK_SOUND)
  set(SOURCE_FILES ${SOURCE_FILES}
          framework/sound/combinedsoundsource.cpp
          framework/sound/oggsoundfile.cpp
          framework/sound/soundbuffer.cpp
          framework/sound/soundchannel.cpp
          framework/sound/soundfile.cpp
          framework/sound/soundmanager.cpp
          framework/sound/soundsource.cpp
          framework/sound/streamsoundsource.cpp
          framework/sound/soundeffect.cpp
  )
endif()

if (WASM)
  set(SOURCE_FILES ${SOURCE_FILES}
          framework/platform/browserplatform.cpp
          framework/platform/browserwindow.cpp
          framework/net/webconnection.cpp
  )
endif()


target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES})
target_link_options(${PROJECT_NAME} PUBLIC -flto=auto)

# *****************************************************************************
# Includes and librarys
# *****************************************************************************
if(MSVC)

  # Set variables to have Windows Vista Value so httplib will build 'inet_pton'
  target_compile_definitions(${PROJECT_NAME}
          PRIVATE
          NTDDI_VERSION=0x06000000
          _WIN32_WINNT=0x0600
  )

  target_compile_options(${PROJECT_NAME} PUBLIC /MP /FS /Zf /EHsc /bigobj)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
  elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
  elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
  endif()

  if(BUILD_STATIC_LIBRARY)
    log_option_enabled("STATIC_LIBRARY")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
    find_package(Vorbis REQUIRED)
    if(ANDROID)
      find_package(Vorbis CONFIG REQUIRED)
    endif()
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
  else()
    log_option_disabled("STATIC_LIBRARY")
    set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "")
  endif()

  target_include_directories(${PROJECT_NAME}
          PRIVATE
          $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/>
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
          ${LUAJIT_INCLUDE_DIR}
          ${Protobuf_INCLUDE_DIRS}
          ${VORBISFILE_INCLUDE_DIR}
          ${GMP_INCLUDE_DIR}
          ${OPENSSL_INCLUDE_DIR}
          ${PHYSFS_INCLUDE_DIR}
          ${PARALLEL_HASHMAP_INCLUDE_DIRS}
          ${NLOHMANN_JSON_INCLUDE_DIR}
          ${BS_THREAD_POOL_INCLUDE_DIR}
          ${STDUUID_INCLUDE_DIRS}
  )
  target_link_libraries(${PROJECT_NAME}
          PRIVATE
          ${LUAJIT_LIBRARY}
          ${CMAKE_THREAD_LIBS_INIT}
          ${PHYSFS_LIBRARY}
          ${ZLIB_LIBRARY}
          ${NLOHMANN_JSON_LIBRARY}
          ${OPENGL_LIBRARIES}
          ${DirectX_LIBRARY}
          ${DirectX_LIBRARIES}
          ${OGG_LIBRARY}
          ${VORBISFILE_LIBRARY}
          ${VORBIS_LIBRARY}
          ${GMP_LIBRARY}
          ${OPENSSL_LIBRARIES}
          ${DBGHELP_LIBRARY}
          ${STDUUID}

          protobuf::libprotobuf protobuf
          ${ABSL_LOG_TARGET}
          Threads::Threads
          GLEW::GLEW
          asio::asio
          OpenAL::OpenAL
          LibLZMA::LibLZMA
          winmm.lib
          pugixml::pugixml
          fmt::fmt-header-only
  )
elseif(ANDROID)
  target_include_directories(${PROJECT_NAME}
          PRIVATE
          $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/>
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
          ${LUAJIT_INCLUDE_DIR}
          ${CMAKE_THREAD_LIBS_INIT}
          ${Protobuf_INCLUDE_DIRS}
          ${GMP_INCLUDE_DIR}
          ${OPENSSL_INCLUDE_DIR}
          ${PHYSFS_INCLUDE_DIR}
          ${EGL_INCLUDE_DIR}
          ${PARALLEL_HASHMAP_INCLUDE_DIRS}
          ${NLOHMANN_JSON_INCLUDE_DIR}
          ${MINIZIP_INCLUDE_DIR}
  )
  target_link_libraries(${PROJECT_NAME}
          PRIVATE
          ${LUA_LIBRARY}
          ${LUAJIT_LIBRARY}
          ${PHYSFS_LIBRARY}
          ${ZLIB_LIBRARY}
          ${NLOHMANN_JSON_LIBRARY}
          ${EGL_LIBRARY}
          ${OGG_LIBRARY}
          ${VORBISFILE_LIBRARY}
          ${VORBIS_LIBRARY}
          ${GMP_LIBRARY}
          ${OpenAL_LIBRARY}
          ${OPENSSL_LIBRARIES}
          ${STDUUID}
          ${MINIZIP_LIBRARY}

          protobuf::libprotobuf protobuf
          absl::log_internal_check_op
          Threads::Threads
          asio::asio
          OpenSLES
          Vorbis::vorbis
          Vorbis::vorbisfile
          LibLZMA::LibLZMA
          game-activity::game-activity
          GLESv3
          android
          log
          pugixml::pugixml
          fmt::fmt-header-only
  )

elseif(WASM)
  target_include_directories(${PROJECT_NAME}
          PRIVATE
          ${CMAKE_SOURCE_DIR}/src
          ${CMAKE_THREAD_LIBS_INIT}
          ${Protobuf_INCLUDE_DIRS}
          ${GMP_INCLUDE_DIR}
          ${PHYSFS_INCLUDE_DIR}
          ${GLEW_INCLUDE_DIR}
          ${PARALLEL_HASHMAP_INCLUDE_DIRS}
          ${NLOHMANN_JSON_INCLUDE_DIR}
          ${OPENSSL_INCLUDE_DIR}
          ${BROWSER_INCLUDE_DIR}
          ${BS_THREAD_POOL_INCLUDE_DIR}
          ${STDUUID_INCLUDE_DIRS}
  )
  target_link_libraries(${PROJECT_NAME}
          PRIVATE
          ${LUA_LIBRARY}
          ${PHYSFS_LIBRARY}
          ${ZLIB_LIBRARY}
          ${PROTOBUF_LIBRARY}
          ${NLOHMANN_JSON_LIBRARY}
          ${GLEW_LIBRARY}
          ${OPENGL_LIBRARIES}
          ${DirectX_LIBRARY}
          ${DirectX_LIBRARIES}
          ${OGG_LIBRARY}
          ${VORBISFILE_LIBRARY}
          ${VORBIS_LIBRARY}
          ${GMP_LIBRARY}
          ${STDUUID}
          ${FOUNDATION}
          ${IOKIT}
          ${OPENSSL_LIBRARY}
          ${OPENSSL_CRYPTO_LIBRARY}
          ${HTTPLIB_LIBRARY}

          protobuf::libprotobuf protobuf
          ${ABSL_LOG_TARGET}

          Threads::Threads
          asio::asio
          # OpenAL::OpenAL (using emscripten openal api)
          LibLZMA::LibLZMA
          pugixml::pugixml
          ZLIB::ZLIB
          OpenSSL::SSL
          OpenSSL::Crypto
          httplib::httplib
          fmt::fmt
          Ogg::ogg
          Vorbis::vorbisfile
          Vorbis::vorbis
  )


  get_property(linkflags TARGET ${PROJECT_NAME} PROPERTY LINK_FLAGS)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME}
            PRIVATE
            -Wall -Wextra -Wpedantic
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    set(linkflags "${linkflags} -sASSERTIONS=1")
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math -frtti -pthread -fexceptions")
  set(linkflags "${linkflags}
			-sINVOKE_RUN=0 -sEXIT_RUNTIME=1 -sTEXTDECODER=0 -sCASE_INSENSITIVE_FS=1
			-lopenal -lidbfs.js -lwebsocket.js -sEXPORTED_RUNTIME_METHODS=ccall -sEXPORTED_FUNCTIONS=_main,_paste_return
			-sINCOMING_MODULE_JS_API=[locateFile,preRun,postRun,print,printErr,canvas,setStatus,monitorRunDependencies]
			-sFORCE_FILESYSTEM -sMALLOC=mimalloc -sWEBSOCKET_SUBPROTOCOL=binary -sOFFSCREENCANVAS_SUPPORT=1
			-sOFFSCREEN_FRAMEBUFFER -sENVIRONMENT=web,worker -sPROXY_TO_PTHREAD -sFULL_ES2=1
			-sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sUSE_PTHREADS=1 -sFETCH=1
			-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency -sALLOW_MEMORY_GROWTH=1
			--preload-file=../otclientrc.lua@otclientrc.lua --preload-file=../init.lua@init.lua
			--preload-file=../data@data --preload-file=../mods@mods --preload-file=../modules@modules
			--shell-file=../browser/shell.html --use-preload-cache")
  set_target_properties(${PROJECT_NAME} PROPERTIES
          LINK_FLAGS ${linkflags})
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  set(VCPKG_TARGET_TRIPLET "wasm32-emscripten" CACHE STRING "")

else() # Linux
  target_include_directories(${PROJECT_NAME}
          PRIVATE
          $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/>
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
          ${LUAJIT_INCLUDE_DIR}
          ${CMAKE_THREAD_LIBS_INIT}
          ${Protobuf_INCLUDE_DIRS}
          ${GMP_INCLUDE_DIR}
          ${PHYSFS_INCLUDE_DIR}
          ${GLEW_INCLUDE_DIR}
          ${PARALLEL_HASHMAP_INCLUDE_DIRS}
          ${NLOHMANN_JSON_INCLUDE_DIR}
          ${OPENSSL_INCLUDE_DIR}
          ${BS_THREAD_POOL_INCLUDE_DIR}
          ${STDUUID_INCLUDE_DIRS}
  )
  target_link_libraries(${PROJECT_NAME}
          PRIVATE
          ${LUAJIT_LIBRARY}
          ${PHYSFS_LIBRARY}
          ${ZLIB_LIBRARY}
          ${NLOHMANN_JSON_LIBRARY}
          ${GLEW_LIBRARY}
          ${OPENGL_LIBRARIES}
          ${DirectX_LIBRARY}
          ${DirectX_LIBRARIES}
          ${OGG_LIBRARY}
          ${VORBISFILE_LIBRARY}
          ${VORBIS_LIBRARY}
          ${GMP_LIBRARY}
          ${STDUUID}
          ${FOUNDATION}
          ${IOKIT}
          ${OPENSSL_LIBRARY}
          ${OPENSSL_CRYPTO_LIBRARY}
          ${HTTPLIB_LIBRARY}

          protobuf::libprotobuf protobuf
          ${ABSL_LOG_TARGET}
          Threads::Threads
          X11::X11
          asio::asio
          OpenAL::OpenAL
          LibLZMA::LibLZMA
          pugixml::pugixml
          ZLIB::ZLIB
          OpenSSL::SSL
          OpenSSL::Crypto
          httplib::httplib
          fmt::fmt-header-only
          Ogg::ogg
          Vorbis::vorbisfile
          Vorbis::vorbis
  )

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME}
            PRIVATE
            -Wall -Wextra -Wpedantic
    )
  endif()

endif()

# Link text stack (freetype/harfbuzz/fribidi)
target_link_libraries(${PROJECT_NAME} PRIVATE otc_textstack)

# *****************************************************************************
# Enable otclient console only for debug build
# *****************************************************************************
if(WIN32)
  set_target_properties(${PROJECT_NAME}
          PROPERTIES
          LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
          LINK_FLAGS_RELEASE "/SUBSYSTEM:windows /ENTRY:mainCRTStartup"
          LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE"
          LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:windows /ENTRY:mainCRTStartup"
  )
endif(WIN32)

if(ANDROID)
  set(CMAKE_SHARED_LINKER_FLAGS "-fuse-ld=lld")
  set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")
endif()

# *****************************************************************************
# Link compilation files folder
# *****************************************************************************

## Link compilation files to build/bin folder, else link to the main dir
if (TOGGLE_BIN_FOLDER)
  set_target_properties(${PROJECT_NAME}
          PROPERTIES
          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
else()
  set_target_properties(${PROJECT_NAME}
          PROPERTIES
          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/"
  )
endif()
