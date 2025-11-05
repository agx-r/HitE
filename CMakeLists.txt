cmake_minimum_required(VERSION 3.20)
project(hite C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags for performance
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native -Wall -Wextra -Wno-missing-braces")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Wall -Wextra -Wno-missing-braces")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG")

# Find packages
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

# TinyScheme library
add_library(tinyscheme STATIC external/tinyscheme/scheme.c)
target_compile_definitions(tinyscheme PRIVATE
    STANDALONE=0
    USE_MATH=1
    USE_CHAR_CLASSIFIERS=1
    USE_ASCII_NAMES=1
    USE_STRING_PORTS=1
    USE_ERROR_HOOK=1
    USE_TRACING=1
    USE_COLON_HOOK=1
    USE_DL=0
    USE_PLIST=0
    USE_INTERFACE=0
)
target_include_directories(tinyscheme PUBLIC external/tinyscheme)
# Disable warnings for TinyScheme (it's external code)
target_compile_options(tinyscheme PRIVATE -w)
target_compile_features(tinyscheme PUBLIC c_std_99)

# Source files
file(GLOB_RECURSE SOURCES
    "src/*.c"
    "src/*.cpp"
)

# Executable
add_executable(hite ${SOURCES})

target_include_directories(hite PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/tinyscheme
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(hite PRIVATE
    Vulkan::Vulkan
    glfw
    tinyscheme
    m
    dl
)

# Shader compilation
file(GLOB_RECURSE SHADER_SOURCES
    "${CMAKE_SOURCE_DIR}/shaders/*.comp"
    "${CMAKE_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_SOURCE_DIR}/shaders/*.frag"
)

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT "${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/shaders"
        COMMAND glslangValidator -V ${SHADER} -o ${SHADER_OUTPUT} || ${Vulkan_GLSLC_EXECUTABLE} ${SHADER} -o ${SHADER_OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader ${SHADER_NAME}"
    )
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach()

add_custom_target(shaders ALL DEPENDS ${SHADER_OUTPUTS})
add_dependencies(hite shaders)

# Copy prefab files to build directory (Scheme format)
file(GLOB PREFAB_FILES "${CMAKE_SOURCE_DIR}/prefabs/*.scm")
foreach(PREFAB_FILE ${PREFAB_FILES})
    get_filename_component(PREFAB_NAME ${PREFAB_FILE} NAME)
    configure_file(${PREFAB_FILE} ${CMAKE_BINARY_DIR}/prefabs/${PREFAB_NAME} COPYONLY)
endforeach()

# Copy world files to build directory (Scheme format)
file(GLOB WORLD_FILES "${CMAKE_SOURCE_DIR}/worlds/*.scm")
foreach(WORLD_FILE ${WORLD_FILES})
    get_filename_component(WORLD_NAME ${WORLD_FILE} NAME)
    configure_file(${WORLD_FILE} ${CMAKE_BINARY_DIR}/worlds/${WORLD_NAME} COPYONLY)
endforeach()

# Install targets
install(TARGETS hite DESTINATION bin)
install(DIRECTORY ${CMAKE_BINARY_DIR}/shaders DESTINATION share/hite)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/prefabs DESTINATION share/hite FILES_MATCHING PATTERN "*.scm")
install(DIRECTORY ${CMAKE_BINARY_DIR}/worlds DESTINATION share/hite FILES_MATCHING PATTERN "*.scm")
