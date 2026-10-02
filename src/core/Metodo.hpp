// Metodo.hpp
// -----------------------------------------------------------------------------
// Contrato común para todos los métodos numéricos de la aplicación: bisección,
// Newton-Raphson y, en el futuro, Jacobi (sistemas). El objetivo es que la GUI
// sea completamente dirigida por el *descriptor* del método (tipo de entrada,
// columnas de tabla, pasos procedimentales y si debe mostrarse el gráfico).
// -----------------------------------------------------------------------------
#pragma once

#include <expected>
#include <functional>
#include <memory>
#include <stop_token>
#include <string>
#include <vector>

#include "Resultado.hpp"

namespace biseccion {

// TipoResolucion vive en Resultado.hpp (las iteraciones también lo necesitan) y
// llega aquí por esa inclusión.

struct DescriptorMetodo {
    std::string clave;              // "biseccion", "newton_raphson", "jacobi"
    std::string nombre;             // rótulo del combo
    std::string descripcion;        // texto breve en el panel
    TipoResolucion tipo = TipoResolucion::RaizIntervalo;

    // Columnas de la tabla de iteraciones (una por método).
    std::vector<ColumnaMetodo> columnas;

    // Etiqueta para la expresión auxiliar ("f'(x)", "g(x)", etc.). Vacío si
    // el método no necesita una expresión secundaria.
    std::string etiquetaAuxiliar;
    bool requiereExpresionAuxiliar = false;

    // Nombre del punto que se aproxima: "m" en bisección, "x" en Newton-Raphson.
    // Lo usan la tabla, el gráfico y el resumen para no asumir «m».
    std::string etiquetaRaiz = "m";

    // Si false, el método no trabaja con una expresión que el usuario escriba:
    // no habrá que analizarla antes de resolver. Un sistema lineal se teclea como
    // matriz, no como f(x), así que su descriptor lo pone a false y la GUI se
    // ahorra un parseo que fallaría siempre con la entrada vacía.
    bool requiereExpresion = true;

    // Si false, la GUI ocultará el gráfico y el split se reajustará.
    bool muestraGrafico = true;

    // Si true, el gráfico dibuja además de la curva f(x) la recta asociada a
    // la iteración (la tangente en xₖ para Newton-Raphson). Solo tiene sentido
    // con derivada: se ignora si el método no pide expresión auxiliar.
    bool muestraTrazado = false;

    // Si true, el método puede usar el criterio de "k ≈ iteraciones para n
    // cifras exactas" (válido para bisección). Newton usa e_a; Jacobi, norma.
    bool admiteEstimacionAbsoluta = true;
};

// Datos de entrada que la GUI compone al pulsar "Resolver". Cada método lee
// únicamente los campos que le corresponden según su TipoResolucion.
struct Entrada {
    std::string expresion;            // f(x) o F(x) según el método
    std::string expresionAuxiliar;    // f'(x), g(x), etc.

    // Intervalo (RaizIntervalo)
    double a = 0.0;
    double b = 1.0;

    // Punto inicial (RaizPuntoInicial)
    double x0 = 0.0;

    // Cifras significativas
    int cifras = 6;

    // Sistema lineal (TipoResolucion::Sistema).
    //
    // `matriz` es A (n×n) y `terminos` es b (n). `dimension` es redundante con
    // matriz.size(), y se mantiene a propósito como COMPROBACIÓN: si la GUI
    // declara una dimensión que no cuadra con la matriz, el método lo rechaza en
    // vez de resolver de más o de menos filas.
    std::vector<std::vector<double>> matriz;
    std::vector<double> terminos;
    int dimension = 0;
    // Iterada inicial x⁽⁰⁾. Vacía = empezar en el vector nulo.
    std::vector<double> vectorInicial;
    // Orden p de la norma con la que se mide el error del sistema:
    // ‖v‖_p = (Σ |vᵢ|^p)^(1/p). El enunciado trabaja con p = 3 («3 NORMA
    // P=3»); aquí es configurable y la GUI lo expone en un campo propio, que es
    // el punto donde queda DEFINIDO el p que luego usa todo el cálculo. Solo lo
    // leen los métodos de sistema: en una raíz hay una sola componente y todas
    // las normas dan el mismo valor absoluto.
    int normaP = 3;
};

class MetodoNumerico {
public:
    virtual ~MetodoNumerico() = default;

    [[nodiscard]] virtual DescriptorMetodo descriptor() const = 0;

    // Interfaz uniforme para la GUI. El método utiliza los campos de Entrada
    // pertinentes a su descriptor (a,b para intervalo; x0 para punto inicial).
    [[nodiscard]] virtual std::expected<Resultado, ErrorMetodo>
    resolver(const Entrada& entrada, std::stop_token detener = {}) const = 0;
};

// Catálogo de métodos disponibles. Siempre en orden estable: el primer
// elemento es el recomendado/primero en el selector.
[[nodiscard]] std::vector<std::unique_ptr<MetodoNumerico>> catalogoMetodos();

// Construye el método descrito por 'descriptor' con las expresiones de 'entrada'
// ya analizadas, listo para resolver. Devuelve nullptr si una expresión no se
// pudo analizar o si el método exige un campo que la entrada no trae.
[[nodiscard]] std::unique_ptr<MetodoNumerico> crearDesdeEntrada(
    const Entrada& entrada, const DescriptorMetodo& descriptor);

// Busca un método por su clave ("biseccion", "newton_raphson", ...).
[[nodiscard]] const MetodoNumerico* buscarMetodo(const std::string& clave);

}  // namespace biseccion