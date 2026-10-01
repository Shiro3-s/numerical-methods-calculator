// Ejercicios.cpp
// -----------------------------------------------------------------------------
// Definición de los cuatro ejercicios de aplicación:
//   E1 · Control de algoritmo (equilibrio):  f(x) = x − cos(x),  [0, 1]
//   E2 · Tráfico de red (saturación):        f(x) = ln(x) − x + 2,  [3, 4]
//   E3 · Dispositivo IoT (cruce energético): f(x) = e^x − 5x,  [0, 1]
//   E4 · Procesamiento de señales:           f(x) = x·sin(x) − 1,  [0, 2]
// -----------------------------------------------------------------------------
#include "Ejercicios.hpp"

#include <cmath>

namespace biseccion {

const std::vector<Ejercicio>& ejerciciosPredeterminados() {
    static const std::vector<Ejercicio> presets = {
        {
            "Ejercicio 1 — Control de algoritmo (equilibrio)",
            "x - cos(x)",
            [](double x) { return x - std::cos(x); },
            0.0,
            1.0,
            "Se busca el punto de equilibrio de un algoritmo de control donde "
            "la salida iguala a la realimentación: f(x) = x − cos(x) = 0 en [0, 1].",
        },
        {
            "Ejercicio 2 — Tráfico de red (saturación)",
            "ln(x) - x + 2",
            [](double x) { return std::log(x) - x + 2.0; },
            3.0,
            4.0,
            "Se modela el nivel de saturación de una red con f(x) = ln(x) − x + 2 "
            "y se localiza el umbral crítico en [3, 4].",
        },
        {
            "Ejercicio 3 — Dispositivo IoT (cruce energético)",
            "exp(x) - 5*x",
            [](double x) { return std::exp(x) - 5.0 * x; },
            0.0,
            1.0,
            "Se determina el punto de cruce energético de un dispositivo IoT "
            "donde e^x = 5x, es decir f(x) = e^x − 5x en [0, 1].",
        },
        {
            "Ejercicio 4 — Procesamiento de señales (resonancia)",
            "x*sin(x) - 1",
            [](double x) { return x * std::sin(x) - 1.0; },
            0.0,
            2.0,
            "Se localiza la frecuencia de resonancia de un sistema de señales "
            "donde f(x) = x·sen(x) − 1 en [0, 2].",
        },
    };
    return presets;
}

}  // namespace biseccion