// Ejercicios.hpp
// -----------------------------------------------------------------------------
// Presets de los cuatro ejercicios aplicados que debe resolver la aplicación
// (sección 5 del enunciado). Se exponen como datos para la GUI y la consola.
// -----------------------------------------------------------------------------
#pragma once

#include <functional>
#include <string>
#include <vector>

namespace biseccion {

struct Ejercicio {
    std::string clave;                     // "e1" … "e4": identificación estable
    std::string nombre;                    // encabezado mostrado en la GUI
    std::string funcionTexto;              // expresión canónica de f(x)
    std::function<double(double)> f;       // evaluación en double
    double a = 0.0;                        // extremo inferior
    double b = 1.0;                        // extremo superior
    std::string descripcion;               // contexto de ingeniería

    // f'(x) del ejercicio. Newton-Raphson la exige siempre, así que se guarda
    // aquí: las mismas cuatro raíces sirven para validar ambos métodos.
    std::string derivadaTexto;
    std::function<double(double)> df;
};

// E1–E4 en orden de presentación.
[[nodiscard]] const std::vector<Ejercicio>& ejerciciosPredeterminados();

}  // namespace biseccion