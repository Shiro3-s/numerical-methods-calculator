// newton_test.cpp
// -----------------------------------------------------------------------------
// Pruebas del método de Newton-Raphson: convergencia con e_a, DerivadaNula,
// RaizExacta en x0 y casos clásicos coincidentes con las raíces de bisección.
// -----------------------------------------------------------------------------
#include <cmath>
#include <print>
#include <string>

#include "core/NewtonRaphson.hpp"

using namespace biseccion;

namespace {

int fallos = 0;

void comprobar(bool condicion, const std::string& mensaje) {
    if (!condicion) {
        std::println("FALLO: {}", mensaje);
        ++fallos;
    }
}

inline double f1(double x) { return x - std::cos(x); }
inline double df1(double x) { return 1.0 + std::sin(x); }

inline double f2(double x) { return std::log(x) - x + 2.0; }
inline double df2(double x) { return (1.0 / x) - 1.0; }

inline double f3(double x) { return std::exp(x) - 5.0 * x; }
inline double df3(double x) { return std::exp(x) - 5.0; }

inline double f4(double x) { return x * std::sin(x) - 1.0; }
inline double df4(double x) { return std::sin(x) + x * std::cos(x); }

inline double fCuad(double x) { return x * x - 2.0; }
inline double dfCuad(double x) { return 2.0 * x; }

inline double fCub(double x) { return x * x * x - 2.0 * x + 2.0; }
inline double dfCub(double x) { return 3.0 * x * x - 2.0; }

inline double fDerNula(double x) { return x * x * x - 3.0 * x; }
inline double dfDerNula(double x) { return 3.0 * x * x - 3.0; }

}  // namespace

