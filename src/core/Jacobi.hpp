// Jacobi.hpp
// -----------------------------------------------------------------------------
// Método de Jacobi para sistemas de ecuaciones lineales A·x = b.
//
// Se despeja cada variable usando únicamente los valores de la iterada ANTERIOR:
//
//     x_i^(k+1) = ( b_i − Σ_{j≠i} a_ij · x_j^(k) ) / a_ii
//
// A diferencia de Gauss-Seidel, ninguna componente del paso usa el valor recién
// calculado: por eso todas se actualizan en paralelo y dos iteraciones nunca se
// pisan. A cambio de esa independencia, converge más despacio y necesita que la
// diagonal no se anule.
//
// EL PROCEDIMIENTO DEL ENUNCIADO, PASO A PASO
// --------------------------------------------
// 1. Verificar la convergencia. Se comprueba fila a fila si A es estrictamente
//    diagonalmente dominante (|a_ii| > Σ_{j≠i} |a_ij|). Si alguna fila no lo es,
//    se intercambian ecuaciones hasta que todas lo sean
//    (`reordenarParaDominancia`). El intercambio mueve la ecuación ENTERA: su
//    fila de coeficientes y su término independiente, que si no se separaría de
//    su igualdad y el sistema resolvería otra cosa. La solución no cambia al
//    reordenar: solo cambia el orden en que se enuncian las ecuaciones.
//
// 2. Despejar una variable de cada ecuación. Es exactamente la fórmula del paso
//    de Jacobi, y por eso no se teclea aparte: sale de la fila i.
//
// 3. La tabla de iteraciones empieza en k = 0 con la iterada inicial que escribió
//    el usuario (x⁰), y en k = 1 con el primer vector calculado. Así el primer
//    error que aparece es el de la iteración 1, tal como lo etiqueta el
//    enunciado.
//
// 4. El error se mide con una norma de orden p —la «P norma» del enunciado, que
//    trabaja con p = 3—: ‖Δx‖_p = (Σ |Δxᵢ|^p)^(1/p), y el relativo porcentual
//    e_a = ‖Δx‖_p / ‖x^(k)‖_p · 100. p entra por `Entrada::normaP` y queda
//    DEFINIDO ahí, no repartido por el código: la tabla, la narración y el
//    criterio de parada leen el mismo p del resultado.
//
// CONVENCIÓN DE ITERACIONES
// -------------------------
// La fila k guarda el vector x^(k) y el error con el que se llegó a él, es decir
// ‖x^(k) − x^(k−1)‖_p. La fila k = 0 es la iterada inicial y no tiene error:
// no existe x^(−1) con el que compararla. Newton registra del mismo modo x_k, de
// modo que las tablas de todos los métodos se leen igual.
// -----------------------------------------------------------------------------
#pragma once

#include <cstddef>
#include <expected>
#include <optional>
#include <stop_token>
#include <vector>

#include "Metodo.hpp"
#include "Resultado.hpp"

namespace biseccion {

class Jacobi : public MetodoNumerico {
public:
    Jacobi() = default;

    [[nodiscard]] DescriptorMetodo descriptor() const override;

    [[nodiscard]] std::expected<Resultado, ErrorMetodo>
    resolver(const Entrada& entrada, std::stop_token detener = {}) const override;

    // Llamada numérica directa (tests y consola), sin pasar por Entrada:
    //   resolver(matriz, terminos, xInicial, esPorcentaje, maxIteraciones)
    // `maxIteraciones` cuenta los PASOS dados; la tabla tiene una fila más que
    // pasos, porque incluye la iterada inicial de k = 0. `normaP` va detrás del
    // `stop_token` para no romper las llamadas que ya pasaban la cancelación en
    // esa posición.
    [[nodiscard]] std::expected<Resultado, ErrorMetodo>
    resolver(const std::vector<std::vector<double>>& matriz,
             const std::vector<double>& terminos,
             const std::vector<double>& iteradaInicial,
             double toleranciaEsPorcentaje,
             int maxIteraciones,
             std::stop_token detener = {},
             int normaP = 3) const;

