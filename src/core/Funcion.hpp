// Funcion.hpp
// -----------------------------------------------------------------------------
// Representación de funciones f(x): std::function + analizador de expresiones.
// Soportes: números, operadores + - * / ^, paréntesis, unario '-',
// multiplicación implícita (5x, 2sin(x), x(x-1), …) y
// funciones: sin cos tan asin acos atan sinh cosh tanh ln log (base 10)
//            log10 log2 exp sqrt cbrt abs floor ceil sign/sgn
// constantes: pi/π, e, tau/τ.
// -----------------------------------------------------------------------------
#pragma once

#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <variant>

namespace biseccion {

// Constantes reconocidas por el analizador.
inline constexpr double PI = 3.14159265358979323846;
inline constexpr double EULER = 2.71828182845904523536;
inline constexpr double TAU = 6.28318530717958647693;

enum class TipoFuncion {
    Sin, Cos, Tan,
    Asin, Acos, Atan,
    Sinh, Cosh, Tanh,
    Ln, Log10, Log2, Exp,
    Sqrt, Cbrt,
    Abs, Floor, Ceil, Signo,
};

// --------------- Árbol de sintaxis abstracta (AST) ---------------
struct Nodo;

struct NodoNumero { double valor = 0.0; };
struct NodoVariable {};

struct NodoBinario {
    enum class Op { Suma, Resta, Multiplicacion, Division, Potencia };
    Op op = Op::Suma;
    std::unique_ptr<Nodo> izquierdo;
    std::unique_ptr<Nodo> derecho;
};

struct NodoUnario {
    enum class Op { Negacion };
    Op op = Op::Negacion;
    std::unique_ptr<Nodo> hijo;
};

struct NodoAplicada {
    TipoFuncion funcion = TipoFuncion::Sin;
    std::unique_ptr<Nodo> argumento;
};

struct Nodo {
    std::variant<NodoNumero, NodoVariable, NodoBinario, NodoUnario, NodoAplicada> datos;
};

// Evalúa el AST en el punto x.
[[nodiscard]] double evaluar(const Nodo& nodo, double x);

// Función evaluable junto con su texto canónico.
struct Funcion {
    std::function<double(double)> f;  // núcleo evaluable
    std::string texto;                // expresión canónica mostrable
};

// Error tipado del analizador (mensaje en español).
struct ErrorParser {
    std::string mensaje;
    std::size_t pos = 0;
};

// Analiza una expresión como f(x).
[[nodiscard]] std::expected<Funcion, ErrorParser> parsearFuncion(const std::string& expresion);

}  // namespace biseccion