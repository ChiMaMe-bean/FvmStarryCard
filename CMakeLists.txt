cmake_minimum_required(VERSION 3.16)

project(starrycard VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==== 设置OpenCV路径，OpenCV路径配置为本地local.cmake ====
if(EXISTS "${CMAKE_SOURCE_DIR}/local.cmake")
    include(${CMAKE_SOURCE_DIR}/local.cmake)
endif()
# ==== OpenCV路径请在local.cmake中配置 ====

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)
find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

set(TS_FILES starrycard_zh_CN.ts)

set(PROJECT_SOURCES
        main.cpp
        starrycard.cpp
        starrycard.h
        starrycard.ui
        ${TS_FILES}
        resources_background.qrc
        resources_bind_state.qrc
        resources_card.qrc
        resources_clover.qrc
        resources_gameImage.qrc
        resources_icons.qrc
        resources_level.qrc
        resources_position.qrc
        resources_recipe.qrc
        resources_spices.qrc
        reciperecognizer.cpp
        reciperecognizer.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(starrycard
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        custombutton.cpp
        custombutton.h
        utils.cpp
        utils.h
        cardrecognizer.cpp
        cardrecognizer.h
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
    ${OpenCV_LIBS}
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
