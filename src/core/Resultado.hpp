// Resultado.hpp
// -----------------------------------------------------------------------------
// Modelo de datos COMÚN a todos los métodos numéricos: por qué se detuvo la
// iteración, qué error impide empezar, qué valores registró cada iteración y
// cómo se presentan en la tabla.
//
// Este encabezado no menciona ningún método concreto: bisección y Newton-Raphson
// (y, más adelante, los sistemas lineales) escriben aquí. `Iteracion` guarda los
// campos del método de intervalo (a, b, m) y los del método de punto inicial
// (x, f(x), f'(x), paso) como `std::optional`, de modo que cada método rellena
// solo los suyos y el resto queda vacío en vez de valiendo cero.
// -----------------------------------------------------------------------------
#pragma once

#include <optional>
#include <string>
#include <vector>

namespace biseccion {

// Cómo se plantea el problema. Vive aquí, y no en Metodo.hpp, porque las
// iteraciones también necesitan saberlo: un método de intervalo rellena a, b y
// m; uno de punto inicial rellena x; uno de sistema, un vector.
enum class TipoResolucion {
    RaizIntervalo,      // dos extremos a, b (cambio de signo)
    RaizPuntoInicial,   // un punto inicial x0
    Sistema,            // sistema de n ecuaciones (Jacobi)
};

// Motivo por el que una ejecución terminó. Es un dato del resultado, no un
// error: la iteración ocurrió y su historia es válida.
enum class MotivoParada {
    Interrumpido,          // el hilo recibió una solicitud de cancelación
    RaizExacta,            // f(raíz) == 0 → la raíz es exactamente el punto obtenido
    ToleranciaAlcanzada,   // e_a < tolerancia
    DerivadaNula,          // f'(x) = 0: no existe el paso de Newton-Raphson
    Divergente,            // las iteraciones se disparan sin converger
    MaxIteraciones,        // límite alcanzado sin cumplir los criterios
};

// Situaciones que impiden siquiera comenzar la resolución.
enum class ErrorMetodo {
    SinCambioDeSigno,     // f(a) y f(b) no tienen signo opuesto (método de intervalo)
    ExpresionInvalida,    // f(x) o la expresión auxiliar no se pudieron analizar
    DimensionInvalida,    // el sistema no tiene dimensiones coherentes
};

// Campo concreto de una iteración que una columna puede mostrar. Al ser datos
// y no código, la tabla se adapta al método sin duplicar el modelo.
enum class CampoIteracion {
    K,     // número de iteración
    A,     // extremo inferior del intervalo
    B,     // extremo superior del intervalo
    M,     // punto medio (bisección)
    Fa,    // f(a)
    Fb,    // f(b)
    Fm,    // f(m)
    X,     // iterada actual (punto inicial)
    Fx,    // f(x)
    Fdx,   // f'(x)
    Paso,  // x_{k+1} − x_k
    Ea,    // error relativo porcentual
};

// Definición de una columna de la tabla de iteraciones: rótulo visible y campo
// de origen. Cada método declara su propia lista.
struct ColumnaMetodo {
    std::string titulo;
    CampoIteracion campo;
};

struct Iteracion {
    int k = 0;

    // Método de intervalo (bisección).
    double a = 0.0;
    double b = 0.0;
    double m = 0.0;
    double fa = 0.0;  // f(a)
    double fb = 0.0;  // f(b)
    double fm = 0.0;  // f(m)

    // Método de punto inicial (Newton-Raphson).
    std::optional<double> x;      // iterada actual
    std::optional<double> fx;     // f(x)
    std::optional<double> fdx;    // f'(x)
    std::optional<double> paso;   // x_{k+1} − x_k

    // Error relativo porcentual; NO existe en la primera iteración de ningún
    // método, porque no hay valor anterior con el que compararlo.
    std::optional<double> eaPorcentaje;
};

// Valor de un campo dentro de una iteración, o vacío si ese campo no aplica al
// tipo de resolución. Importante: un método de punto inicial deja a, b, m, fa,
// fb y fm en cero (no en NaN), así que sin este filtro la tabla de un método de
// punto inicial mostraría ceros falsos en columnas de intervalo.
[[nodiscard]] std::optional<double> valorCampo(const Iteracion& iteracion, CampoIteracion campo,
                                                TipoResolucion tipo);

struct Resultado {
    std::vector<Iteracion> iteraciones;
    MotivoParada motivo = MotivoParada::MaxIteraciones;
    TipoResolucion tipo = TipoResolucion::RaizIntervalo;

    // Raíz aproximada del último punto válido, sea el punto medio de un
    // intervalo o la iterada de un método de punto inicial.
    [[nodiscard]] double raiz() const;
    [[nodiscard]] int iteracionesUsadas() const;
};

}  // namespace biseccion