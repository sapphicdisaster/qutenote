# Only for Clang (especially useful for Android builds)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-ferror-limit=0)
endif()
cmake_minimum_required(VERSION 3.16)

project(QuteNote VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Use C++17 for non-Android builds (Android builds will use C++14 via target properties)
if(NOT ANDROID)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core Gui LinguistTools Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core Gui LinguistTools Concurrent)

set(TS_FILES QuteNote_en_NZ.ts)

# Define the color picker library
add_library(colorpicker
    colorpicker.cpp
    colorpicker.h
    colorpicker_touch.cpp
    colorpicker_touch.h
    huesatmap.cpp
    huesatmap.h
    huesatmapcache.cpp
    huesatmapcache.h
    touchinteraction.cpp
    touchinteraction.h
    physicsengine.cpp
    physicsengine.h
    resourcemanager.cpp
    resourcemanager.h
)
target_link_libraries(colorpicker PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_compile_definitions(colorpicker PRIVATE COLORPICKER_LIBRARY)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
    titlebarwidget.cpp
    titlebarwidget.h
    filebrowser.cpp
    filebrowser.h
    filebrowser_touch.cpp
    filebrowsertreewidget.cpp
    filebrowsertreewidget.h
    filebrowsertouchhandler.cpp
    filebrowsertouchhandler.h
    filebrowserdividerdelegate.cpp
    filebrowserdividerdelegate.h
        settingsviewtouchhandler.cpp
        settingsviewtouchhandler.h
        texteditor.cpp
        texteditor.h
        texteditortouchhandler.cpp
        texteditortouchhandler.h
        mainview.cpp
        mainview.h
        documentlist.cpp
        documentlist.h
        documentmodel.cpp
        documentmodel.h
        lazydocumentmodel.cpp
        lazydocumentmodel.h
        filewatcherguard.cpp
        filewatcherguard.h
        componentbase.cpp
        componentbase.h
        uiutils.cpp
        uiutils.h
        touchinteractionhandler.cpp
        touchinteractionhandler.h
        thememanager.cpp
        thememanager.h
        themepreview.cpp
        themepreview.h
        settingsview.cpp
        settingsview.h
        themesettingspage.cpp
        themesettingspage.h
    aboutdialog.cpp
    aboutdialog.h
        licensesettingspage.cpp
        licensesettingspage.h
        backupsettingspage.cpp
        backupsettingspage.h
        colorpickertouchhandler.cpp
        colorpickertouchhandler.h
        resources.qrc
        ${TS_FILES}
)

# Link the color picker library to the main executable
list(APPEND PROJECT_SOURCES $<TARGET_OBJECTS:colorpicker>)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(QuteNote
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )

    if(ANDROID)
        set_target_properties(QuteNote PROPERTIES
            QT_ANDROID_MIN_SDK_VERSION 23
            QT_ANDROID_PACKAGE_NAME "com.angel.qutenote"
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
            QT_ANDROID_EXTRA_LIBS "${ANDROID_NDK}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so"
        )
        
        # Fix duplicate classes issue
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/android/build.gradle.in")
            file(READ "${CMAKE_CURRENT_SOURCE_DIR}/android/build.gradle.in" BUILD_GRADLE_CONTENT)
            string(APPEND BUILD_GRADLE_CONTENT "\n\nandroid {\n    packagingOptions {\n        exclude 'META-INF/DEPENDENCIES'\n        exclude 'META-INF/LICENSE'\n        exclude 'META-INF/LICENSE.txt'\n        exclude 'META-INF/license.txt'\n        exclude 'META-INF/NOTICE'\n        exclude 'META-INF/NOTICE.txt'\n        exclude 'META-INF/notice.txt'\n        exclude 'META-INF/*.kotlin_module'\n        pickFirst 'lib/*/libc++_shared.so'\n    }\n}")
            file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/android/build.gradle.in" "${BUILD_GRADLE_CONTENT}")
        endif()
    endif()
# Define target properties for Android with Qt 6
    set_property(TARGET QuteNote APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
    set_property(TARGET QuteNote APPEND PROPERTY QT_ANDROID_VERSION_CODE 3)
    set_property(TARGET QuteNote APPEND PROPERTY QT_ANDROID_VERSION_NAME "1.0.0")
    set_property(TARGET QuteNote APPEND PROPERTY QT_ANDROID_TARGET_SDK_VERSION 35)

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(QuteNote SHARED
            ${PROJECT_SOURCES}
        )
        
        # Set C++17 for Android builds
        set_target_properties(QuteNote PROPERTIES 
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
        )
        
        # Note: Compiler warning flags removed as Android NDK Clang doesn't recognize them
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(QuteNote
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(QuteNote PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.QuteNote)
endif()
set_target_properties(QuteNote PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS QuteNote
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(QuteNote)
endif()
