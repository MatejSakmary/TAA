cmake_minimum_required (VERSION 3.21)
project(TAA)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")

add_executable(${PROJECT_NAME}
    "source/main.cpp"
    "source/scene.cpp"
    "source/camera.cpp"
    "source/application.cpp"
    "source/renderer/renderer.cpp"
    "source/external/stb_image_impl.cpp"
)

find_package(assimp CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(daxa CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb_c_lexer.h")

target_include_directories(${PROJECT_NAME} PRIVATE ${STB_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE
    glm::glm
    imgui::imgui
    daxa::daxa
    assimp::assimp
    glfw
)
# Debug mode defines
target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:LOG_DEBUG>")

# This creates a marko define that can be used to find the daxa include folder for shader compilation.
set(DAXA_INCLUDE_DIR "$<TARGET_FILE_DIR:TAA>/../../vcpkg_installed/x64-$<LOWER_CASE:$<PLATFORM_ID>>/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE DAXA_SHADER_INCLUDE_DIR="${DAXA_INCLUDE_DIR}")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

set(PACKAGE_APP true)

if(PACKAGE_APP)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        list(APPEND RUNTIME_ARTIFACT_TARGETS glfw assimp::assimp)
    endif()

    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
    install(IMPORTED_RUNTIME_ARTIFACTS ${RUNTIME_ARTIFACT_TARGETS})
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/source/renderer/shaders" DESTINATION bin)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/source/renderer/shared" DESTINATION bin/shared)
    install(FILES "${DAXA_INCLUDE_DIR}/daxa/daxa.inl" "${DAXA_INCLUDE_DIR}/daxa/daxa.glsl" "${DAXA_INCLUDE_DIR}/daxa/daxa.hlsl" DESTINATION bin/shaders/daxa)
    install(FILES "${CMAKE_SOURCE_DIR}/imgui.ini" DESTINATION bin)

    set(CPACK_PACKAGE_NAME "TAA app")
    set(CPACK_PACKAGE_VENDOR "Matej-Sakmary")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "TAA")
    set(CPACK_PACKAGE_DESCRIPTION "TAA")

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(CPACK_PACKAGE_EXECUTABLES ${PROJECT_NAME} "TAA app")

        # Set the default installation directory. In this case it becomes C:/Program Files/GabeVoxelGame
        set(CPACK_PACKAGE_INSTALL_DIRECTORY "TAA app")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    endif()

    include(InstallRequiredSystemLibraries)
    include(CPack)
endif()