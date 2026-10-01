// CifrasSignificativas.cpp
// -----------------------------------------------------------------------------
// Implementación del formateo dinámico de decimales basado en cifras
// significativas (paso 5 del algoritmo de bisección).
// -----------------------------------------------------------------------------
#include "CifrasSignificativas.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace biseccion {

int cifrasDesdeEs(double Es_porcentaje) {
    if (!(Es_porcentaje > 0.0)) {
        return 6;  // valor por defecto seguro
    }
    const double n = 2.0 - std::log10(Es_porcentaje / 0.5);
    return std::clamp(static_cast<int>(std::llround(n)), 1, 12);
}

int decimalesParaCifras(double valor, int n) {
    n = std::clamp(n, 1, 15);
    if (valor == 0.0) {
        return std::max(0, n - 1);
    }
    const double absValor = std::fabs(valor);
    const int exponente10 = static_cast<int>(std::floor(std::log10(absValor)));
    // Para 'n' cifras significativas: decimales = n - 1 - floor(log10(|x|))
    return std::max(0, n - 1 - exponente10);
}

std::string formatearParaCifras(double valor, int n) {
    const int decimales = decimalesParaCifras(valor, n);
    return std::format("{:.{}f}", valor, decimales);
}

}  // namespace biseccion