    // Dominancia diagonal ESTRICTA: |a_ii| > Σ_{j≠i} |a_ij| en todas las filas.
    // Es condición suficiente de convergencia de Jacobi, no necesaria. Por eso
    // `resolver` NO la exige: informa, y sigue resolviendo. Bloquear aquí
    // rechazaría sistemas que convergen (y aceptaría otros que divergen).
    [[nodiscard]] static bool esDiagonalmenteDominante(
        const std::vector<std::vector<double>>& matriz);

    // La misma comprobación, fila a fila, con los números que la justifican
    // (|a_ii| y la suma del resto). Narrarla con un sí/no global obligaría a
    // recalcular aquí lo mismo que el método ya sabe.
    [[nodiscard]] static std::vector<DominanciaFila> dominanciaPorFila(
        const std::vector<std::vector<double>>& matriz);

    // Intercambia ecuaciones hasta que todas las filas sean dominantes, o hasta
    // que no quede ningún intercambio que mejore el número de filas dominantes.
    // Mueve las dos piezas que van POR ECUACIÓN —la fila de coeficientes y su
    // término independiente—, porque si se moviera solo una se separaría de su
    // igualdad y el sistema resolvería otra cosa.
    //
    // Lo que NO se mueve es la iterada inicial, y es el punto que más fácil se
    // equivoca de este procedimiento: la fila en la posición i siempre despeja la
    // variable i, porque su diagonal es el coeficiente a_ii de esa misma variable.
    // Intercambiar las ecuaciones 1 y 3 cambia QUÉ ECUACIÓN está en cada sitio,
    // pero el lugar i sigue resolviendo xᵢ. Por eso x⁰ está indexado por variable
    // y no por ecuación, y una iterada inicial (0, 0, 1) sigue siendo (0, 0, 1)
    // después de reordenar: si también se moviera, se leería como «la primera
    // ecuación empieza en 1», que no es lo que el usuario escribió.
    //
    // La estrategia es de mejor mejora: entre todos los intercambios posibles
    // elige el que más filas dominantes deja, y para. Con el sistema del
    // enunciado (solo la fila 2 dominante) eso da un único intercambio, el de las
    // filas 1 y 3. Terminar antes es importante: puede no existir ningún orden
    // dominante, y entonces se informa y se sigue con el que haya.
    static Reordenamiento reordenarParaDominancia(std::vector<std::vector<double>>& matriz,
                                                  std::vector<double>& terminos);

    // ‖v‖_p = (Σ |vᵢ|^p)^(1/p), el orden del error que usa el enunciado.
    // Con p = 1 da la suma de los valores absolutos y con p = 2 el módulo
    // euclídeo; p = 1 sube el error y p ≥ 3 lo baja, porque una norma de orden
    // alto reparte más el peso entre las componentes grandes. Se rechaza p < 1
    // por la misma razón que se rechaza la raíz cuadrada de un determinante
    // negativo: no es un número.
    [[nodiscard]] static double norma(std::vector<double> valores, int p);

    // Fórmula de solución exacta por la regla de Cramer: x = Dx/D, y = Dy/D,
    // z = Dz/D. Es la solución del SISTEMA, no la aproximación de Jacobi: sirve
    // justamente para contrastar la aproximación con el valor verdadero, que es
    // lo que el enunciado pide al pedir la fórmula de solución.
    //
    // Devuelve nullopt cuando det(A) es cero o despreciable: entonces no hay
    // solución única y la fórmula no existe. No es un error del método — Jacobi
    // puede converger en un sistema compatible singular — así que el cálculo
    // sigue adelante y solo deja de presentarse la fórmula.
    [[nodiscard]] static std::optional<FormulaSolucion>
    formulaCramer(const std::vector<std::vector<double>>& matriz,
                  const std::vector<double>& terminos);

    // Comprobación de forma: A cuadrada, b con el mismo número de filas, sin
    // valores no finitos. Devuelve DimensionInvalida con el motivo si algo falla.
    [[nodiscard]] static std::expected<void, ErrorMetodo>
    validar(const std::vector<std::vector<double>>& matriz,
            const std::vector<double>& terminos);
};

}  // namespace biseccion