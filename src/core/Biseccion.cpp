// Biseccion.cpp
// -----------------------------------------------------------------------------
// Implementación del algoritmo con la estructura obligatoria de los 5 pasos.
// -----------------------------------------------------------------------------
#include "Biseccion.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace biseccion {

double Resultado::raiz() const {
    if (iteraciones.empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return iteraciones.back().m;
}

int Resultado::iteracionesUsadas() const {
    return static_cast<int>(iteraciones.size());
}

Biseccion::Biseccion(std::function<double(double)> f) : f_(std::move(f)) {}

int Biseccion::iteracionesParaCifras(double a, double b, int n) {
    n = std::clamp(n, 1, 15);
    const double ancho = std::fabs(b - a);
    const double toleranciaAbsoluta = 0.5 * std::pow(10.0, -n);
    return static_cast<int>(std::ceil(std::log2(ancho / toleranciaAbsoluta)));
}

std::expected<Resultado, ErrorBiseccion>
Biseccion::resolver(double a, double b, double toleranciaEsPorcentaje,
                    int maxIteraciones, std::stop_token detener) const {
    // ---------- Paso 1: verificación inicial -----------------------------
    // Si f(a)·f(b) no es negativo, no se garantiza una raíz en [a, b].
    const double faInicial = f_(a);
    if (!(faInicial * f_(b) < 0.0)) {
        return std::unexpected(ErrorBiseccion::SinCambioDeSigno);
    }

    Resultado resultado;
    double fa = faInicial;
    double mAnterior = std::numeric_limits<double>::quiet_NaN();

    for (int k = 1; k <= maxIteraciones; ++k) {
        if (detener.stop_requested()) {
            resultado.motivo = MotivoParada::Interrumpido;
            break;
        }

        // ---------- Paso 2: punto medio --------------------------------
        const double m = (a + b) / 2.0;
        const double fm = f_(m);

        Iteracion iteracion;
        iteracion.k = k;
        iteracion.a = a;
        iteracion.b = b;
        iteracion.m = m;
        iteracion.fa = fa;
        iteracion.fb = f_(b);  // f(b) evaluado cada vuelta
        iteracion.fm = fm;

        // ---------- Paso 4: error relativo porcentual -------------------
        // e_a = |(m_actual − m_anterior) / m_actual| · 100
        // NO se calcula en la primera iteración (no hay m anterior).
        if (k > 1) {
            const double ea = std::fabs((m - mAnterior) / m) * 100.0;
            iteracion.eaPorcentaje = ea;
            if (ea < toleranciaEsPorcentaje) {
                resultado.motivo = MotivoParada::ToleranciaAlcanzada;
            }
        }
        resultado.iteraciones.push_back(iteracion);
        if (resultado.motivo == MotivoParada::ToleranciaAlcanzada) {
            break;
        }

        // ---------- Paso 3: signo de f(a)·f(m) --------------------------
        // Se evalúa una única vez (sin redundancias).
        const double producto = fa * fm;
        if (producto == 0.0) {
            // 3c: raíz exacta en m; el algoritmo termina.
            resultado.motivo = MotivoParada::RaizExacta;
            break;
        }
        if (producto < 0.0) {
            // 3a: la raíz está en la mitad inferior → nuevo b = m.
            b = m;
        } else {
            // 3b: la raíz está en la mitad superior → nuevo a = m.
            a = m;
            fa = fm;
        }

        mAnterior = m;
    }

    return resultado;
}

}  // namespace biseccion