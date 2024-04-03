cmake_minimum_required(VERSION 3.5)

project(TradingCatClient VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network Charts)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Network Charts)

SET(COMMON_FILES
    ${CMAKE_SOURCE_DIR}/../../Common/Common/common.h
    ${CMAKE_SOURCE_DIR}/../../Common/Common/common.cpp
)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        types.h
        types.cpp
        mainwindow.h
        mainwindow.ui
        filter.h
        filter.cpp

        localconfig.h localconfig.cpp
        httpsquery.h httpsquery.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(TradingCatClient
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${COMMON_FILES}
        resurce.qrc
        logindialog.h logindialog.cpp logindialog.ui

        usermessage.h usermessage.cpp usermessage.ui
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET TraidingCatBotClient APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(TradingCatClient SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(TradingCatClient
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(TradingCatClient PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Charts
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Core
    idbfs.js
)

target_link_options(TradingCatClient PRIVATE "SHELL:-s FORCE_FILESYSTEM=1 ")
target_link_options(TradingCatClient PUBLIC -sASYNCIFY -Os -sFETCH -sMAXIMUM_MEMORY=512MB)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/../../Common)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER TradingCatClient)
endif()
set_target_properties(TradingCatClient PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS TradingCatClient
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(TradingCatClient)
endif()
