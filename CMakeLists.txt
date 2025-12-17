cmake_minimum_required(VERSION 3.16)

# Project setup
project(Pyre VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# /FS forces synchronous writes to the PDB file
if(MSVC)
    add_compile_options(/FS) 
endif()

# DEPENDENCIES
include(FetchContent)

# GLFW
message(STATUS "Downloading GLFW...")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    URL https://github.com/glfw/glfw/archive/refs/tags/3.3.8.zip
)
FetchContent_MakeAvailable(glfw)

# ASSIMP
message(STATUS "Building Assimp from local source...")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE) 

FetchContent_Declare(
    assimp
    URL ${CMAKE_CURRENT_SOURCE_DIR}/deps/assimp-5.2.5.zip
)
FetchContent_MakeAvailable(assimp)
# SOURCE FILES
file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "src/*.c"
    "includes/thirdparty/*.cpp"
    "includes/thirdparty/*.c"
)

# EXECUTABLE TARGET
add_executable(${PROJECT_NAME} ${SOURCES})

# INCLUDE DIRECTORIES
target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/thirdparty 
)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw assimp)

# LINKING LIBRARIES (OS Specifics)
find_package(OpenGL REQUIRED)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
elseif (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL dl pthread)
endif()

# ASSET COPYING
# This creates the folders inside build/Debug/ and copies the files.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
        
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources

    COMMENT "Copying shaders and resources to build directory..."
)

#  VS DEBUGGER CONFIGURATION
if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES 
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
endif()