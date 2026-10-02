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

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
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
    DiagonalNula,          // a_ii = 0: no existe el paso de Jacobi para esa ecuación
    SolucionExacta,        // A·x = b se cumple exactamente con el vector obtenido
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
    // Sistema lineal. VectorX es una columna *de familia*: el modelo la expande
    // en una subcolumna por cada componente del vector (x₁…x_n), usando `indice`.
    VectorX,  // componente i-ésima de x⁽ᵏ⁾
    Norma,    // ‖x⁽ᵏ⁾ − x⁽ᵏ⁻¹⁾‖_p  (la p del descriptor, no la infinita)
    Ea,       // error relativo porcentual
};

// Definición de una columna de la tabla de iteraciones: rótulo visible y campo
// de origen. Cada método declara su propia lista.
struct ColumnaMetodo {
    std::string titulo;
    CampoIteracion campo;
    // Solo para CampoIteracion::VectorX: qué componente del vector muestra esta
    // columna (-1 en el resto). Lo rellena el modelo al expandir la familia.
    int indice = -1;
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

    // Sistema lineal (Jacobi). Vector vacío en los métodos de raíz.
    //
    // CONVENCIÓN: la iteración k registra el vector x⁽ᵏ⁾, y k empieza en 0. La
    // fila k = 0 es la iterada inicial que tecleó el usuario (x⁰), tal y como la
    // presenta el libro de texto; a partir de ahí cada fila trae el vector
    // recién calculado. Así el error de la fila k compara x⁽ᵏ⁾ con x⁽ᵏ⁻¹⁾ y el
    // primero que existe es el de k = 1, igual que en el enunciado.
    std::vector<double> vectorX;
    // ‖x⁽ᵏ⁾ − x⁽ᵏ⁻¹⁾‖_p con el orden p que se resolvió. Vacío en k = 0: no hay
    // iteración anterior con la que comparar.
    std::optional<double> norma;

    // Los otros dos errores que el enunciado menciona para un sistema, uno por
    // componente: el absoluto |xᵢ⁽ᵏ⁾ − xᵢ⁽ᵏ⁻¹⁾ y el relativo
    // |xᵢ⁽ᵏ⁾ − xᵢ⁽ᵏ⁻¹⁾| / |xᵢ⁽ᵏ⁾|. Vacíos en k = 0, por lo mismo que `norma`.
    std::vector<double> errorAbsoluto;
    std::vector<double> errorRelativo;

    // Error relativo porcentual con la norma p; NO existe en la primera
    // iteración de ningún método, porque no hay valor anterior con el que
    // compararlo.
    std::optional<double> eaPorcentaje;
};

// Valor de un campo dentro de una iteración, o vacío si ese campo no aplica al
// tipo de resolución. Importante: un método de punto inicial deja a, b, m, fa,
// fb y fm en cero (no en NaN), así que sin este filtro la tabla de un método de
// punto inicial mostraría ceros falsos en columnas de intervalo.
[[nodiscard]] std::optional<double> valorCampo(const Iteracion& iteracion, CampoIteracion campo,
                                                TipoResolucion tipo);

// Componente `indice` del vector de una iteración de sistema. Es hermano de
// `valorCampo` y está separado porque un vector no cabe en un `double`: la
// columna `VectorX` se declara UNA vez en el descriptor y la tabla la expande
// en una subcolumna por componente, que es lo que llama a esta función.
[[nodiscard]] std::optional<double> valorComponente(const Iteracion& iteracion,
                                                    std::size_t indice);

// Rótulo de la variable i-ésima de un sistema: x₁, x₂ … x₁₀ con subíndice
// Unicode. Vive en el núcleo (y no en la GUI) porque lo necesitan tanto la
// tabla, al expandir la columna de un sistema, como la narrativa procedimental:
// que las dos coincidence es lo que evita que la columna diga «x1» y la
// explicación diga «x(1)».
[[nodiscard]] std::string etiquetaVariable(std::size_t indice);

// Nombre de variable *tal y como lo escribe el enunciado*: x, y, z… Es la forma
// que tienen que llevar las cabeceras de la rejilla del sistema, porque las
// columnas son los coeficientes de x, de y y de z —llamarlas a₁, a₂, a₃
// escondía justo lo que el ejercicio pide ver. `etiquetaVariable` (x₁, x₂…) se
// reserva para las subcolumnas de la tabla de iteraciones, donde el subíndice
// distingue componentes de un vector y el nombre propio estorbaría: con cuatro
// ecuaciones habría dos «x».
[[nodiscard]] std::string nombreVariable(std::size_t indice);

