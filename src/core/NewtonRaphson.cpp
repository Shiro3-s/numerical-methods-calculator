// NewtonRaphson.cpp
// -----------------------------------------------------------------------------
// Implementación del método de Newton-Raphson con las guardas recomendadas.
// -----------------------------------------------------------------------------
#include "NewtonRaphson.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <utility>

namespace biseccion {

namespace {

constexpr double kCotaDivergente = 1e12;
constexpr double kEpsilonDerivada = 1e-18;

[[nodiscard]] inline bool casiCero(double v) {
    return std::fabs(v) <= kEpsilonDerivada;
}

}  // namespace

NewtonRaphson::NewtonRaphson(std::function<double(double)> f,
                             std::function<double(double)> df)
    : f_(std::move(f)), df_(std::move(df)) {}

DescriptorMetodo NewtonRaphson::descriptor() const {
    DescriptorMetodo d;
    d.clave = "newton_raphson";
    d.nombre = "Newton-Raphson";
    d.descripcion = "Método de Newton-Raphson: x_{k+1} = x_k - f(x_k)/f'(x_k)";
    d.tipo = TipoResolucion::RaizPuntoInicial;
    d.etiquetaAuxiliar = "f'(x)";
    d.requiereExpresionAuxiliar = true;
    d.etiquetaRaiz = "x";
    d.muestraGrafico = true;
    d.muestraTrazado = true;  // la recta tangente en x_k es la clave del método
    d.admiteEstimacionAbsoluta = false;
    d.columnas = {
        { "k", CampoIteracion::K },
        { "x", CampoIteracion::X },
        { "f(x)", CampoIteracion::Fx },
        { "f'(x)", CampoIteracion::Fdx },
        { "paso", CampoIteracion::Paso },
        { "e_a (%)", CampoIteracion::Ea },
    };
    return d;
}

std::expected<Resultado, ErrorMetodo>
NewtonRaphson::resolver(const Entrada& entrada, std::stop_token detener) const {
    const int n = std::clamp(entrada.cifras, 1, 12);
    const double es = 0.5 * std::pow(10.0, 2.0 - n);
    const int maxIter = std::max(150, n * 12);
    return resolver(entrada.x0, es, maxIter, detener);
}

std::expected<Resultado, ErrorMetodo>
NewtonRaphson::resolver(double x0,
                        double toleranciaEsPorcentaje,
                        int maxIteraciones,
                        std::stop_token detener) const {
    Resultado resultado;
    resultado.tipo = TipoResolucion::RaizPuntoInicial;
    double xAnterior = std::numeric_limits<double>::quiet_NaN();
    double x = x0;

    for (int k = 1; k <= maxIteraciones; ++k) {
        if (detener.stop_requested()) {
            resultado.motivo = MotivoParada::Interrumpido;
            break;
        }

        const double fx = f_(x);
        const double dfx = df_(x);

        Iteracion iteracion;
        iteracion.k = k;
        iteracion.x = x;
        iteracion.fx = fx;
        iteracion.fdx = dfx;
        iteracion.a = 0.0;  // no aplica
        iteracion.b = 0.0;
        iteracion.m = x;

        if (k > 1) {
            const double ea = std::fabs((x - xAnterior) / x) * 100.0;
            iteracion.eaPorcentaje = ea;
            if (ea < toleranciaEsPorcentaje) {
                resultado.motivo = MotivoParada::ToleranciaAlcanzada;
            }
        }

        resultado.iteraciones.push_back(iteracion);

        if (resultado.motivo == MotivoParada::ToleranciaAlcanzada) {
            break;
        }

        // Raíz exacta en x.
        if (fx == 0.0) {
            resultado.motivo = MotivoParada::RaizExacta;
            break;
        }

        // f'(x) nula o muy pequeña → no se puede calcular el paso.
        if (dfx == 0.0 || !std::isfinite(dfx) || casiCero(dfx)) {
            resultado.motivo = MotivoParada::DerivadaNula;
            break;
        }

        const double paso = fx / dfx;
        iteracion.paso = -paso;
        // Actualizar el paso en la iteración registrada (ya empujada: reemplazamos el último)
        if (!resultado.iteraciones.empty()) {
            resultado.iteraciones.back().paso = -paso;
        }

        const double xSiguiente = x - paso;

        // Guardas de divergencia: valores no finitos o explosivos.
        if (!std::isfinite(xSiguiente) || std::fabs(xSiguiente) > kCotaDivergente) {
            resultado.motivo = MotivoParada::Divergente;
            break;
        }

        xAnterior = x;
        x = xSiguiente;
    }

    return resultado;
}

}  // namespace biseccion