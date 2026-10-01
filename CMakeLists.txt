cmake_minimum_required(VERSION 3.21)

project(Biseccion
    VERSION 1.0.0
    DESCRIPTION "Aplicación interactiva del método de bisección (Qt 6)"
    LANGUAGES CXX)

# ---------------- Estándar C++23 (skill modern-cpp) ----------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_SANITIZERS "Activar ASan + UBSan para depuración" OFF)
option(STRICT_WARNINGS "Tratar las advertencias de compilación como errores" ON)

# ---------------- Endurecimiento del compilador (skill modern-cpp) ----------------
if(NOT MSVC)
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(STRICT_WARNINGS)
        add_compile_options(-Werror)
    endif()
    add_compile_options(
        -fstack-protector-strong
        -fstack-clash-protection
        -fPIC)
    # _FORTIFY_SOURCE requiere optimización; no aplica en builds de depuración.
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-D_FORTIFY_SOURCE=3)
    endif()
    add_link_options(-Wl,-z,relro,-z,now)
    if(ENABLE_SANITIZERS)
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

# ---------------- Qt 6 ----------------
find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets PrintSupport)
qt_standard_project_setup()

# ---------------- QCustomPlot vendoreado (GPL-3.0; ver third_party/qcustomplot/GPL.txt) ----------------
add_library(qcustomplot STATIC third_party/qcustomplot/qcustomplot.cpp)
target_include_directories(qcustomplot SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/qcustomplot)
target_compile_definitions(qcustomplot PUBLIC QCUSTOMPLOT_USE_LIBRARY)
target_link_libraries(qcustomplot PUBLIC Qt6::Widgets Qt6::PrintSupport)
# La biblioteca heredada no debe romper por las advertencias estrictas de nuestro código.
target_compile_options(qcustomplot PRIVATE -w)

# ---------------- Aplicación ----------------
qt_add_executable(Biseccion
    src/main.cpp
    src/core/Biseccion.cpp
    src/core/CifrasSignificativas.cpp
    src/core/Funcion.cpp
    src/ejercicios/Ejercicios.cpp
    src/gui/GraficoBiseccion.cpp
    src/gui/ModeloIteraciones.cpp
    src/gui/PanelEntrada.cpp
    src/gui/PanelProcedimiento.cpp
    src/gui/TablaIteraciones.cpp
    src/gui/VentanaPrincipal.cpp
)
target_include_directories(Biseccion PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# El include de QCustomPlot también debe alcanzar al código autogenerado por moc.
target_include_directories(Biseccion PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/qcustomplot)
target_link_libraries(Biseccion PRIVATE qcustomplot Qt6::Widgets)

# ---------------- Pruebas automáticas (CTest) ----------------
# El núcleo (Biseccion, CifrasSignificativas, Funcion, Ejercicios) no depende
# de Qt; los tests se compilan sin ella para ser rápidos y de propósito simple.
enable_testing()

function(registrar_prueba nombre)
    # Los argumentos extra (ARGN) son las fuentes del ejecutable de prueba.
    add_executable(${nombre} ${ARGN})
    target_include_directories(${nombre} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    # Sin clases QObject: no se necesita automoc.
    set_target_properties(${nombre} PROPERTIES AUTOMOC OFF)
    add_test(NAME ${nombre} COMMAND ${nombre})
endfunction()

registrar_prueba(parser_tests
    tests/parser_test.cpp
    src/core/Funcion.cpp)

registrar_prueba(biseccion_tests
    tests/biseccion_test.cpp
    src/core/Biseccion.cpp
    src/core/CifrasSignificativas.cpp
    src/ejercicios/Ejercicios.cpp)