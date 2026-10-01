// CifrasSignificativas.hpp
// -----------------------------------------------------------------------------
// Lógica de cifras significativas y presentación dinámica de decimales.
// Paso 5 del algoritmo: E_s = 0.5·10^(2-n) %  →  'n' cifras significativas.
// Este módulo es independiente de Qt para poder usarse en consola y tests.
// -----------------------------------------------------------------------------
#pragma once

#include <string>

namespace biseccion {

// Despeja el número de cifras significativas 'n' a partir del error
//   E_s = 0.5·10^(2-n) %   (porcentaje).
[[nodiscard]] int cifrasDesdeEs(double Es_porcentaje);

// Devuelve la cantidad de decimales con la que debe presentarse 'valor' para
// mostrar exactamente 'n' cifras significativas (evita la basura decimal).
[[nodiscard]] int decimalesParaCifras(double valor, int n);

// Formatea 'valor' con los decimales justos para 'n' cifras significativas.
[[nodiscard]] std::string formatearParaCifras(double valor, int n);

}  // namespace biseccion