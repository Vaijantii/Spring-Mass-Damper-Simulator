cmake_minimum_required(VERSION 3.19)
project(firstapp_new LANGUAGES CXX)

set(CMAKE_PREFIX_PATH "C:/Qt/6.11.0/mingw_64/lib/cmake/Qt6")

find_package(Qt6 6 REQUIRED COMPONENTS Core Widgets Gui Qml LinguistTools Charts)

qt_standard_project_setup()

qt_add_executable(firstapp
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    massitem.cpp
    massitem.h
    springitem.h
    springitem.cpp
    simulationscene.h
    simulationscene.cpp
    damperitem.h
    damperitem.cpp
    wallitem.h
    wallitem.cpp
    forcefunction.h
    forcedialog.h
    forcedialog.cpp
    systembuilder.h
)

qt_add_translations(
    TARGETS firstapp
    TS_FILES firstapp_en_IN.ts
)

target_link_libraries(firstapp
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Gui
        Qt::Qml
        Qt::Charts
)

install(TARGETS firstapp
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET firstapp
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

add_executable(qr_solver qr.cpp)
set_target_properties(qr_solver PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_executable(lu_solver lu.cpp)
set_target_properties(lu_solver PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_executable(sensitivity_solver sensitivity.cpp)

set_target_properties(lu_solver qr_solver sensitivity_solver PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")