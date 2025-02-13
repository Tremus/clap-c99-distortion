cmake_minimum_required(VERSION 3.15)
project(clap-c99-distortion VERSION 0.9.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "Build for 10.15" FORCE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

# use asan as an option (currently mac only)
option(USE_SANITIZER "Build and link with ASAN" FALSE)

# Copy on mac (could expand to other platforms)
option(COPY_AFTER_BUILD "Copy the clap to ~/Library on MACOS, ~/.clap on linux" FALSE)

add_subdirectory(libs/clap EXCLUDE_FROM_ALL)
set(CLAP_WRAPPER_OUTPUT_NAME ${PROJECT_NAME})
set(VST3_SDK_ROOT ${PROJECT_SOURCE_DIR}/libs/vst3sdk)
add_subdirectory(libs/clap-wrapper)
add_subdirectory(libs/nanovg_compat)

set(LIBS clap-core plugin_platform nanovg_compat)
set(DIRS 
    libs/clap/include
    libs/nanovg_compat/src
    libs/nanovg_compat/modules/nanovg_dx11/src
)

if (APPLE)
    add_library(plugin_platform STATIC src/platform_macos.m)
    list(APPEND LIBS "-framework Cocoa -framework Metal -framework QuartzCore")
    list(APPEND DIRS libs/nanovg_compat/modules/MetalNanoVG/src)
    target_include_directories(plugin_platform PRIVATE ${DIRS})
elseif(WIN32)
    list(APPEND LIBS d3d11 dxguid)
endif()

add_library(${PROJECT_NAME} MODULE
        src/clap-c99-distortion.c)
target_include_directories(${PROJECT_NAME} PRIVATE ${DIRS})
target_link_libraries(${PROJECT_NAME} ${LIBS})

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        BUNDLE True
        BUNDLE_EXTENSION clap
        MACOSX_BUNDLE_GUI_IDENTIFIER org.surge-synth-team.${PROJECT_NAME}
        MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/cmake/${PROJECT_NAME}.plist.in
        )
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Werror
        $<$<BOOL:${USE_SANITIZER}>:-fsanitize=address>
        $<$<BOOL:${USE_SANITIZER}>:-fsanitize=undefined>
        )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<BOOL:${USE_SANITIZER}>:-fsanitize=address>
        $<$<BOOL:${USE_SANITIZER}>:-fsanitize=undefined>
        )

    if (${COPY_AFTER_BUILD})
        message(STATUS "Will copy plugin after every build")
        set(products_folder ${CMAKE_BINARY_DIR})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Installing ${products_folder}/${PROJECT_NAME}.clap to ~/Library/Audio/Plug-Ins/CLAP/"
            COMMAND ${CMAKE_COMMAND} -E make_directory "~/Library/Audio/Plug-Ins/CLAP"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${products_folder}/${PROJECT_NAME}.clap" "~/Library/Audio/Plug-Ins/CLAP/${PROJECT_NAME}.clap"
            )
    endif()
elseif(UNIX)
    target_sources(${PROJECT_NAME} PRIVATE src/linux-vstgui-adapter.cpp)
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".clap" PREFIX "")
    if (${COPY_AFTER_BUILD})
        message(STATUS "Will copy plugin after every build")
        set(products_folder ${CMAKE_BINARY_DIR})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Installing ${products_folder}/${PROJECT_NAME}.clap to ~/.clap"
            COMMAND ${CMAKE_COMMAND} -E make_directory "~/.clap"
            COMMAND ${CMAKE_COMMAND} -E copy "${products_folder}/${PROJECT_NAME}.clap" "~/.clap"
            )
    endif()

else()
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".clap" PREFIX "")
    # if (${COPY_AFTER_BUILD})
    #     message(STATUS "Will copy plugin after every build")
    #     set(products_folder ${CMAKE_BINARY_DIR})
    #     set(installation_folder "$ENV{ProgramW6432}\\Common\ Files\\CLAP\\")
    #     add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    #         COMMAND ${CMAKE_COMMAND} -E echo "Installing ${products_folder}\\${PROJECT_NAME}.clap to ${installation_folder}"
    #         COMMAND ${CMAKE_COMMAND} -E make_directory ${installation_folder}
    #         COMMAND ${CMAKE_COMMAND} -E copy "${products_folder}\\${PROJECT_NAME}.clap" "${installation_folder}"
    #         )
    # endif()
endif()