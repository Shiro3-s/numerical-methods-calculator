// NewtonRaphson.hpp
// -----------------------------------------------------------------------------
// Método de Newton-Raphson: x_{k+1} = x_k - f(x_k) / f'(x_k) partiendo de x0.
// Criterio de parada primario: error relativo porcentual e_a < E_s, por
// coherencia con el resto de la app. Se añaden guardas: f'(x_k) == 0 →
// MotivoParada::DerivadaNula; valores no finitos o explosivos →
// MotivoParada::Divergente; f(x_k) == 0 → RaizExacta.
// -----------------------------------------------------------------------------
#pragma once

#include <expected>
#include <functional>
#include <stop_token>

#include "Metodo.hpp"

namespace biseccion {

class NewtonRaphson : public MetodoNumerico {
public:
    NewtonRaphson(std::function<double(double)> f, std::function<double(double)> df);

    [[nodiscard]] DescriptorMetodo descriptor() const override;

    [[nodiscard]] std::expected<Resultado, ErrorMetodo>
    resolver(const Entrada& entrada, std::stop_token detener = {}) const override;

    // API directa para tests (usa los valores numéricos, no analiza cadenas).
    [[nodiscard]] std::expected<Resultado, ErrorMetodo>
    resolver(double x0,
             double toleranciaEsPorcentaje,
             int maxIteraciones = 120,
             std::stop_token detener = {}) const;

private:
    std::function<double(double)> f_;
    std::function<double(double)> df_;
};

}  // namespace biseccion