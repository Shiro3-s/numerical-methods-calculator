// Biseccion.hpp
// -----------------------------------------------------------------------------
// Núcleo matemático: método de bisección siguiendo estrictamente los 5 pasos:
//   1. Verificación inicial  f(a)·f(b) < 0
//   2. Punto medio           m = (a + b) / 2
//   3. Signo de f(a)·f(m):   < 0 → mitad inferior (nuevo b = m)
//                            > 0 → mitad superior (nuevo a = m)
//                            == 0 → raíz exacta en m (fin)
//   4. Criterio de parada    e_a = |(m_actual − m_anterior)/m_actual|·100,
//                            parar si e_a < Tolerancia (sin e_a en iter. 1)
//   5. Cifras significativas (se resuelven en CifrasSignificativas.hpp)
// -----------------------------------------------------------------------------
#pragma once

#include <expected>
#include <functional>
#include <stop_token>
#include <vector>

#include "Metodo.hpp"

namespace biseccion {

// Solventa el método en [a, b] con la tolerancia relativa indicada.
class Biseccion : public MetodoNumerico {
public:
    explicit Biseccion(std::function<double(double)> f);

    [[nodiscard]] DescriptorMetodo descriptor() const override;

    // Interfaz uniforme con el resto de métodos.
    [[nodiscard]] std::expected<Resultado, ErrorMetodo>
    resolver(const Entrada& entrada, std::stop_token detener = {}) const override;

    // API directa (conservada para tests y uso interno).
    [[nodiscard]] std::expected<Resultado, ErrorMetodo>
    resolver(double a, double b,
             double toleranciaEsPorcentaje,
             int maxIteraciones = 120,
             std::stop_token detener = {}) const;

    // Criterio absoluto: iteraciones garantizadas para 'n' cifras exactas.
    //  (b - a)/2^k <= 0.5·10^(-n)  →  k = ceil(log2((b-a)/(0.5·10^(-n))))
    // Devuelve 0 si el intervalo ya es más estrecho que la tolerancia (o si
    // a == b) y está acotado a 10 000 para que un intervalo patológico no
    // derive un bucle interminable en la GUI.
    [[nodiscard]] static int iteracionesParaCifras(double a, double b, int n);

    [[nodiscard]] const std::function<double(double)>& funcion() const;

private:
    std::function<double(double)> f_;
};

}  // namespace biseccion