// Intercambios de filas que hicieron falta para conseguir la diagonal dominante.
// El enunciado lo pide explícitamente: si una fila no es dominante se cambia por
// otra hasta que todas lo sean. Se guarda qué filas se movieron para poder
// contarlo en la narración, y `aplicado` distingue «no hizo falta nada» (el
// sistema ya venía dominante) de «no se pudo» (no existe un orden dominante).
struct Reordenamiento {
    bool aplicado = false;
    // Pares (posición origen, posición destino) en el orden en que se hicieron,
    // con índices de 0. Con 3 ecuaciones y solo la fila 2 dominante, el resultado
    // es un único intercambio {(0, 2)}.
    std::vector<std::pair<std::size_t, std::size_t>> intercambios;
};

// Fórmula de solución exacta por la regla de Cramer. Es la "fórmula de
// solución" que pide el enunciado: x = Dx/D, y = Dy/D, z = Dz/D. Se calcula por
// determinantes (no por el propio Jacobi, que es lo que se quiere comprobar), y
// permite contrastar la aproximación con el valor verdadero.
struct FormulaSolucion {
    double determinante = 0.0;       // D = det(A)
    std::vector<double> determinantes;  // Dx, Dy, Dz… uno por variable
    std::vector<double> valores;         // la solución exacta que resulta de dividir
};

// Una fila del sistema y su dominantica, para poder explicar la comprobación
// fila a fila en vez de dar un sí/no global.
struct DominanciaFila {
    std::size_t indice = 0;      // posición de la fila (0-based)
    double diagonal = 0.0;       // |a_ii|
    double sumaResto = 0.0;      // Σ_{j≠i} |a_ij|
    [[nodiscard]] bool dominante() const { return diagonal > sumaResto; }
};

struct Resultado {
    std::vector<Iteracion> iteraciones;
    MotivoParada motivo = MotivoParada::MaxIteraciones;
    TipoResolucion tipo = TipoResolucion::RaizIntervalo;

    // Sistema lineal del que salió el resultado (matriz A y términos b). Vacío en
    // los métodos de raíz. Viaja dentro del resultado, y no aparte, para que un
    // resultado se describa solo y la GUI pueda narrar las ecuaciones sin
    // acordarse de qué entrada lo produjo.
    std::vector<std::vector<double>> matriz;
    std::vector<double> terminos;
    // Si A es estrictamente diagonalmente dominante, condición suficiente (no
    // necesaria) para que Jacobi converja. Se informa, pero no bloquea: hay
    // sistemas no dominantes que convergen y dominantes que no.
    bool dominanteEstricto = false;

    // ---- Todo lo que el enunciado pide reportar de un sistema ----

    // Orden de p con el que se midió el error. El enunciado trabaja con la
    // "P norma" de orden 3; aquí el orden es configurable, y se guarda para que
    // la tabla y la narración no tengan que suponerlo.
    int normaP = 3;

    // Las filas TAL COMO LAS ESCRIBIÓ el usuario, antes de reordenar. Sin esto,
    // la narración de la comprobación de dominancia describiría el sistema ya
    // reordenado y no se vería cuántas filas hubo que mover.
    std::vector<std::vector<double>> matrizOriginal;
    std::vector<double> terminosOriginal;

    // Comprobación fila a fila, en el orden original y en el final.
    std::vector<DominanciaFila> dominanciaOriginal;
    std::vector<DominanciaFila> dominanciaFinal;

    // Intercambios aplicados para llegar al orden dominante.
    Reordenamiento reordenamiento;

    // Fórmula de solución por Cramer (D, Dx, Dy, Dz y la solución exacta).
    // Vacía cuando det(A) = 0: entonces no hay solución única y la fórmula no
    // existe. No bloquea el cálculo, solo deja de presentarse.
    std::optional<FormulaSolucion> formulaSolucion;

    // Raíz aproximada del último punto válido, sea el punto medio de un
    // intervalo o la iterada de un método de punto inicial. Para un sistema
    // devuelve NaN: la solución es un vector y se lee con `solucion()`.
    [[nodiscard]] double raiz() const;
    [[nodiscard]] int iteracionesUsadas() const;

    // Vector solución del último punto válido de un sistema.
    [[nodiscard]] const std::vector<double>& solucion() const;
    // Número de ecuaciones del sistema (0 si el resultado no es de un sistema).
    [[nodiscard]] int dimension() const;
};

}  // namespace biseccion