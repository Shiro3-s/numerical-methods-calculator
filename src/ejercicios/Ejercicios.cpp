// Ejercicios.cpp
// -----------------------------------------------------------------------------
// Definición de los cuatro ejercicios de aplicación:
//   E1 · Control de algoritmo (equilibrio):  f(x) = x − cos(x),  [0, 1]
//   E2 · Tráfico de red (saturación):        f(x) = ln(x) − x + 2,  [3, 4]
//   E3 · Dispositivo IoT (cruce energético): f(x) = e^x − 5x,  [0, 1]
//   E4 · Procesamiento de señales:           f(x) = x·sin(x) − 1,  [0, 2]
//
// Cada uno lleva también su derivada: con las mismas cuatro raíces se validan
// bisección y Newton-Raphson (--comparar, tests/biseccion_test.cpp,
// tests/newton_test.cpp).
//
// Los datos se inicializan con DESIGNATED INITIALIZERS a propósito. Con init
// posicional, intercambiar `a` y `b` (ambos `double`) o `descripcion` y
// `derivadaTexto` (ambos `std::string`) compila sin un solo aviso y deja los
// datos del enunciado silenciosamente cambiados por los de otro ejercicio.
// Con designadores, cada valor está etiquetado y reordenar campos no puede
// corromper nada. Es la regla de modern-cpp "Designated initializers en vez de
// init posicional de struct".
// -----------------------------------------------------------------------------
#include "Ejercicios.hpp"

#include <cmath>

namespace biseccion {

const std::vector<Ejercicio>& ejerciciosPredeterminados() {
    static const std::vector<Ejercicio> presets = {
        {
            .clave = "e1",
            .nombre = "Ejercicio 1 — Control de algoritmo (equilibrio)",
            .funcionTexto = "x - cos(x)",
            .f = [](double x) { return x - std::cos(x); },
            .a = 0.0,
            .b = 1.0,
            .descripcion =
                "Se busca el punto de equilibrio de un algoritmo de control donde "
                "la salida iguala a la realimentación: f(x) = x − cos(x) = 0 en [0, 1].",
            .derivadaTexto = "1 + sin(x)",
            .df = [](double x) { return 1.0 + std::sin(x); },
        },
        {
            .clave = "e2",
            .nombre = "Ejercicio 2 — Tráfico de red (saturación)",
            .funcionTexto = "ln(x) - x + 2",
            .f = [](double x) { return std::log(x) - x + 2.0; },
            .a = 3.0,
            .b = 4.0,
            .descripcion =
                "Se modela el nivel de saturación de una red con f(x) = ln(x) − x + 2 "
                "y se localiza el umbral crítico en [3, 4].",
            .derivadaTexto = "1/x - 1",
            .df = [](double x) { return (1.0 / x) - 1.0; },
        },
        {
            .clave = "e3",
            .nombre = "Ejercicio 3 — Dispositivo IoT (cruce energético)",
            .funcionTexto = "exp(x) - 5*x",
            .f = [](double x) { return std::exp(x) - 5.0 * x; },
            .a = 0.0,
            .b = 1.0,
            .descripcion =
                "Se determina el punto de cruce energético de un dispositivo IoT "
                "donde e^x = 5x, es decir f(x) = e^x − 5x en [0, 1].",
            .derivadaTexto = "exp(x) - 5",
            .df = [](double x) { return std::exp(x) - 5.0; },
        },
        {
            .clave = "e4",
            .nombre = "Ejercicio 4 — Procesamiento de señales (resonancia)",
            .funcionTexto = "x*sin(x) - 1",
            .f = [](double x) { return x * std::sin(x) - 1.0; },
            .a = 0.0,
            .b = 2.0,
            .descripcion =
                "Se localiza la frecuencia de resonancia de un sistema de señales "
                "donde f(x) = x·sen(x) − 1 en [0, 2].",
            .derivadaTexto = "sin(x) + x*cos(x)",
            .df = [](double x) { return std::sin(x) + x * std::cos(x); },
        },
    };
    return presets;
}

}  // namespace biseccion
