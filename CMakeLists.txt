cmake_minimum_required(VERSION 3.16)

project(starrycard VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
