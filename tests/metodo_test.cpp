// metodo_test.cpp
// -----------------------------------------------------------------------------
// Pruebas del contrato común a los métodos: el catálogo, los descriptores que
// la GUI usa para configurarse, la fábrica `crearDesdeEntrada` y la equivalencia
// entre resolver(Entrada) y la llamada numérica directa.
// Sin dependencias de Qt.
// -----------------------------------------------------------------------------
#include <cmath>
#include <optional>
#include <print>
#include <string>

#include "core/Biseccion.hpp"
#include "core/Jacobi.hpp"
#include "core/Metodo.hpp"
#include "core/Funcion.hpp"
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

// Busca el descriptor de una clave en el catálogo. Lo devuelve por valor
// (`descriptor()` construye uno nuevo) para no colgar punteros de temporales.
[[nodiscard]] std::optional<DescriptorMetodo> descriptorDe(const std::string& clave) {
    static const auto metodos = catalogoMetodos();
    for (const auto& m : metodos) {
        const DescriptorMetodo d = m->descriptor();
        if (d.clave == clave) {
            return d;
        }
    }
    return std::nullopt;
}

}  // namespace

int main() {
    // ---------- El catálogo trae los tres métodos, en orden y sin repetir ----
    const auto metodos = catalogoMetodos();
    comprobar(metodos.size() == 3, "El catálogo debe traer 3 métodos");
    if (metodos.size() == 3) {
        comprobar(metodos[0]->descriptor().clave == "biseccion",
                  "El primer método del selector es Bisección");
        comprobar(metodos[1]->descriptor().clave == "newton_raphson",
                  "El segundo método del selector es Newton-Raphson");
        comprobar(metodos[2]->descriptor().clave == "jacobi",
                  "El tercer método del selector es Jacobi");
    }
    comprobar(buscarMetodo("newton_raphson") != nullptr,
              "buscarMetodo debe encontrar newton_raphson");
    comprobar(buscarMetodo("jacobi") != nullptr,
              "buscarMetodo debe encontrar jacobi");
    comprobar(buscarMetodo("gauss_seidel") == nullptr,
              "buscarMetodo debe devolver nullptr para un método inexistente");

    // ---------- Descriptores: lo que la GUI necesita para configurarse ----
    const std::optional<DescriptorMetodo> biseccion = descriptorDe("biseccion");
    comprobar(biseccion.has_value(), "Debe existir el descriptor de bisección");
    if (biseccion) {
        comprobar(biseccion->tipo == TipoResolucion::RaizIntervalo,
                  "Bisección resuelve en un intervalo");
        comprobar(biseccion->muestraGrafico, "Bisección dibuja la curva");
        comprobar(!biseccion->muestraTrazado,
                  "Bisección no dibuja ninguna recta asociada");
        comprobar(!biseccion->requiereExpresionAuxiliar,
                  "Bisección no pide expresión auxiliar");
        comprobar(biseccion->admiteEstimacionAbsoluta,
                  "Bisección admite la estimación absoluta de iteraciones");
        comprobar(biseccion->columnas.size() == 6, "Bisección define 6 columnas");
        comprobar(biseccion->etiquetaRaiz == "m",
                  "Bisección llama «m» al punto que aproxima");
    }

    const std::optional<DescriptorMetodo> newton = descriptorDe("newton_raphson");
    comprobar(newton.has_value(), "Debe existir el descriptor de Newton-Raphson");
    if (newton) {
        comprobar(newton->tipo == TipoResolucion::RaizPuntoInicial,
                  "Newton-Raphson resuelve desde un punto inicial");
        comprobar(newton->requiereExpresionAuxiliar,
                  "Newton-Raphson exige escribir f'(x)");
        comprobar(newton->etiquetaAuxiliar == "f'(x)",
                  "La etiqueta de la expresión auxiliar es f'(x)");
        comprobar(newton->muestraTrazado,
                  "Newton-Raphson dibuja la tangente: es la idea gráfica del método");
        comprobar(!newton->admiteEstimacionAbsoluta,
                  "Newton-Raphson no admite estimación absoluta (usa e_a)");
        comprobar(newton->columnas.size() == 6, "Newton-Raphson define 6 columnas");
        comprobar(newton->etiquetaRaiz == "x",
                  "Newton-Raphson llama «x» al punto que aproxima");
        comprobar(newton->requiereExpresion,
                  "Newton-Raphson sí trabaja con una expresión escrita");
    }

    const std::optional<DescriptorMetodo> jacobi = descriptorDe("jacobi");
    comprobar(jacobi.has_value(), "Debe existir el descriptor de Jacobi");
    if (jacobi) {
        comprobar(jacobi->tipo == TipoResolucion::Sistema,
                  "Jacobi resuelve un sistema de ecuaciones");
        comprobar(!jacobi->requiereExpresion,
                  "Jacobi no pide f(x): un sistema es una matriz, no una expresión");
        comprobar(!jacobi->requiereExpresionAuxiliar,
                  "Jacobi no pide expresión auxiliar");
        comprobar(!jacobi->muestraGrafico,
                  "Un sistema no tiene curva que dibujar: el gráfico se oculta");
        comprobar(!jacobi->muestraTrazado, "Jacobi no dibuja ninguna recta asociada");
        comprobar(!jacobi->admiteEstimacionAbsoluta,
                  "Jacobi no admite estimación absoluta (usa la norma del error)");
        comprobar(jacobi->columnas.size() == 4,
                  "Jacobi declara 4 columnas (una de ellas se expande en x₁…x_n)");
        comprobar(jacobi->columnas[0].campo == CampoIteracion::K,
                  "Jacobi empieza por k");
        comprobar(jacobi->columnas[1].campo == CampoIteracion::VectorX,
                  "La segunda columna de Jacobi es el vector, que la tabla expande");
        comprobar(jacobi->columnas[2].campo == CampoIteracion::Norma,
                  "Jacobi muestra ‖Δx‖ con el orden p que se eligió");
        // El título va en genérico porque el descriptor NO conoce el p: lo elige
        // el usuario en el panel de entrada. Poner «‖Δx‖∞» fijo mentiría en
        // cuanto lo cambiara, así que la tabla lo reescribe con el p real.
        comprobar(jacobi->columnas[2].titulo == "‖Δx‖p",
                  "El descriptor declara la norma del error en genérico, sin fijar el orden");
        comprobar(jacobi->columnas[3].campo == CampoIteracion::Ea,
                  "Jacobi cierra con e_a");
    }

    // ---------- La fábrica construye métodos ya configurados -------------
    {
        Entrada e;
        e.expresion = "x - cos(x)";
        e.expresionAuxiliar = "1 + sin(x)";
        e.a = 0.0;
        e.b = 1.0;
        e.x0 = 1.0;
        e.cifras = 6;
        const std::optional<DescriptorMetodo> descriptor = descriptorDe("newton_raphson");

        auto metodo = descriptor ? crearDesdeEntrada(e, *descriptor) : nullptr;
        comprobar(metodo != nullptr, "crearDesdeEntrada debe construir Newton-Raphson");
        if (metodo) {
            const auto porFactory = metodo->resolver(e);
            comprobar(porFactory.has_value(), "El método de la fábrica debe resolver");

            // La misma cuenta hecha con evaluadores propios debe coincidir.
            const auto propio = parsearFuncion("x - cos(x)");
            const auto propiaDerivada = parsearFuncion("1 + sin(x)");
            if (propio && propiaDerivada) {
                const NewtonRaphson directo(propio->f, propiaDerivada->f);
                const auto porDirecto = directo.resolver(e);
                comprobar(porDirecto.has_value() == porFactory.has_value(),
                          "fábrica y llamada directa deben coincidir");
                if (porDirecto && porFactory) {
                    comprobar(porDirecto->iteracionesUsadas() == porFactory->iteracionesUsadas(),
                              "Ambas rutas deben hacer el mismo número de iteraciones");
                    comprobar(std::fabs(porDirecto->raiz() - porFactory->raiz()) < 1e-15,
                              "Ambas rutas deben dar la misma raíz");
                }
            }
        }
    }

    // Una expresión inválida debe hacer fallar la construcción, no revientar.
    {
        Entrada e;
        e.expresion = "x - cos(";
        e.a = 0.0;
        e.b = 1.0;
        const std::optional<DescriptorMetodo> descriptor = descriptorDe("biseccion");
        comprobar(descriptor.has_value() && crearDesdeEntrada(e, *descriptor) == nullptr,
                  "Una expresión inválida no debe producir un método");
    }

    // Jacobi se construye SIN analizar nada: no hay expresión que parsear. Si la
    // fábrica exigiera una, el método sería inalcanzable desde la GUI.
    {
        Entrada e;
        e.matriz = { { 2.0, 0.0 }, { 0.0, 4.0 } };
        e.terminos = { 4.0, 8.0 };
        e.dimension = 2;
        e.cifras = 6;
        const std::optional<DescriptorMetodo> descriptor = descriptorDe("jacobi");
        comprobar(descriptor.has_value(), "Debe existir el descriptor de Jacobi");
        auto metodo = descriptor ? crearDesdeEntrada(e, *descriptor) : nullptr;
        comprobar(metodo != nullptr, "crearDesdeEntrada debe construir Jacobi sin expresión");
        if (metodo) {
            const auto porFactory = metodo->resolver(e);
            comprobar(porFactory.has_value(), "El método de la fábrica debe resolver el sistema");
            // La misma cuenta por la llamada numérica directa debe coincidir.
            if (porFactory) {
                const Jacobi directo;
                const auto porDirecto = directo.resolver(e.matriz, e.terminos, e.vectorInicial,
                                                         0.5 * std::pow(10.0, 2.0 - 6), 100);
                comprobar(porDirecto.has_value(), "La llamada directa también debe resolver");
                if (porDirecto) {
                    comprobar(porDirecto->iteracionesUsadas() == porFactory->iteracionesUsadas(),
                              "Ambas rutas deben hacer el mismo número de iteraciones");
                    comprobar(porDirecto->solucion() == porFactory->solucion(),
                              "Ambas rutas deben dar el mismo vector solución");
                }
            }
        }
    }

    // Una forma inválida NO impide construir el método: se detecta al resolver, y
    // se informa como DimensionInvalida en vez de como un fallo de construcción.
    {
        Entrada e;
        e.matriz = { { 1.0, 0.0, 0.0 } };
        e.terminos = { 1.0 };
        e.dimension = 1;
        e.cifras = 6;
        const std::optional<DescriptorMetodo> descriptor = descriptorDe("jacobi");
        auto metodo = descriptor ? crearDesdeEntrada(e, *descriptor) : nullptr;
        comprobar(metodo != nullptr, "La forma del sistema no es cosa de la fábrica");
        if (metodo) {
            const auto r = metodo->resolver(e);
            comprobar(!r.has_value() && r.error() == ErrorMetodo::DimensionInvalida,
                      "Un sistema mal formado debe dar DimensionInvalida al resolver");
        }
    }

    // ---------- resolver(Entrada) equivale a la llamada numérica ---------
    {
        const Biseccion b([](double x) { return x - std::cos(x); });
        Entrada e;
        e.expresion = "x - cos(x)";
        e.a = 0.0;
        e.b = 1.0;
        e.cifras = 6;
        const auto rEntrada = b.resolver(e);
        const double es = 0.5 * std::pow(10.0, 2.0 - 6);
        const auto rNum = b.resolver(0.0, 1.0, es, 200);
        comprobar(rEntrada.has_value() == rNum.has_value(),
                  "resolver(Entrada) y resolver(numérico) deben coincidir");
        if (rEntrada && rNum) {
            comprobar(rEntrada->iteracionesUsadas() == rNum->iteracionesUsadas(),
                      "El número de iteraciones debe coincidir");
            comprobar(std::fabs(rEntrada->raiz() - rNum->raiz()) < 1e-12,
                      "La raíz debe coincidir");
        }
    }

    // ---------- Resultado::raiz() lee x si existe, y si no, m ----------
    {
        Resultado vacio;
        comprobar(std::isnan(vacio.raiz()),
                  "Un resultado sin iteraciones no tiene raíz");
        comprobar(vacio.iteracionesUsadas() == 0, "Sin iteraciones, cero iteraciones");

        Iteracion deNewton;
        deNewton.k = 1;
        deNewton.m = 7.0;  // irrelevante para Newton
        deNewton.x = 0.5;
        vacio.iteraciones.push_back(deNewton);
        comprobar(vacio.raiz() == 0.5, "Con x presente, la raíz es x");

        Iteracion deBiseccion;
        deBiseccion.k = 1;
        deBiseccion.m = 0.25;
        Resultado soloBiseccion;
        soloBiseccion.iteraciones.push_back(deBiseccion);
        comprobar(soloBiseccion.raiz() == 0.25, "Sin x, la raíz es el punto medio m");
    }

    // ---------- Cada método etiqueta su propio resultado ----------------
    // Sin esto, un Resultado suelto no diría si m o x es la respuesta, y la
    // tabla podría enseñar columnas de intervalo con ceros falsos.
    {
        const Biseccion b([](double x) { return x - std::cos(x); });
        const auto conBiseccion = b.resolver(0.0, 1.0, 0.05, 200);
        comprobar(conBiseccion.has_value(), "Bisección debe resolver [0, 1]");
        if (conBiseccion) {
            comprobar(conBiseccion->tipo == TipoResolucion::RaizIntervalo,
                      "El resultado de bisección se etiqueta como intervalo");

            const NewtonRaphson n([](double x) { return x - std::cos(x); },
                                  [](double x) { return 1.0 + std::sin(x); });
            const auto conNewton = n.resolver(1.0, 0.05, 200);
            comprobar(conNewton.has_value(), "Newton-Raphson debe resolver desde x₀=1");
            if (conNewton) {
                comprobar(conNewton->tipo == TipoResolucion::RaizPuntoInicial,
                          "El resultado de Newton-Raphson se etiqueta como punto inicial");
                // Ambos paran con e_a < 0.05 %, así que no tienen por qué dar el
                // mismo dígito: basta con que la diferencia entre las dos raíces
                // sea del orden de la tolerancia pedida, no del orden de las
                // máquinas. Y que a f le quede cerca de cero en ambos casos.
                const double raizBiseccion = conBiseccion->raiz();
                const double raizNewton = conNewton->raiz();
                comprobar(std::fabs(raizBiseccion - raizNewton) / std::fabs(raizBiseccion)
                              < 1e-3,
                          "Ambos métodos deben converger al mismo orden de tolerancia");
                comprobar(std::fabs(raizBiseccion - std::cos(raizBiseccion)) < 1e-3
                              && std::fabs(raizNewton - std::cos(raizNewton)) < 1e-3,
                          "En la raíz de ambos métodos, f(x) debe ser casi cero");
            }
        }
    }

    // ---------- valorCampo: campo presente y campo no usado ------------
    {
        Iteracion it;
        it.k = 3;
        it.a = 1.0;
        it.b = 2.0;
        it.m = 1.5;
        it.fm = -0.25;
        it.x = 1.5;
        it.fx = -0.25;
        it.fdx = 2.0;
        it.paso = -0.75;
        it.eaPorcentaje = 12.5;

        comprobar(valorCampo(it, CampoIteracion::K, TipoResolucion::RaizIntervalo) == 3.0,
                  "valorCampo lee k");
        comprobar(valorCampo(it, CampoIteracion::A, TipoResolucion::RaizIntervalo) == 1.0,
                  "valorCampo lee a en un método de intervalo");
        comprobar(valorCampo(it, CampoIteracion::Fm, TipoResolucion::RaizIntervalo) == -0.25,
                  "valorCampo lee f(m) en un método de intervalo");
        comprobar(valorCampo(it, CampoIteracion::X, TipoResolucion::RaizPuntoInicial) == 1.5,
                  "valorCampo lee x en un método de punto inicial");
        comprobar(valorCampo(it, CampoIteracion::Fdx, TipoResolucion::RaizPuntoInicial) == 2.0,
                  "valorCampo lee f'(x) en un método de punto inicial");
        comprobar(valorCampo(it, CampoIteracion::Paso, TipoResolucion::RaizPuntoInicial)
                      == -0.75,
                  "valorCampo lee el paso en un método de punto inicial");
    }
    {
        // Iteración de Newton: a, b, fa, fb quedan en cero porque no aplican.
        Iteracion it;
        it.k = 1;
        it.x = 1.0;
        comprobar(!valorCampo(it, CampoIteracion::Fa, TipoResolucion::RaizPuntoInicial)
                       .has_value(),
                  "fa no existe en una iteración de punto inicial");
        comprobar(!valorCampo(it, CampoIteracion::X, TipoResolucion::RaizIntervalo)
                       .has_value(),
                  "x no existe en una iteración de intervalo");
    }
    {
        // e_a no existe en la primera iteración de ningún método.
        Iteracion it;
        it.k = 1;
        comprobar(!valorCampo(it, CampoIteracion::Ea, TipoResolucion::RaizPuntoInicial)
                       .has_value(),
                  "e_a no existe en la iteración 1");
        it.eaPorcentaje = 4.0;
        comprobar(valorCampo(it, CampoIteracion::Ea, TipoResolucion::RaizPuntoInicial) == 4.0,
                  "e_a se lee cuando existe");
    }

    std::println("metodo_test: {} fallos", fallos);
    return fallos == 0 ? 0 : 1;
}