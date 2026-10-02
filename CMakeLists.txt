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
option(ENABLE_TSAN "Activar ThreadSanitizer (el cálculo corre en un std::jthread)" OFF)
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
        # Inicializa a cero todo lo que se declare sin valor. Elimina de raíz la
        # clase de fallos «usar una variable antes de inicializarla», que es de
        # donde salen los leaks de memoria y los discloses de información.
        -ftrivial-auto-var-init=zero
        -fPIE
        -fPIC)
    # Executables reubicables (Position Independent Executable): sin esto el
    # binario se enlaza en direcciones fijas y el ASLR no puede protegerlo.
    add_link_options(-pie)
    # _FORTIFY_SOURCE requiere optimización; no aplica en builds de depuración.
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-D_FORTIFY_SOURCE=3)
    endif()
    add_link_options(-Wl,-z,relro,-z,now)
    if(ENABLE_SANITIZERS)
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif()
    # La resolución numérica corre en un std::jthread mientras la interfaz sigue
    # viva, así que el checklist de modern-cpp pide TSan para código concurrente.
    # ASan y TSan son incompatibles entre sí: activarlos a la vez no funciona.
    if(ENABLE_TSAN)
        if(ENABLE_SANITIZERS)
            message(FATAL_ERROR "ENABLE_TSAN y ENABLE_SANITIZERS son mutuamente excluyentes")
        endif()
        add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
        add_link_options(-fsanitize=thread)
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
    src/core/Jacobi.cpp
    src/core/Metodo.cpp
    src/core/NewtonRaphson.cpp
    src/core/Resultado.cpp
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
    src/core/Resultado.cpp
    src/core/CifrasSignificativas.cpp
    src/ejercicios/Ejercicios.cpp)

registrar_prueba(metodo_tests
    tests/metodo_test.cpp
    src/core/Biseccion.cpp
    src/core/Jacobi.cpp
    src/core/Metodo.cpp
    src/core/NewtonRaphson.cpp
    src/core/Resultado.cpp
    src/core/CifrasSignificativas.cpp
    src/core/Funcion.cpp)

registrar_prueba(newton_tests
    tests/newton_test.cpp
    src/core/NewtonRaphson.cpp
    src/core/Resultado.cpp
    src/core/Funcion.cpp)

registrar_prueba(jacobi_tests
    tests/jacobi_test.cpp
    src/core/Jacobi.cpp
    src/core/Resultado.cpp)

# ---------------- Pruebas de GUI (offscreen) ----------------
# Dependen de Qt, así que van aparte de las de núcleo. Se ejecutan con
# QT_QPA_PLATFORM=offscreen para no necesitar una sesión gráfica.
option(BUILD_GUI_TESTS "Construir las pruebas de interfaz (offscreen)" ON)

if(BUILD_GUI_TESTS)
    qt_add_executable(gui_tests tests/gui_test.cpp)
    target_sources(gui_tests PRIVATE
        src/core/Biseccion.cpp
        src/core/Jacobi.cpp
        src/core/Metodo.cpp
        src/core/NewtonRaphson.cpp
        src/core/Resultado.cpp
        src/core/CifrasSignificativas.cpp
        src/core/Funcion.cpp
        src/ejercicios/Ejercicios.cpp
        src/gui/GraficoBiseccion.cpp
        src/gui/ModeloIteraciones.cpp
        src/gui/PanelEntrada.cpp
        src/gui/PanelProcedimiento.cpp
        src/gui/TablaIteraciones.cpp
        src/gui/VentanaPrincipal.cpp)
    target_include_directories(gui_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/qcustomplot)
    target_link_libraries(gui_tests PRIVATE qcustomplot Qt6::Widgets)
    add_test(NAME gui_tests COMMAND gui_tests)
    set_tests_properties(gui_tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    # Qt no está anotado para ThreadSanitizer: ve "carreras" donde dentro de Qt
    # sí hay sincronización (QThreadPool, QWaitCondition), y la mayoría de sus
    # símbolos llegan al informe sin nombre, así que no se pueden suprimir de
    # forma selectiva. Y suprimir por biblioteca tampoco vale: TSan casa el
    # patrón con cualquier frame del stack, y como Qt despacha todo por su
    # event loop, una regla `race:libQt6Core.so` también calla las carreras del
    # código de este proyecto (comprobado con un global escrito a la vez por el
    # hilo de cálculo y por el de la GUI: sin la regla salía, con ella no).
    #
    # Por eso bajo TSan se desactiva esta prueba en lugar de tapar el ruido:
    # quien quiera validar la concurrencia (el paso de resultado del std::jthread
    # a la interfaz) debe hacerlo sobre los símbolos de este proyecto, no sobre
    # los de Qt. Los cuatro tests de núcleo sí se validan con TSan: no enlazan Qt.
    if(ENABLE_TSAN)
        set_tests_properties(gui_tests PROPERTIES DISABLED TRUE)
    endif()
endif()