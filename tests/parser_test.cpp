// parser_test.cpp
// -----------------------------------------------------------------------------
// Pruebas unitarias del analizador de expresiones (sin dependencias de Qt).
// Se ejecutan con CTest:  cmake --build build && ctest --test-dir build
// -----------------------------------------------------------------------------
#include <cmath>
#include <print>
#include <string>

#include "core/Funcion.hpp"

using namespace biseccion;

namespace {

int fallos = 0;

void probarValida(const std::string& expr, double x, double esperado, double tol = 1e-9) {
    auto resultado = parsearFuncion(expr);
    if (!resultado) {
        std::println("FALLO: '{}' no debería fallar: {}", expr, resultado.error().mensaje);
        ++fallos;
        return;
    }
    const double obtenido = resultado->f(x);
    if (std::fabs(obtenido - esperado) > tol) {
        std::println("FALLO: '{}'(x={}) = {} (esperado {})", expr, x, obtenido, esperado);
        ++fallos;
    }
}

void probarInvalida(const std::string& expr) {
    auto resultado = parsearFuncion(expr);
    if (resultado) {
        std::println("FALLO: '{}' debería ser inválida", expr);
        ++fallos;
    }
}

}  // namespace

int main() {
    // ---------- Ejercicios del enunciado (regresión) ----------
    probarValida("x - cos(x)", 0.75, 0.75 - std::cos(0.75));
    probarValida("ln(x) - x + 2", 3.5, std::log(3.5) - 3.5 + 2.0);
    probarValida("exp(x) - 5*x", 0.75, std::exp(0.75) - 5.0 * 0.75);
    probarValida("x*sin(x) - 1", 1.5, 1.5 * std::sin(1.5) - 1.0);

    // ---------- Precedencia y potencias ----------
    probarValida("x^2 - 3*x + 2", 1.5, 1.5 * 1.5 - 3.0 * 1.5 + 2.0);
    probarValida("-x^2", 2.0, -4.0);            // -(x²)
    probarValida("3x^2", 2.0, 12.0);            // 3·(x²)
    probarValida("x^2.5", 4.0, std::pow(4.0, 2.5));
    probarValida("x^2(x+1)", 2.0, 4.0 * 3.0);   // (x²)·(x+1)

    // ---------- Potencia con «**» (estilo Python, equivalente a «^») ----------
    probarValida("x**2", 2.0, 4.0);
    probarValida("2**x", 3.0, 8.0);           // 2 elevado a x
    probarValida("2**3", 0.0, 8.0);
    probarValida("x**2**3", 2.0, std::pow(2.0, 8.0));  // asociativa a la derecha
    probarValida("-2**x", 3.0, -8.0);         // -(2^x)
    probarValida("x^2**3", 2.0, std::pow(2.0, 8.0));   // «^» y «**» mezclados

    // ---------- Multiplicación implícita ----------
    probarValida("5x", 2.0, 10.0);
    probarValida("2.5x + 1", 2.0, 2.5 * 2.0 + 1.0);
    probarValida("2sin(x)", 0.5, 2.0 * std::sin(0.5));
    probarValida("x(x-1)", 2.0, 2.0 * 1.0);
    probarValida("(x-1)(x+2)", 3.0, 2.0 * 5.0);
    probarValida("x sin(x)", 0.3, 0.3 * std::sin(0.3));

    // ---------- Constantes ----------
    probarValida("2pi", 0.0, 2.0 * PI);
    probarValida("2π", 0.0, 2.0 * PI);
    probarValida("tau", 0.0, TAU);
    probarValida("τ", 0.0, TAU);
    probarValida("pi/2", 0.0, PI / 2.0);

    // ---------- Exponente científico (con mirilla) ----------
    probarValida("1e-3*x", 4.0, 1e-3 * 4.0);
    probarValida("2e3", 0.0, 2000.0);
    probarValida("2ex", 1.5, 2.0 * EULER * 1.5);  // 2·e·x, no "2e" inválido
    probarValida("e^2", 0.0, EULER * EULER);

    // ---------- Funciones nuevas ----------
    probarValida("asin(x)", 0.5, std::asin(0.5));
    probarValida("acos(x)", 0.5, std::acos(0.5));
    probarValida("atan(x)", 0.5, std::atan(0.5));
    probarValida("sinh(x)", 0.5, std::sinh(0.5));
    probarValida("cosh(x)", 0.5, std::cosh(0.5));
    probarValida("tanh(x)", 0.5, std::tanh(0.5));
    probarValida("abs(x - 1)", -2.0, 3.0);
    probarValida("floor(x)", 2.7, 2.0);
    probarValida("ceil(x)", 2.1, 3.0);
    probarValida("sign(x)", -3.0, -1.0);
    probarValida("sgn(x)", 3.0, 1.0);
    probarValida("log2(x)", 8.0, 3.0);
    probarValida("log10(x)", 100.0, 2.0);
    probarValida("log(x)", 100.0, 2.0);   // log = base 10
    probarValida("cbrt(x)", 27.0, 3.0);
    probarValida("sqrt(x)", 16.0, 4.0);

    // ---------- Compuestas e identidades ----------
    probarValida("sin(x)^2 + cos(x)^2", 0.7, 1.0, 1e-6);
    probarValida("e^x - 5*x", 0.75, std::exp(0.75) - 5.0 * 0.75);

    // ---------- Expresiones inválidas ----------
    probarInvalida("");
    probarInvalida("5x +");
    probarInvalida("sin(x");
    probarInvalida("x + )");
    probarInvalida("x^");
    probarInvalida("foo(x)");
    // «**» (estilo Python) ya no es inválido: es un operador de potencia válido,
    // equivalente a «^». La comprobación de validez correspondiente está arriba
    // en la sección de potencias (probarValida("2**x", 2.0, 4.0)).
    probarInvalida("x + *3");

    std::println("parser_test: {} fallos", fallos);
    return fallos == 0 ? 0 : 1;
}