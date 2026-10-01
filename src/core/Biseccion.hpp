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
#include <optional>
#include <stop_token>
#include <vector>

namespace biseccion {

enum class MotivoParada {
    Interrumpido,          // el hilo recibió una solicitud de cancelación
    RaizExacta,            // f(m) == 0 → la raíz es exactamente m
    ToleranciaAlcanzada,   // e_a < tolerancia (paso 4)
    MaxIteraciones,        // límite de iteraciones sin cumplir los criterios
};

enum class ErrorBiseccion {
    SinCambioDeSigno,      // f(a)·f(b) >= 0 → no se garantiza una raíz en [a, b]
};

struct Iteracion {
    int k = 0;
    double a = 0.0;
    double b = 0.0;
    double m = 0.0;
    double fa = 0.0;  // f(a)
    double fb = 0.0;  // f(b)
    double fm = 0.0;  // f(m)
    // Error relativo porcentual; NO existe en la primera iteración (paso 4).
    std::optional<double> eaPorcentaje;
};

struct Resultado {
    std::vector<Iteracion> iteraciones;
    MotivoParada motivo = MotivoParada::MaxIteraciones;

    [[nodiscard]] double raiz() const;
    [[nodiscard]] int iteracionesUsadas() const;
};

// Solventa el método en [a, b] con la tolerancia relativa indicada.
class Biseccion {
public:
    explicit Biseccion(std::function<double(double)> f);

    // Devuelve la tabla completa de iteraciones o un error tipado.
    [[nodiscard]] std::expected<Resultado, ErrorBiseccion>
    resolver(double a, double b,
             double toleranciaEsPorcentaje,
             int maxIteraciones = 120,
             std::stop_token detener = {}) const;

    // Criterio absoluto: iteraciones garantizadas para 'n' cifras exactas.
    //  (b - a)/2^k <= 0.5·10^(-n)  →  k = ceil(log2((b-a)/(0.5·10^(-n))))
    [[nodiscard]] static int iteracionesParaCifras(double a, double b, int n);

private:
    std::function<double(double)> f_;
};

}  // namespace biseccion