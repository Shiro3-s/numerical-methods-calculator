// jacobi_test.cpp
// -----------------------------------------------------------------------------
// Pruebas del método de Jacobi para sistemas lineales.
//
// Las tablas de referencia se calcularon por fuera (Python, aritmética de doble
// precisión, sin compartir código con la implementación) para que la prueba
// compare dos caminos distintos y no uno consigo mismo. Se documentan los
// valores medidos y el criterio de parada exacto que se usó:
//     E_s = 0.5 · 10^(2−n) %,  con n = 6  →  E_s = 5·10⁻⁵ %
//     e_a = ‖x⁽ᵏ⁾ − x⁽ᵏ⁻¹⁾‖_p / ‖x⁽ᵏ⁾‖_p · 100,  con p = 3 por defecto
//
// CONVENCIÓN: la fila k registra x⁽ᵏ⁾ y el error con el que se llegó a él. La
// tabla empieza en k = 0 con la iterada inicial —que no tiene error, porque no
// existe x⁽⁻¹⁾— y el primer error es el de k = 1, como en el enunciado.
// Sin dependencias de Qt.
// -----------------------------------------------------------------------------
#include <cmath>
#include <print>
#include <string>
#include <vector>

#include "core/Jacobi.hpp"
#include "core/Resultado.hpp"

using namespace biseccion;

namespace {

int fallos = 0;

void comprobar(bool condicion, const std::string& mensaje) {
    if (!condicion) {
        std::println("FALLO: {}", mensaje);
        ++fallos;
    }
}

void comprobarCercano(double obtenido, double esperado, double tolerancia,
                      const std::string& mensaje) {
    if (!(std::fabs(obtenido - esperado) <= tolerancia)) {
        std::println("FALLO: {}  (obtenido {:.15g}, esperado {:.15g})",
                     mensaje, obtenido, esperado);
        ++fallos;
    }
}

// E_s para n cifras significativas, el mismo criterio que usa la aplicación.
[[nodiscard]] double esPara(int n) {
    return 0.5 * std::pow(10.0, 2.0 - n);
}

// Sistema 3×3 dominante estricto del libro de texto:
//     4x₁ +  x₂ − x₃ = 1
//    −x₁ + 4x₂ − x₃ = 2
//     x₁ −  x₂ + 4x₃ = 2
// Solución exacta: (0.227272727272727, 0.712121212121212, 0.621212121212121).
[[nodiscard]] std::vector<std::vector<double>> matrizClasica() {
    return { { 4.0, 1.0, -1.0 }, { -1.0, 4.0, -1.0 }, { 1.0, -1.0, 4.0 } };
}

[[nodiscard]] std::vector<double> terminosClasicos() {
    return { 1.0, 2.0, 2.0 };
}

// Sistema del ENUNCIADO, escrito en el ORDEN en que aparece en el documento:
//      x +  y + 4z = 15      ← no dominante: |1| no supera |1| + |4| = 5
//    −2x + 4y +  z =  9      ← dominante:     |4| > |2| + |1| = 3
//     6x + 3y − 2z =  6      ← no dominante: |2| no supera |6| + |3| = 9
// Con solo la fila 2 dominante. Intercambiando las filas 1 y 3 quedan las tres
// dominantes, y la solución exacta es (1, 2, 3).
[[nodiscard]] std::vector<std::vector<double>> matrizEnunciado() {
    return { { 1.0, 1.0, 4.0 }, { -2.0, 4.0, 1.0 }, { 6.0, 3.0, -2.0 } };
}

[[nodiscard]] std::vector<double> terminosEnunciado() {
    return { 15.0, 9.0, 6.0 };
}

// La iterada inicial que da el enunciado: x⁰ = (0, 0, 1).
[[nodiscard]] std::vector<double> inicialEnunciado() {
    return { 0.0, 0.0, 1.0 };
}

}  // namespace

