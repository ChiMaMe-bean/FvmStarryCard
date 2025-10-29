cmake_minimum_required(VERSION 3.16)

project(starrycard VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 根据构建类型添加编译定义
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
    message(STATUS "Release build: QT_NO_DEBUG_OUTPUT enabled")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_definitions(-DDEBUG_BUILD)
    message(STATUS "Debug build: DEBUG_BUILD enabled")
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(TS_FILES localization/starrycard_zh_CN.ts)

# 设置源文件
set(CORE_SOURCES
    src/core/starrycard.cpp
    src/core/starrycard.h
    src/core/utils.cpp
    src/core/utils.h
    src/debug_resources.cpp
)

set(RECOGNITION_SOURCES
    src/recognition/cardrecognizer.cpp
    src/recognition/cardrecognizer.h
    src/recognition/reciperecognizer.cpp
    src/recognition/reciperecognizer.h
)

set(UI_SOURCES
    src/ui/custombutton.cpp
    src/ui/custombutton.h
    src/ui/starrycard.ui
)

set(RESOURCE_FILES
    resources/qrc/resources_background.qrc
    resources/qrc/resources_bind_state.qrc
    resources/qrc/resources_card.qrc
    resources/qrc/resources_clover.qrc
    resources/qrc/resources_gameImage.qrc
    resources/qrc/resources_icons.qrc
    resources/qrc/resources_level.qrc
    resources/qrc/resources_position.qrc
    resources/qrc/resources_recipe.qrc
    resources/qrc/resources_spices.qrc
    resources/qrc/resources_spicesShow.qrc
)

set(PROJECT_SOURCES
    src/main.cpp
    ${CORE_SOURCES}
    ${RECOGNITION_SOURCES}
    ${UI_SOURCES}
    ${RESOURCE_FILES}
    ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(starrycard
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET starrycard APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(starrycard SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(starrycard
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(starrycard PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.starrycard)
endif()
set_target_properties(starrycard PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS starrycard
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(starrycard)
endif()

# 创建发布包配置
if(WIN32)
    # 在所有构建类型时都创建发布包
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        # 设置发布包目录
        set(RELEASE_PACKAGE_DIR "${CMAKE_BINARY_DIR}/StarryCard_Release_Package")
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        # 设置发布包目录
        set(RELEASE_PACKAGE_DIR "${CMAKE_BINARY_DIR}/StarryCard_RelWithDebInfo_Package")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # 设置发布包目录
        set(RELEASE_PACKAGE_DIR "${CMAKE_BINARY_DIR}/StarryCard_Debug_Package")
    endif()
    
    # 创建发布包目录
    file(MAKE_DIRECTORY "${RELEASE_PACKAGE_DIR}")
    
    # 复制可执行文件
    add_custom_command(TARGET starrycard POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:starrycard>
        "${RELEASE_PACKAGE_DIR}/"
        COMMENT "复制可执行文件到发布包"
    )
    
    # 复制Qt DLL文件
    if(QT_VERSION_MAJOR EQUAL 6)
        find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)
        get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(QT_WINDEPLOYQT_EXECUTABLE ${QT_QMAKE_EXECUTABLE} PATH)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${QT_WINDEPLOYQT_EXECUTABLE})
        
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET starrycard POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E chdir "${RELEASE_PACKAGE_DIR}" 
                ${WINDEPLOYQT_EXECUTABLE} --force --no-translations --no-system-d3d-compiler --no-opengl-sw 
                "${RELEASE_PACKAGE_DIR}/starrycard.exe"
                COMMENT "使用windeployqt复制Qt依赖库"
            )
        endif()
    endif()
    
    # 复制MinGW运行时库（如果使用MinGW）
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        find_library(LIBGCC_S_SEH libgcc_s_seh-1.dll)
        find_library(LIBSTDCXX libstdc++-6.dll)
        find_library(LIBWINPTHREAD libwinpthread-1.dll)
        
        if(LIBGCC_S_SEH)
            add_custom_command(TARGET starrycard POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${LIBGCC_S_SEH}
                "${RELEASE_PACKAGE_DIR}/"
                COMMENT "复制libgcc_s_seh-1.dll"
            )
        endif()
        
        if(LIBSTDCXX)
            add_custom_command(TARGET starrycard POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${LIBSTDCXX}
                "${RELEASE_PACKAGE_DIR}/"
                COMMENT "复制libstdc++-6.dll"
            )
        endif()
        
        if(LIBWINPTHREAD)
            add_custom_command(TARGET starrycard POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${LIBWINPTHREAD}
                "${RELEASE_PACKAGE_DIR}/"
                COMMENT "复制libwinpthread-1.dll"
            )
        endif()
    endif()
    
    # 复制配置文件（如果存在）
    if(EXISTS "${CMAKE_BINARY_DIR}/custom_bg_config.json")
        add_custom_command(TARGET starrycard POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/custom_bg_config.json"
            "${RELEASE_PACKAGE_DIR}/"
            COMMENT "复制custom_bg_config.json"
        )
    endif()
    
    if(EXISTS "${CMAKE_BINARY_DIR}/enhancement_config.json")
        add_custom_command(TARGET starrycard POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/enhancement_config.json"
            "${RELEASE_PACKAGE_DIR}/"
            COMMENT "复制enhancement_config.json"
        )
    endif()
    
    if(EXISTS "${CMAKE_BINARY_DIR}/spice_config.json")
        add_custom_command(TARGET starrycard POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/spice_config.json"
            "${RELEASE_PACKAGE_DIR}/"
            COMMENT "复制spice_config.json"
        )
    endif()
    
    if(EXISTS "${CMAKE_BINARY_DIR}/production_statistics.json")
        add_custom_command(TARGET starrycard POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/production_statistics.json"
            "${RELEASE_PACKAGE_DIR}/"
            COMMENT "复制production_statistics.json"
        )
    endif()
    
    # 复制debug相关目录（在所有构建类型时都复制）
    if(TRUE)
        if(EXISTS "${CMAKE_BINARY_DIR}/debug_clover")
            add_custom_command(TARGET starrycard POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_BINARY_DIR}/debug_clover"
                "${RELEASE_PACKAGE_DIR}/debug_clover"
                COMMENT "复制debug_clover目录"
            )
        endif()
        
        if(EXISTS "${CMAKE_BINARY_DIR}/debug_recipe")
            add_custom_command(TARGET starrycard POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_BINARY_DIR}/debug_recipe"
                "${RELEASE_PACKAGE_DIR}/debug_recipe"
                COMMENT "复制debug_recipe目录"
            )
        endif()
        
        if(EXISTS "${CMAKE_BINARY_DIR}/debug_dynamic_recipe")
            add_custom_command(TARGET starrycard POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_BINARY_DIR}/debug_dynamic_recipe"
                "${RELEASE_PACKAGE_DIR}/debug_dynamic_recipe"
                COMMENT "复制debug_dynamic_recipe目录"
            )
        endif()
    endif()
    
    # 复制screenshots目录（如果存在）
    if(EXISTS "${CMAKE_BINARY_DIR}/screenshots")
        add_custom_command(TARGET starrycard POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/screenshots"
            "${RELEASE_PACKAGE_DIR}/screenshots"
            COMMENT "复制screenshots目录"
        )
    endif()
    
    message(STATUS "发布包将创建在: ${RELEASE_PACKAGE_DIR}")
endif()
