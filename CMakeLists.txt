cmake_minimum_required(VERSION 3.19)
project(spring_mass_damper_simulator LANGUAGES CXX)

set(CMAKE_PREFIX_PATH "C:/Qt/6.11.0/mingw_64/lib/cmake/Qt6")

find_package(Qt6 6 REQUIRED COMPONENTS Core Widgets Gui Qml LinguistTools Charts)

qt_standard_project_setup()

qt_add_executable(spring_mass_damper_simulator
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
    TARGETS spring_mass_damper_simulator
    TS_FILES spring_mass_damper_simulator_en_IN.ts
)

target_link_libraries(spring_mass_damper_simulator
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Gui
        Qt::Qml
        Qt::Charts
)

install(TARGETS spring_mass_damper_simulator
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET spring_mass_damper_simulator
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