int main() {
    const Jacobi jacobi;
    const double es6 = esPara(6);

    // =====================================================================
    // El sistema del enunciado, extremo a extremo
    // =====================================================================

    // ---- Paso 1 del procedimiento: verificación de la convergencia ----
    {
        std::vector<std::vector<double>> matriz = matrizEnunciado();
        std::vector<double> terminos = terminosEnunciado();

        // En el orden en que está escrito, solo la fila 2 cumple la condición.
        comprobar(!Jacobi::esDiagonalmenteDominante(matriz),
                  "El sistema del enunciado NO viene dominante: hay que reordenar");
        const auto original = Jacobi::dominanciaPorFila(matriz);
        comprobar(original.size() == 3, "Se informa de las tres filas");
        comprobar(!original[0].dominante(), "Fila 1: |1| no supera 1 + 4");
        comprobar(original[1].dominante(), "Fila 2: |4| supera 2 + 1");
        comprobar(!original[2].dominante(), "Fila 3: |2| no supera 6 + 3");
        comprobarCercano(original[0].diagonal, 1.0, 0.0, "Fila 1, |a₁₁| = 1");
        comprobarCercano(original[0].sumaResto, 5.0, 0.0, "Fila 1, suma del resto = 5");
        comprobarCercano(original[1].diagonal, 4.0, 0.0, "Fila 2, |a₂₂| = 4");
        comprobarCercano(original[1].sumaResto, 3.0, 0.0, "Fila 2, suma del resto = 3");
        comprobarCercano(original[2].diagonal, 2.0, 0.0, "Fila 3, |a₃₃| = 2");
        comprobarCercano(original[2].sumaResto, 9.0, 0.0, "Fila 3, suma del resto = 9");

        const Reordenamiento reordenamiento = Jacobi::reordenarParaDominancia(matriz, terminos);
        comprobar(reordenamiento.aplicado, "Hubo que intercambiar filas");
        comprobar(reordenamiento.intercambios.size() == 1,
                  "Basta con UN intercambio para este sistema");
        if (reordenamiento.intercambios.size() == 1) {
            const auto& par = reordenamiento.intercambios[0];
            comprobar(par.first == 0 && par.second == 2,
                      "El intercambio es el de las filas 1 y 3");
        }

        // Las TRES filas del enunciado, ya dominantes, y el b movió con ellas:
        // sin mover el término independiente se resolvería otro sistema.
        comprobar(Jacobi::esDiagonalmenteDominante(matriz),
                  "Tras el intercambio las tres filas son dominantes");
        const std::vector<std::vector<double>> esperada = {
            { 6.0, 3.0, -2.0 }, { -2.0, 4.0, 1.0 }, { 1.0, 1.0, 4.0 },
        };
        const std::vector<double> ladosEsperados = { 6.0, 9.0, 15.0 };
        comprobar(matriz == esperada, "La matriz queda en el orden del enunciado");
        comprobar(terminos == ladosEsperados,
                  "El término independiente viaja con su ecuación");
    }

    // ---- Varios intercambios: no basta con una transposition -------------
    {
        // Cada fila es dominante solo en UNA columna concreta, y esas columnas
        // forman un 3-ciclo: la fila de x solo sirve en el puesto 2, la de y en el
        // 3 y la de z en el 1. Ninguna transposición única coloca a la vez las
        // tres, así que hacen falta dos intercambios.
        //
        //     [ 1  6  2 ]   |1| < 6+2    |6| > 1+2    |2| < 6+1
        //     [ 2  1  5 ]   |2| < 1+5    |1| < 2+5    |5| > 2+1
        //     [ 7  1  2 ]   |7| > 1+2    |1| < 7+2    |2| < 7+1
        std::vector<std::vector<double>> matriz = { { 1.0, 6.0, 2.0 },
                                                     { 2.0, 1.0, 5.0 },
                                                     { 7.0, 1.0, 2.0 } };
        std::vector<double> terminos = { 19.0, 19.0, 15.0 };
        comprobar(!Jacobi::esDiagonalmenteDominante(matriz),
                  "Tal como está escrito no es dominante: ninguna fila encaja en su sitio");

        const Reordenamiento reordenamiento = Jacobi::reordenarParaDominancia(matriz, terminos);
        comprobar(reordenamiento.aplicado, "Hubo que reordenar");
        comprobar(reordenamiento.intercambios.size() == 2,
                  "Un 3-ciclo necesita dos intercambios, no uno");
        comprobar(Jacobi::esDiagonalmenteDominante(matriz),
                  "Tras los dos intercambios las tres filas son dominantes");
        const std::vector<std::vector<double>> esperada = { { 7.0, 1.0, 2.0 },
                                                            { 1.0, 6.0, 2.0 },
                                                            { 2.0, 1.0, 5.0 } };
        const std::vector<double> ladosEsperados = { 15.0, 19.0, 19.0 };
        comprobar(matriz == esperada, "Cada fila acaba en el puesto donde es dominante");
        comprobar(terminos == ladosEsperados,
                  "Y cada una lleva su término independiente: b = (19, 19, 15) → (15, 19, 19)");

        // Y el sistema resuelto desde ahí da la solución entera que se escondía
        // detrás del 3-ciclo.
        const auto r = jacobi.resolver({ { 1.0, 6.0, 2.0 }, { 2.0, 1.0, 5.0 }, { 7.0, 1.0, 2.0 } },
                                       { 19.0, 19.0, 15.0 }, {}, es6, 150);
        comprobar(r.has_value(), "El sistema del 3-ciclo debe resolverse");
        if (r) {
            comprobar(r->dominanteEstricto, "Acaba dominante");
            comprobar(r->reordenamiento.intercambios.size() == 2, "Con dos intercambios");
            // La tolerancia de parada es 5·10⁻⁵ % sobre la norma, así que la
            // aproximación landing around 1e-7: lo que se comprueba es que converge
            // al entero, no que lo alcance exactamente.
            comprobarCercano(r->solucion()[0], 1.0, 1e-6, "x → 1");
            comprobarCercano(r->solucion()[1], 2.0, 1e-6, "y → 2");
            comprobarCercano(r->solucion()[2], 3.0, 1e-6, "z → 3");
        }
    }

    // ---- La iterada inicial NO se reordena con las ecuaciones -------------
    {
        std::vector<std::vector<double>> matriz = matrizEnunciado();
        std::vector<double> terminos = terminosEnunciado();
        std::vector<double> inicial = inicialEnunciado();
        Jacobi::reordenarParaDominancia(matriz, terminos);

        // El lugar i sigue despejando la variable i (su diagonal es a_ii), así que
        // x⁰ no se toca. Si también se moviera, (0,0,1) se leería como «la primera
        // ecuación arranca en 1», que no es lo que escribió el usuario.
        comprobar(inicial == inicialEnunciado(),
                  "x⁰ está indexado por variable: el reordenamiento no lo mueve");
    }

    // ---- La resolución completa del enunciado ----------------------------
    {
        const auto r =
            jacobi.resolver(matrizEnunciado(), terminosEnunciado(), inicialEnunciado(), es6,
                            100);
        comprobar(r.has_value(), "El sistema del enunciado debe resolverse");
        if (r) {
            comprobar(r->motivo == MotivoParada::ToleranciaAlcanzada,
                      "Debe parar por tolerancia alcanzada");
            comprobar(r->dimension() == 3, "El sistema tiene 3 ecuaciones");
            comprobar(r->normaP == 3, "El orden de la norma por defecto es p = 3");
            comprobar(r->dominanteEstricto,
                      "Tras el reordenamiento la matriz es dominante");
            comprobar(r->reordenamiento.aplicado, "Se informa del reordenamiento");

            // Se guardan las dos versiones para poder explicarlas.
            comprobar(r->matrizOriginal == matrizEnunciado(),
                      "Se guarda la matriz tal como la escribió el usuario");
            comprobar(r->terminosOriginal == terminosEnunciado(),
                      "Se guardan los términos originales");
            comprobar(r->dominanciaOriginal.size() == 3 && !r->dominanciaOriginal[0].dominante() &&
                          r->dominanciaOriginal[1].dominante() &&
                          !r->dominanciaOriginal[2].dominante(),
                      "Se informa de la dominancia en el orden ORIGINAL");
            comprobar(r->dominanciaFinal.size() == 3 && r->dominanciaFinal[0].dominante() &&
                          r->dominanciaFinal[1].dominante() &&
                          r->dominanciaFinal[2].dominante(),
                      "Se informa de la dominancia en el orden FINAL");
            comprobar(r->matriz[0] == std::vector<double>({ 6.0, 3.0, -2.0 }),
                      "La matriz iterada es la reordenada");

            // ---- Paso 3: la tabla empieza en k = 0 con la iterada inicial ----
            comprobar(r->iteracionesUsadas() == 26,
                      "Debe registrar 26 filas: k = 0 (iterada inicial) + 25 pasos");
            if (r->iteraciones.size() >= 3) {
                const Iteracion& k0 = r->iteraciones[0];
                comprobar(k0.k == 0, "La primera fila es k = 0");
                comprobarCercano(k0.vectorX[0], 0.0, 0.0, "x⁰ = 0");
                comprobarCercano(k0.vectorX[1], 0.0, 0.0, "y⁰ = 0");
                comprobarCercano(k0.vectorX[2], 1.0, 0.0, "z⁰ = 1");
                comprobar(!k0.norma.has_value(),
                          "La iterada inicial no tiene norma: no hay vector anterior");
                comprobar(!k0.eaPorcentaje.has_value(),
                          "La iterada inicial no tiene e_a: no hay vector anterior");
                comprobar(k0.errorAbsoluto.empty(),
                          "La iterada inicial no tiene errores absolutos");
                comprobar(k0.errorRelativo.empty(),
                          "La iterada inicial no tiene errores relativos");

                // ---- K = 1: el primer paso, y el 3.147019 del enunciado ----
                const Iteracion& k1 = r->iteraciones[1];
                comprobar(k1.k == 1, "La segunda fila es k = 1");
                comprobarCercano(k1.vectorX[0], 4.0 / 3.0, 1e-15, "x¹ = (6 − 3y⁰ + 2z⁰)/6 = 4/3");
                comprobarCercano(k1.vectorX[1], 2.0, 1e-15, "y¹ = (9 + 2x⁰ − z⁰)/4 = 2");
                comprobarCercano(k1.vectorX[2], 15.0 / 4.0, 1e-15, "z¹ = (15 − x⁰ − y⁰)/4 = 15/4");

                // El número que da el enunciado con «3 NORMA P=3» en K=1:
                // ‖Δx‖₃ = ( (4/3)³ + 2³ + (11/4)³ )^(1/3) = 3.147019786
                comprobar(k1.norma.has_value(), "En k = 1 ya hay norma");
                if (k1.norma) {
                    comprobarCercano(*k1.norma, 3.1470197855036, 1e-12,
                                     "‖Δx‖₃ en K = 1 (el enunciado dice 3.147019)");
                }
                if (k1.eaPorcentaje) {
                    comprobarCercano(*k1.eaPorcentaje, 79.0458002324419, 1e-10,
                                     "e_a en K = 1 = ‖Δx‖₃ / ‖x¹‖₃ · 100");
                }

                // ---- Paso 4: los otros dos errores del enunciado ----
                comprobar(k1.errorAbsoluto.size() == 3,
                          "El error absoluto va por componente");
                comprobar(k1.errorRelativo.size() == 3,
                          "El error relativo va por componente");
                if (k1.errorAbsoluto.size() == 3 && k1.errorRelativo.size() == 3) {
                    comprobarCercano(k1.errorAbsoluto[0], 4.0 / 3.0, 1e-15, "|x₁¹ − x₁⁰|");
                    comprobarCercano(k1.errorAbsoluto[1], 2.0, 1e-15, "|x₂¹ − x₂⁰|");
                    comprobarCercano(k1.errorAbsoluto[2], 11.0 / 4.0, 1e-15, "|x₃¹ − x₃⁰| = 11/4");
                    // |Δ| / |x¹| · 100. Las dos primeras dan 100 % porque x⁰ y x¹
                    // tienen el mismo signo; la tercera, 2.75/3.75.
                    comprobarCercano(k1.errorRelativo[0], 100.0, 1e-12, "relativo de x = 100 %");
                    comprobarCercano(k1.errorRelativo[1], 100.0, 1e-12, "relativo de y = 100 %");
                    comprobarCercano(k1.errorRelativo[2], 73.3333333333333, 1e-11,
                                     "relativo de z = 73.3333… %");
                }

                // ---- K = 2: el paso siguiente ----
                const Iteracion& k2 = r->iteraciones[2];
                comprobar(k2.k == 2, "La tercera fila es k = 2");
                comprobarCercano(k2.vectorX[0], 1.25, 1e-15,
                                 "x² = (6 − 3·2 + 2·15/4)/6 = 1.25");
                comprobarCercano(k2.vectorX[1], 95.0 / 48.0, 1e-15,
                                 "y² = (9 + 2·4/3 − 15/4)/4");
                comprobarCercano(k2.vectorX[2], 35.0 / 12.0, 1e-15,
                                 "z² = (15 − 4/3 − 2)/4");
                if (k2.norma) {
                    comprobarCercano(*k2.norma, 0.833615355934025, 1e-12, "‖Δx‖₃ en K = 2");
                }
                if (k2.eaPorcentaje) {
                    comprobarCercano(*k2.eaPorcentaje, 25.6026745192605, 1e-10,
                                     "e_a en K = 2");
                }
            }

            // El último punto de la tabla es la aproximación final.
            const std::vector<double>& x = r->solucion();
            comprobarCercano(x[0], 1.000000566396, 1e-11, "x debe converger a 1");
            comprobarCercano(x[1], 2.00000017478829, 1e-11, "y debe converger a 2");
            comprobarCercano(x[2], 3.00000002324276, 1e-11, "z debe converger a 3");

            // ---- Fórmula de solución por Cramer ----
            comprobar(r->formulaSolucion.has_value(),
                      "El enunciado tiene solución única: debe haber fórmula");
            if (r->formulaSolucion) {
                const FormulaSolucion& f = *r->formulaSolucion;
                comprobarCercano(f.determinante, 129.0, 1e-9, "D = det(A) = 129");
                comprobar(f.determinantes.size() == 3, "Un determinante por variable");
                if (f.determinantes.size() == 3) {
                    comprobarCercano(f.determinantes[0], 129.0, 1e-9, "Dx = 129");
                    comprobarCercano(f.determinantes[1], 258.0, 1e-9, "Dy = 258");
                    comprobarCercano(f.determinantes[2], 387.0, 1e-9, "Dz = 387");
                }
                comprobar(f.valores.size() == 3, "La fórmula da un valor por variable");
                if (f.valores.size() == 3) {
                    comprobarCercano(f.valores[0], 1.0, 1e-12, "Dx/D = 1");
                    comprobarCercano(f.valores[1], 2.0, 1e-12, "Dy/D = 2");
                    comprobarCercano(f.valores[2], 3.0, 1e-12, "Dz/D = 3");
                    // La aproximación de Jacobi debe acercarse a la solución exacta,
                    // que es justo para lo que sirve la fórmula.
                    for (std::size_t i = 0; i < 3; ++i) {
                        comprobarCercano(x[i], f.valores[i], 1e-6,
                                         "La aproximación debe acercarse a la solución exacta");
                    }
                }
            }
        }
    }

    // ---- Cambiar p cambia el error, pero no la solución ------------------
    {
        const auto p1 = jacobi.resolver(matrizEnunciado(), terminosEnunciado(),
                                        inicialEnunciado(), es6, 200, {}, 1);
        const auto p3 = jacobi.resolver(matrizEnunciado(), terminosEnunciado(),
                                        inicialEnunciado(), es6, 200, {}, 3);
        const auto p8 = jacobi.resolver(matrizEnunciado(), terminosEnunciado(),
                                        inicialEnunciado(), es6, 200, {}, 8);
        comprobar(p1.has_value() && p3.has_value() && p8.has_value(),
                  "Las tres normas deben resolverse");
        if (p1 && p3 && p8) {
            comprobar(p1->normaP == 1 && p3->normaP == 3 && p8->normaP == 8,
                      "Cada resultado guarda el orden con el que se midió su error");
            // Un orden mayor reparte más el peso entre las componentes grandes, así
            // que el error de la primera iteración baja al subir p.
            if (p1->iteraciones.size() > 1 && p3->iteraciones.size() > 1 &&
                p8->iteraciones.size() > 1) {
                const double e1 = *p1->iteraciones[1].norma;
                const double e3 = *p3->iteraciones[1].norma;
                const double e8 = *p8->iteraciones[1].norma;
                comprobar(e1 > e3 && e3 > e8,
                          "Con el mismo Δ, ‖Δ‖ₚ baja al subir el orden p");
                // ‖Δ‖₁ = 4/3 + 2 + 11/4 = 6.083333…
                comprobarCercano(e1, 6.08333333333333, 1e-12, "‖Δ‖₁ del primer paso");
            }
            // Y las tres convergen al mismo sitio.
            comprobarCercano(p1->solucion()[0], p3->solucion()[0], 1e-6,
                             "La solución no depende del orden de la norma");
            comprobarCercano(p8->solucion()[2], p3->solucion()[2], 1e-6,
                             "La solución no depende del orden de la norma");
        }
    }

    // ---- La norma de orden p, aislada ------------------------------------
    {
        comprobarCercano(Jacobi::norma({ 3.0, 4.0 }, 2), 5.0, 1e-15, "‖(3,4)‖₂ = 5");
        comprobarCercano(Jacobi::norma({ -1.0, 1.0 }, 1), 2.0, 1e-15, "‖(−1,1)‖₁ = 2");
        comprobarCercano(Jacobi::norma({ 1.0, 1.0, 1.0 }, 3), std::cbrt(3.0), 1e-15,
                         "‖(1,1,1)‖₃ = ³√3");
        comprobarCercano(Jacobi::norma({}, 3), 0.0, 0.0, "El vector vacío tiene norma 0");
        comprobarCercano(Jacobi::norma({ 2.0, 2.0 }, 1), 4.0, 1e-15,
                         "Un p < 1 se trata como p = 1");
        comprobarCercano(Jacobi::norma({ -2.0 }, 7), 2.0, 1e-15,
                         "‖(−2)‖₇ = 2");
    }

    // =====================================================================
    // El sistema clásico 3×3 (ya dominante, sin reordenamiento)
    // =====================================================================
    {
        const auto r = jacobi.resolver(matrizClasica(), terminosClasicos(), {}, es6, 150);
        comprobar(r.has_value(), "El sistema de referencia debe resolverse");
        if (r) {
            comprobar(r->motivo == MotivoParada::ToleranciaAlcanzada,
                      "Debe parar por tolerancia alcanzada");
            comprobar(r->tipo == TipoResolucion::Sistema,
                      "El resultado debe autodescribirse como sistema");
            comprobar(r->dominanteEstricto,
                      "La matriz de referencia es diagonalmente dominante");
            comprobar(!r->reordenamiento.aplicado,
                      "Ya venía dominante: no hay nada que reordenar");
            comprobar(r->reordenamiento.intercambios.empty(),
                      "Y no debe registrar ningún intercambio");
            comprobar(r->dimension() == 3, "El sistema tiene 3 ecuaciones");

            const std::vector<double>& x = r->solucion();
            comprobarCercano(x[0], 0.227272756397724, 1e-12, "x₁ debe converger a la solución");
            comprobarCercano(x[1], 0.712121147662401, 1e-12, "x₂ debe converger a la solución");
            comprobarCercano(x[2], 0.621212180703878, 1e-12, "x₃ debe converger a la solución");

            // Y debe estar cerca de la solución EXACTA, no solo de la referencia.
            comprobarCercano(x[0], 0.227272727272727, 1e-6,
                             "x₁ debe acercarse a la solución exacta");
            comprobarCercano(x[1], 0.712121212121212, 1e-6,
                             "x₂ debe acercarse a la solución exacta");
            comprobarCercano(x[2], 0.621212121212121, 1e-6,
                             "x₃ debe acercarse a la solución exacta");

            // Y de la fórmula de Cramer: D = 66, Dx = 15, Dy = 47, Dz = 41.
            comprobar(r->formulaSolucion.has_value(), "Debe haber fórmula de solución");
            if (r->formulaSolucion) {
                comprobarCercano(r->formulaSolucion->determinante, 66.0, 1e-9, "D = 66");
                if (r->formulaSolucion->determinantes.size() == 3) {
                    comprobarCercano(r->formulaSolucion->determinantes[0], 15.0, 1e-9, "Dx = 15");
                    comprobarCercano(r->formulaSolucion->determinantes[1], 47.0, 1e-9, "Dy = 47");
                    comprobarCercano(r->formulaSolucion->determinantes[2], 41.0, 1e-9, "Dz = 41");
                }
            }
        }
    }

    // ---------- La tabla reproduce la referencia, ahora desde k = 0 -------
    {
        const auto r = jacobi.resolver(matrizClasica(), terminosClasicos(), {}, es6, 150);
        comprobar(r.has_value(), "Segunda resolución para la tabla");
        if (r && r->iteraciones.size() >= 4) {
            const Iteracion& k0 = r->iteraciones[0];
            comprobar(k0.k == 0, "La tabla empieza en k = 0");
            comprobarCercano(k0.vectorX[0], 0.0, 0.0, "x⁽⁰⁾₁ es 0");

            // k=1 es el primer paso calculado desde x⁽⁰⁾: x₁=b₁/a₁₁=1/4, x₂=1/2.
            const Iteracion& k1 = r->iteraciones[1];
            comprobar(k1.k == 1, "La segunda fila es k = 1");
            comprobarCercano(k1.vectorX[0], 0.25, 1e-15, "primer paso: x₁ = b₁/a₁₁ = 0.25");
            comprobarCercano(k1.vectorX[1], 0.50, 1e-15, "primer paso: x₂ = b₂/a₂₂ = 0.5");
            comprobarCercano(k1.vectorX[2], 0.50, 1e-15, "primer paso: x₃ = b₃/a₃₃ = 0.5");
            comprobar(k1.norma.has_value() && k1.eaPorcentaje.has_value(),
                      "Desde k=1 hay norma y e_a");
            if (k1.norma && k1.eaPorcentaje) {
                // ‖(0.25, 0.5, 0.5)‖₃ = (0.015625 + 0.125 + 0.125)^(1/3)
                comprobarCercano(*k1.norma, 0.642820397664559, 1e-12, "‖Δx‖₃ del primer paso");
                comprobarCercano(*k1.eaPorcentaje, 100.0, 1e-12,
                                 "e_a del primer paso = 100 %");
            }

            // k=2: cada componente usa SOLO la iterada anterior (x⁽¹⁾), no la que
            // su vecina acaba de calcular. Es lo que distingue a Jacobi de
            // Gauss-Seidel, así que el valor concreto lo fija la prueba.
            const Iteracion& k2 = r->iteraciones[2];
            comprobarCercano(k2.vectorX[0], 0.25, 1e-15, "k=2: x₁ = (1−0.5+0.5)/4 = 0.25");
            comprobarCercano(k2.vectorX[1], 0.6875, 1e-15, "k=2: x₂ = (2+0.25−0.5)/4 = 0.6875");
            comprobarCercano(k2.vectorX[2], 0.5625, 1e-15, "k=2: x₃ = (2−0.25+0.5)/4 = 0.5625");
            if (k2.norma) {
                comprobarCercano(*k2.norma, 0.189786810742229, 1e-12, "‖Δx‖₃ en K = 2");
            }
            if (k2.eaPorcentaje) {
                comprobarCercano(*k2.eaPorcentaje, 23.622970429084, 1e-10, "e_a en K = 2");
            }

            const Iteracion& k3 = r->iteraciones[3];
            if (k3.norma) {
                comprobarCercano(*k3.norma, 0.0515926132639785, 1e-12, "‖Δx‖₃ en K = 3");
            }
            if (k3.eaPorcentaje) {
                comprobarCercano(*k3.eaPorcentaje, 6.17105264742109, 1e-10, "e_a en K = 3");
            }
        }
    }

    // ---------- e_a monótona mientras la convergencia es limpia -----------
    {
        const auto r = jacobi.resolver(matrizClasica(), terminosClasicos(), {}, es6, 150);
        comprobar(r.has_value(), "Resolución para la monotonía");
        if (r) {
            bool decreciente = true;
            for (std::size_t i = 1; i < r->iteraciones.size(); ++i) {
                const auto& actual = r->iteraciones[i].eaPorcentaje;
                const auto& previa = r->iteraciones[i - 1].eaPorcentaje;
                if (actual && previa && *actual > *previa) {
                    decreciente = false;
                }
            }
            comprobar(decreciente,
                      "En un sistema dominante e_a debe decrecer monótonamente");
        }
    }

    // ---------- Dominancia diagonal: informa, no bloquea ----------------
    {
        const auto dominante = jacobi.resolver(matrizClasica(), terminosClasicos(), {}, es6, 150);
        comprobar(dominante.has_value() && dominante->dominanteEstricto,
                  "El sistema clásico debe detectarse como dominante");

        // NO dominante y NO reordenable a dominante, pero CONVERGENTE:
        // |1| < |2| en la primera fila y ningún intercambio lo arregla.
        // Si el método exigiera la dominante, esto se rechazaría y sería un fallo
        // real: la condición es suficiente, no necesaria.
        const auto noDominante =
            jacobi.resolver({ { 1.0, 2.0 }, { 0.2, 1.0 } }, { 1.0, 2.0 }, {}, es6, 200);
        comprobar(noDominante.has_value(),
                  "Un sistema no dominante debe resolverse igualmente");
        if (noDominante) {
            comprobar(!noDominante->dominanteEstricto,
                      "Este sistema NO es diagonalmente dominante");
            comprobar(noDominante->motivo == MotivoParada::ToleranciaAlcanzada,
                      "Aun así, Jacobi converge en él");
            comprobar(noDominante->iteracionesUsadas() == 32,
                      "Converge en 32 filas (iterada inicial + 31 pasos)");
            comprobarCercano(noDominante->solucion()[0], -5.0, 1e-5, "x₁ → −5");
            comprobarCercano(noDominante->solucion()[1], 3.0, 1e-5, "x₂ → 3");
        }

        // La comprobación directa sobre la matriz.
        comprobar(Jacobi::esDiagonalmenteDominante(matrizClasica()),
                  "esDiagonalmenteDominante debe aceptar el sistema clásico");
        comprobar(!Jacobi::esDiagonalmenteDominante({ { 1.0, 2.0 }, { 0.2, 1.0 } }),
                  "esDiagonalmenteDominante debe rechazar [1 2; 0.2 1]");
        comprobar(!Jacobi::esDiagonalmenteDominante({ { 2.0, 2.0 }, { 1.0, 2.0 } }),
                  "Una igualdad (|a_ii| = suma) NO es dominante estricta");
        comprobar(!Jacobi::esDiagonalmenteDominante({ { 3.0, 1.0 } }),
                  "Una matriz no cuadrada no es dominante");
        comprobar(!Jacobi::esDiagonalmenteDominante({}),
                  "Una matriz vacía no es dominante");
    }

    // ---------- El reordenamiento REPARA una diagonal nula ----------------
    {
        // a₁₁ = 0 hace Jacobi imposible tal como está escrito, pero
        // intercambiando las ecuaciones la diagonal deja de anularse y el sistema
        // resuelve. Es el caso para el que el enunciado pide el intercambio.
        const auto r = jacobi.resolver({ { 0.0, 1.0 }, { 1.0, 1.0 } }, { 1.0, 2.0 }, {}, es6, 50);
        comprobar(r.has_value(), "Con a₁₁ = 0 el método aún devuelve un resultado");
        if (r) {
            comprobar(r->reordenamiento.aplicado,
                      "Con a₁₁ = 0 hay que intercambiar las ecuaciones");
            comprobar(r->motivo == MotivoParada::SolucionExacta,
                      "Reordenado, el sistema resuelve exactamente");
            comprobarCercano(r->solucion()[0], 1.0, 1e-12, "x₁ = 1");
            comprobarCercano(r->solucion()[1], 1.0, 1e-12, "y₁ = 1");
        }
    }

    // ---------- Guarda: diagonal nula que el reordenamiento NO arregla ----
    {
        // Ningún intercambio saca a esta matriz de la diagonal nula: la fila 1
        // es toda ceros y la 2 tiene los dos coeficientes iguales. Aquí sí, y solo
        // aquí, el método tiene que rendirse.
        const auto r = jacobi.resolver({ { 0.0, 0.0 }, { 1.0, 1.0 } }, { 1.0, 2.0 }, {}, es6, 50);
        comprobar(r.has_value(), "Con diagonal nula el método aún devuelve un resultado");
        if (r) {
            comprobar(r->motivo == MotivoParada::DiagonalNula,
                      "a₁₁ = 0 sin remedio debe avisar de diagonal nula");
            comprobar(r->iteracionesUsadas() == 1,
                      "Se detiene tras la iterada inicial: no puede avanzar");
        }

        // Y el caso espejo, con la fila nula abajo.
        const auto espejo = jacobi.resolver({ { 1.0, 1.0 }, { 0.0, 0.0 } }, { 1.0, 2.0 },
                                            {}, es6, 50);
        comprobar(espejo.has_value() && espejo->motivo == MotivoParada::DiagonalNula,
                  "a₂₂ = 0 también debe avisar de diagonal nula");
    }

    // ---------- Guarda: divergencia --------------------------------------
    {
        // ρ(D⁻¹(L+U)) = √6 > 1: las iteraciones se disparan.
        const auto r = jacobi.resolver({ { 1.0, 2.0 }, { 3.0, 4.0 } }, { 1.0, 2.0 },
                                       { 1.0, 1.0 }, es6, 150);
        comprobar(r.has_value(), "Un sistema divergente debe devolver resultado");
        if (r) {
            comprobar(r->motivo == MotivoParada::Divergente,
                      "Un sistema divergente debe avisar de divergencia");
        }
    }

    // ---------- Solución exacta ------------------------------------------
    {
        // 2x₁ = 4 y 2x₂ = 6 tienen solución exactamente representable.
        const auto r = jacobi.resolver({ { 2.0, 0.0 }, { 0.0, 2.0 } }, { 4.0, 6.0 },
                                       { 2.0, 3.0 }, es6, 50);
        comprobar(r.has_value(), "El sistema trivial debe resolverse");
        if (r) {
            comprobar(r->motivo == MotivoParada::SolucionExacta,
                      "Partiendo de la solución, el residuo se anula");
            comprobar(r->iteracionesUsadas() == 1,
                      "Basta con comprobar la iterada inicial");
            comprobarCercano(r->solucion()[0], 2.0, 0.0, "x₁ = 2 exacto");
            comprobarCercano(r->solucion()[1], 3.0, 0.0, "x₂ = 3 exacto");
        }
    }

    // ---------- La fórmula de solución no existe si A es singular ---------
    {
        comprobar(!Jacobi::formulaCramer({ { 1.0, 2.0 }, { 2.0, 4.0 } }, { 1.0, 2.0 })
                       .has_value(),
                  "A singular sin solución única: no hay fórmula de Cramer");
        comprobar(!Jacobi::formulaCramer({ { 1.0, 1.0 }, { 1.0, 1.0 } }, { 1.0, 1.0 })
                       .has_value(),
                  "Filas idénticas: no hay fórmula de Cramer");
        // Pero el cálculo del método sigue adelante: informa, no bloquea.
        const auto r = jacobi.resolver({ { 1.0, 1.0 }, { 1.0, 1.0 } }, { 1.0, 1.0 },
                                       { 0.0, 0.0 }, es6, 50);
        comprobar(r.has_value(),
                  "Un sistema singular debe resolverse aunque no tenga fórmula");
        if (r) {
            comprobar(!r->formulaSolucion.has_value(),
                      "Y el resultado no inventa una fórmula de solución");
        }
        // Matriz mal formada: tampoco hay fórmula.
        comprobar(!Jacobi::formulaCramer({ { 1.0, 2.0 } }, { 1.0 }).has_value(),
                  "Una matriz que no valida no tiene fórmula");
    }

    // ---------- Forma inválida: DimensionInvalida ------------------------
    {
        const std::vector<std::vector<std::vector<double>>> matrices = {
            {},                                             // sin ecuaciones
            { { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 } },      // 2×3, no cuadrada
            { { 1.0, 0.0 }, { 0.0 } },                     // filas de tamaño distinto
            { { 1.0, 0.0 }, { 0.0, 1.0 } },                // correcta: control
        };
        const std::vector<std::vector<double>> terminos = {
            {}, { 1.0, 2.0 }, { 1.0, 2.0 }, { 1.0, 2.0 },
        };
        for (std::size_t i = 0; i < matrices.size(); ++i) {
            const auto r = jacobi.resolver(matrices[i], terminos[i], {}, es6, 50);
            if (i < matrices.size() - 1) {
                comprobar(!r.has_value() && r.error() == ErrorMetodo::DimensionInvalida,
                          "Una matriz mal formada debe dar DimensionInvalida");
            } else {
                comprobar(r.has_value(), "La matriz 2×2 de control debe resolverse");
            }
        }

        // b con longitud distinta de la matriz.
        const auto descuadre =
            jacobi.resolver({ { 1.0, 0.0 }, { 0.0, 1.0 } }, { 1.0 }, {}, es6, 50);
        comprobar(!descuadre.has_value() && descuadre.error() == ErrorMetodo::DimensionInvalida,
                  "b con menos componentes que filas debe rechazarse");

        // Un NaN en la matriz no puede propagarse en silencio.
        const auto conNaN = jacobi.resolver({ { 1.0, 0.0 }, { 0.0, std::nan("") } },
                                            { 1.0, 2.0 }, {}, es6, 50);
        comprobar(!conNaN.has_value() && conNaN.error() == ErrorMetodo::DimensionInvalida,
                  "Un valor no finito en la matriz debe rechazarse");
    }

    // ---------- La entrada con dimensión incoherente se rechaza ----------
    {
        Entrada e;
        e.matriz = matrizClasica();
        e.terminos = terminosClasicos();
        e.dimension = 4;  // la matriz es 3×3: estado imposible
        e.cifras = 6;
        const auto r = jacobi.resolver(e);
        comprobar(!r.has_value() && r.error() == ErrorMetodo::DimensionInvalida,
                  "Declarar una dimensión que no cuadra con la matriz debe rechazarse");

        // Y la coherente sí.
        e.dimension = 3;
        const auto buena = jacobi.resolver(e);
        comprobar(buena.has_value(), "Con la dimensión correcta debe resolverse");
    }

    // ---------- El p de la entrada llega hasta el resultado ---------------
    {
        Entrada e;
        e.matriz = matrizClasica();
        e.terminos = terminosClasicos();
        e.dimension = 3;
        e.cifras = 6;
        e.normaP = 2;
        const auto r = jacobi.resolver(e);
        comprobar(r.has_value(), "Con p en la entrada debe resolverse");
        if (r) {
            comprobar(r->normaP == 2, "Entrada::normaP debe llegar al resultado");
        }

        // Y el valor por defecto de Entrada es p = 3, el del enunciado.
        comprobar(Entrada{}.normaP == 3, "El p por defecto de la entrada es 3");
    }

    // ---------- Entrada vacía: sistema por defecto -----------------------
    {
        Entrada e;  // sin matriz: debe usarse el ejemplo del enunciado
        e.cifras = 6;
        const auto r = jacobi.resolver(e);
        comprobar(r.has_value(), "Sin sistema escrito debe usarse el de ejemplo");
        if (r) {
            comprobar(r->dimension() == 3, "El sistema por defecto tiene 3 ecuaciones");
            comprobar(r->reordenamiento.aplicado,
                      "El ejemplo viene en el orden SIN reordenar, para que se vea el paso 1");
            comprobar(r->dominanteEstricto, "El sistema por defecto acaba dominante");
            comprobarCercano(r->solucion()[0], 1.0, 1e-6, "x del sistema por defecto → 1");
            comprobarCercano(r->solucion()[1], 2.0, 1e-6, "y del sistema por defecto → 2");
            comprobarCercano(r->solucion()[2], 3.0, 1e-6, "z del sistema por defecto → 3");
        }
    }

    // ---------- Criterio de parada: más cifras, más iteraciones ----------
    {
        const auto con4 = jacobi.resolver(matrizClasica(), terminosClasicos(), {}, esPara(4), 400);
        const auto con6 = jacobi.resolver(matrizClasica(), terminosClasicos(), {}, esPara(6), 400);
        comprobar(con4.has_value() && con6.has_value(), "Ambas tolerancias deben resolverse");
        if (con4 && con6) {
            comprobar(con4->iteracionesUsadas() == 11, "Con 4 cifras son 11 filas");
            comprobar(con6->iteracionesUsadas() == 15, "Con 6 cifras son 15 filas");
            comprobar(con6->iteracionesUsadas() > con4->iteracionesUsadas(),
                      "Exigir más cifras debe costar más iteraciones");
        }
    }

    // ---------- Límite de iteraciones se respeta -------------------------
    {
        const auto r = jacobi.resolver(matrizClasica(), terminosClasicos(), {}, esPara(14), 5);
        comprobar(r.has_value(), "Con tope bajo debe terminar, no fallar");
        if (r) {
            comprobar(r->motivo == MotivoParada::MaxIteraciones,
                      "Sin cumplir la tolerancia debe parar por límite de iteraciones");
            comprobar(r->iteracionesUsadas() == 6,
                      "Con tope de 5 pasos son 6 filas: la iterada inicial también cuenta");
        }
    }

    // ---------- La solución no depende de la iterada inicial ---------------
    {
        // El mismo sistema resuelto desde otro punto inicial debe llegar al mismo
        // lugar: la solución no depende del punto de partida.
        const auto desdeCero = jacobi.resolver(matrizClasica(), terminosClasicos(), {}, es6, 150);
        const auto desdeOtro =
            jacobi.resolver(matrizClasica(), terminosClasicos(), { 1.0, -1.0, 2.0 }, es6, 150);
        comprobar(desdeCero.has_value() && desdeOtro.has_value(),
                  "Ambas iteradas iniciales deben resolverse");
        if (desdeCero && desdeOtro) {
            comprobarCercano(desdeCero->solucion()[0], desdeOtro->solucion()[0], 1e-6,
                             "x₁ no depende de la iterada inicial");
            comprobarCercano(desdeCero->solucion()[1], desdeOtro->solucion()[1], 1e-6,
                             "x₂ no depende de la iterada inicial");
            comprobarCercano(desdeCero->solucion()[2], desdeOtro->solucion()[2], 1e-6,
                             "x₃ no depende de la iterada inicial");
        }
    }

    // ---------- Cancelación ----------------------------------------------
    {
        std::stop_source fuente;
        fuente.request_stop();
        const auto r = jacobi.resolver(matrizClasica(), terminosClasicos(), {}, es6, 150,
                                        fuente.get_token());
        comprobar(r.has_value(), "Una cancelación no debe fallar la resolución");
        if (r) {
            comprobar(r->motivo == MotivoParada::Interrumpido,
                      "Con stop pedido antes de empezar debe parar interrumpido");
            comprobar(r->iteraciones.empty(), "No debe registrar iteraciones si no empezó");
        }
    }

    // ---------- El modelo de datos etiqueta y filtra por tipo ------------
    {
        const auto r = jacobi.resolver(matrizClasica(), terminosClasicos(), {}, es6, 150);
        comprobar(r.has_value(), "Resolución para las pruebas del modelo");
        if (r && !r->iteraciones.empty()) {
            const Iteracion& ultima = r->iteraciones.back();
            // Un sistema no tiene raíz escalar: devolver el m = 0.0 que el
            // sistema deja en su último punto presentaría un cero como respuesta.
            comprobar(std::isnan(r->raiz()), "raiz() debe ser NaN para un sistema");

            // valorCampo debe filtrar: los campos de intervalo y de punto inicial
            // no existen aquí, y deben salir vacíos, no como ceros falsos.
            for (const CampoIteracion campo : { CampoIteracion::A, CampoIteracion::B,
                                                 CampoIteracion::M, CampoIteracion::Fa,
                                                 CampoIteracion::Fb, CampoIteracion::Fm,
                                                 CampoIteracion::X, CampoIteracion::Fx,
                                                 CampoIteracion::Fdx, CampoIteracion::Paso }) {
                comprobar(!valorCampo(ultima, campo, r->tipo).has_value(),
                          "Un campo de otro método debe salir vacío en un sistema");
            }
            comprobar(valorCampo(ultima, CampoIteracion::K, r->tipo).has_value(),
                      "k existe en todos los métodos");
            comprobar(valorCampo(ultima, CampoIteracion::Norma, r->tipo).has_value(),
                      "‖Δx‖ existe en un sistema");
            comprobar(valorCampo(ultima, CampoIteracion::VectorX, r->tipo).value_or(9.0) == 9.0,
                      "VectorX no se resuelve por valorCampo: lo resuelve la tabla");

            // valorComponente: dentro de rango sí, fuera no.
            comprobar(valorComponente(ultima, 0).has_value(), "Componente 0 debe existir");
            comprobar(valorComponente(ultima, 2).has_value(), "Componente 2 debe existir");
            comprobar(!valorComponente(ultima, 3).has_value(),
                      "Componente 3 está fuera: debe salir vacía, no como 0");
        }
    }

    // ---------- Los nombres de variable del enunciado ---------------------
    {
        // La rejilla del sistema lleva x, y, z… y no a₁, a₂, a₃: las columnas son
        // los coeficientes de cada variable, y llamarlas así lo escondía.
        comprobar(nombreVariable(0) == "x", "La primera variable se llama x");
        comprobar(nombreVariable(1) == "y", "La segunda variable se llama y");
        comprobar(nombreVariable(2) == "z", "La tercera variable se llama z");
        comprobar(nombreVariable(3) == "w", "La cuarta variable se llama w");
        // Ni la p ni la e: son el orden de la norma y el símbolo del error. La lista
        // tiene 16 letras, así que la última es la j.
        comprobar(nombreVariable(15) == "j", "La última letra libre es la j");
        comprobar(nombreVariable(16) == etiquetaVariable(16),
                  "Sin letras libres se cae a x₁, x₂…");
        // Y sigue siendo distinto del rótulo de la tabla de iteraciones.
        comprobar(etiquetaVariable(0) == "x\u2081",
                  "La tabla de iteraciones sigue usando x₁, x₂…");
    }

    if (fallos == 0) {
        std::println("jacobi_test: 0 fallos");
        return 0;
    }
    std::println("jacobi_test: {} fallos", fallos);
    return 1;
}