int main() {
    // x - cos(x), x0=1.0, n=6 → 5 iteraciones, raíz exacta conocida.
    {
        NewtonRaphson n(f1, df1);
        const double es6 = 0.5 * std::pow(10.0, 2.0 - 6);
        const auto r = n.resolver(1.0, es6, 200);
        comprobar(r.has_value(), "Newton debe resolver x-cos(x)");
        if (r) {
            comprobar(std::fabs(r->raiz() - 0.7390851332151607) < 1e-9,
                      "Raíz esperada para x-cos(x)");
            // Con n=6 esperamos ~5 iteraciones (criterio e_a)
            comprobar(r->iteracionesUsadas() == 5,
                      "Iteraciones esperadas con n=6 (x-cos(x))");
            comprobar(r->motivo == MotivoParada::ToleranciaAlcanzada ||
                      r->motivo == MotivoParada::RaizExacta,
                      "Debe alcanzar tolerancia o raíz exacta");
        }
    }

    // ln(x)-x+2, x0=3.5, n=6 → 5 iteraciones
    {
        NewtonRaphson n(f2, df2);
        const double es6 = 0.5 * std::pow(10.0, 2.0 - 6);
        const auto r = n.resolver(3.5, es6, 200);
        comprobar(r.has_value(), "Newton debe resolver ln(x)-x+2");
        if (r) {
            comprobar(std::fabs(r->raiz() - 3.1461932206205825) < 1e-9,
                      "Raíz esperada para ln(x)-x+2");
            comprobar(r->iteracionesUsadas() == 5,
                      "Iteraciones esperadas con n=6 (ln(x)-x+2)");
        }
    }

    // exp(x)-5x, x0=0.5, n=6 → 5 iteraciones
    {
        NewtonRaphson n(f3, df3);
        const double es6 = 0.5 * std::pow(10.0, 2.0 - 6);
        const auto r = n.resolver(0.5, es6, 200);
        comprobar(r.has_value(), "Newton debe resolver exp(x)-5x");
        if (r) {
            comprobar(std::fabs(r->raiz() - 0.2591711018190738) < 1e-9,
                      "Raíz esperada para exp(x)-5x");
            comprobar(r->iteracionesUsadas() == 5,
                      "Iteraciones esperadas con n=6 (exp(x)-5x)");
        }
    }

    // x*sin(x)-1, x0=1.0, n=6 → 4 iteraciones
    {
        NewtonRaphson n(f4, df4);
        const double es6 = 0.5 * std::pow(10.0, 2.0 - 6);
        const auto r = n.resolver(1.0, es6, 200);
        comprobar(r.has_value(), "Newton debe resolver x*sin(x)-1");
        if (r) {
            comprobar(std::fabs(r->raiz() - 1.1141571408719300) < 1e-9,
                      "Raíz esperada para x*sin(x)-1");
            comprobar(r->iteracionesUsadas() == 4,
                      "Iteraciones esperadas con n=6 (x*sin(x)-1)");
        }
    }

    // x^2-2, x0=1.0, n=6 → 6 iteraciones
    {
        NewtonRaphson n(fCuad, dfCuad);
        const double es6 = 0.5 * std::pow(10.0, 2.0 - 6);
        const auto r = n.resolver(1.0, es6, 200);
        comprobar(r.has_value(), "Newton debe resolver x^2-2");
        if (r) {
            comprobar(std::fabs(r->raiz() - std::sqrt(2.0)) < 1e-9,
                      "Raíz esperada para x^2-2");
            comprobar(r->iteracionesUsadas() == 6,
                      "Iteraciones esperadas con n=6 (x^2-2)");
        }
    }

    // x^3-2x+2, x0=-1.5, n=6 → 6 iteraciones
    {
        NewtonRaphson n(fCub, dfCub);
        const double es6 = 0.5 * std::pow(10.0, 2.0 - 6);
        const auto r = n.resolver(-1.5, es6, 200);
        comprobar(r.has_value(), "Newton debe resolver x^3-2x+2");
        if (r) {
            comprobar(std::fabs(r->raiz() - (-1.7692923542386314)) < 1e-9,
                      "Raíz esperada para x^3-2x+2");
            comprobar(r->iteracionesUsadas() == 6,
                      "Iteraciones esperadas con n=6 (x^3-2x+2)");
        }
    }

    // Derivada nula en x0=1 para f(x)=x^3-3x: f'(1)=0 → DerivadaNula
    {
        NewtonRaphson n(fDerNula, dfDerNula);
        const double es6 = 0.5 * std::pow(10.0, 2.0 - 6);
        const auto r = n.resolver(1.0, es6, 50);
        comprobar(r.has_value(), "Newton devuelve resultado aunque f'(x0)=0");
        if (r) {
            comprobar(r->motivo == MotivoParada::DerivadaNula,
                      "f'(1)=0 debe provocar DerivadaNula");
        }
    }

    // Raíz exacta en x0=0 para f(x)=x^3-3x: f(0)=0 → RaizExacta
    {
        NewtonRaphson n(fDerNula, dfDerNula);
        const double es6 = 0.5 * std::pow(10.0, 2.0 - 6);
        const auto r = n.resolver(0.0, es6, 50);
        comprobar(r.has_value(), "Newton devuelve resultado con raíz en x0");
        if (r) {
            comprobar(r->motivo == MotivoParada::RaizExacta,
                      "f(x0)=0 debe provocar RaizExacta");
            comprobar(std::fabs(r->raiz()) < 1e-12, "Raíz en x0 = 0");
        }
    }

    // e_a ausente en iteración 1, presente en iteración 2
    {
        NewtonRaphson n(f1, df1);
        const double es4 = 0.5 * std::pow(10.0, 2.0 - 4);
        const auto r = n.resolver(1.0, es4, 100);
        comprobar(r.has_value(), "Newton debe resolver con n=4");
        if (r && !r->iteraciones.empty()) {
            comprobar(!r->iteraciones.front().eaPorcentaje.has_value(),
                      "e_a no existe en la iteración 1 (Newton)");
            if (r->iteraciones.size() > 1) {
                comprobar(r->iteraciones[1].eaPorcentaje.has_value(),
                          "e_a existe desde la iteración 2 (Newton)");
            }
        }
    }

    std::println("newton_test: {} fallos", fallos);
    return fallos == 0 ? 0 : 1;
}