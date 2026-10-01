// biseccion_test.cpp
// -----------------------------------------------------------------------------
// Pruebas del método de bisección: raíces, conteos de iteraciones (criterios
// absoluto y relativo), ausencia de e_a en la iteración 1, error por falta de
// cambio de signo y formato dinámico de decimales. Sin dependencias de Qt.
// -----------------------------------------------------------------------------
#include <cmath>
#include <format>
#include <print>
#include <string>

#include "core/Biseccion.hpp"
#include "core/CifrasSignificativas.hpp"
#include "ejercicios/Ejercicios.hpp"

using namespace biseccion;

namespace {

int fallos = 0;

void comprobar(bool condicion, const std::string& mensaje) {
    if (!condicion) {
        std::println("FALLO: {}", mensaje);
        ++fallos;
    }
}

}  // namespace

int main() {
    // ---------- Criterio absoluto (previsible con ceil(log2)) ----------
    comprobar(Biseccion::iteracionesParaCifras(0.0, 1.0, 4) == 15, "E1 cifras exactas n=4");
    comprobar(Biseccion::iteracionesParaCifras(0.0, 1.0, 5) == 18, "E1 cifras exactas n=5");
    comprobar(Biseccion::iteracionesParaCifras(0.0, 1.0, 6) == 21, "E1 cifras exactas n=6");
    comprobar(Biseccion::iteracionesParaCifras(0.0, 2.0, 4) == 16, "E4 cifras exactas n=4");

    // ---------- Raíces de E1–E4 ----------
    const double raicesEsperadas[] = {
        0.7390851332151607, 3.1461932206205825,
        0.2591711018190738, 1.1141571408719302,
    };
    const int relativasEsperadas[] = {22, 20, 23, 22};  // n = 6, e_a < E_s
    const int absolutasEsperadas[] = {21, 21, 21, 22};  // n = 6, cifras exactas

    std::size_t indice = 0;
    for (const Ejercicio& ejercicio : ejerciciosPredeterminados()) {
        Biseccion biseccion(ejercicio.f);
        const double es = 0.5 * std::pow(10.0, 2.0 - 6);

        const auto alta = biseccion.resolver(ejercicio.a, ejercicio.b,
                                             0.5 * std::pow(10.0, 2.0 - 12), 200);
        comprobar(alta.has_value(), std::format("E{} debe resolverse", indice + 1));
        if (alta) {
            const double diferencia = std::fabs(alta->raiz() - raicesEsperadas[indice]);
            comprobar(diferencia < 1e-6,
                      std::format("E{} raíz: {} == {}", indice + 1, alta->raiz(),
                                  raicesEsperadas[indice]));
        }

        // Criterio relativo: cantidad de iteraciones hasta e_a < E_s.
        const auto relativo = biseccion.resolver(ejercicio.a, ejercicio.b, es, 200);
        comprobar(relativo.has_value(), std::format("E{} debe cumplir el criterio relativo", indice + 1));
        if (relativo) {
            comprobar(relativo->iteracionesUsadas() == relativasEsperadas[indice],
                      std::format("E{} iteraciones relativas n=6: {} == {}",
                                  indice + 1, relativo->iteracionesUsadas(),
                                  relativasEsperadas[indice]));
        }

        comprobar(Biseccion::iteracionesParaCifras(ejercicio.a, ejercicio.b, 6) ==
                      absolutasEsperadas[indice],
                  std::format("E{} iteraciones absolutas n=6", indice + 1));
        ++indice;
    }

    // ---------- e_a no existe en la primera iteración ----------
    {
        Biseccion biseccion([](double x) { return x - std::cos(x); });
        const auto resultado = biseccion.resolver(0.0, 1.0, 0.5 * std::pow(10.0, 2.0 - 6), 100);
        comprobar(resultado.has_value(), "resolver E1");
        if (resultado) {
            comprobar(!resultado->iteraciones.front().eaPorcentaje.has_value(),
                      "e_a debe estar ausente en la iteración 1");
            comprobar(resultado->iteraciones.size() > 1, "debe haber más de una iteración");
            if (resultado->iteraciones.size() > 1) {
                comprobar(resultado->iteraciones[1].eaPorcentaje.has_value(),
                          "e_a debe existir desde la iteración 2");
                comprobar(resultado->iteraciones[1].eaPorcentaje.value() > 0.0,
                          "e_a debe ser positivo");
            }
        }
    }

    // ---------- Sin cambio de signo → error tipado ----------
    {
        Biseccion biseccion([](double x) { return x * x + 1.0; });  // nunca cruza el cero
        const auto resultado = biseccion.resolver(-1.0, 1.0, 0.5 * std::pow(10.0, 2.0 - 6), 100);
        comprobar(!resultado.has_value(), "f(a)·f(b) >= 0 debe reportar error");
        if (!resultado) {
            comprobar(resultado.error() == ErrorBiseccion::SinCambioDeSigno,
                      "el error debe ser SinCambioDeSigno");
        }
    }

    // ---------- Cifras significativas y decimales dinámicos ----------
    comprobar(cifrasDesdeEs(0.5 * std::pow(10.0, 2.0 - 6)) == 6, "cifrasDesdeEs(0.0005) == 6");
    comprobar(decimalesParaCifras(0.7390851332151607, 6) == 6, "decimales de 0.739… n=6");
    comprobar(decimalesParaCifras(1000.0, 4) == 0, "decimales de 1000 n=4");
    comprobar(decimalesParaCifras(3.1461932206205825, 5) == 4, "decimales de 3.146… n=5");
    comprobar(formatearParaCifras(0.7390851332151607, 6) == "0.739085", "formato 6 cifras");
    comprobar(formatearParaCifras(12345.678, 6) == "12345.7", "formato 12345.678 n=6");
    comprobar(formatearParaCifras(1000.0, 4) == "1000", "formato 1000 n=4 sin basura");

    std::println("biseccion_test: {} fallos", fallos);
    return fallos == 0 ? 0 : 1;
}