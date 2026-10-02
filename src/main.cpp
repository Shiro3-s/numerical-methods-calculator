// main.cpp
// -----------------------------------------------------------------------------
// Punto de entrada de la aplicación. Modo gráfico por defecto y modos de
// consola para verificar la solución matemática sin abrir la interfaz:
//   ./Biseccion              → abre la interfaz gráfica
//   ./Biseccion --metodos    → describe el catálogo de métodos disponible
//   ./Biseccion --verificar  → comprueba raíces e iteraciones (E1–E4)
//   ./Biseccion --tablas     → imprime las tablas completas de iteraciones
//   ./Biseccion --comparar   → contrasta bisección y Newton-Raphson en las
//                              mismas raíces (E1–E4)
// -----------------------------------------------------------------------------
#include <QApplication>
#include <QTimer>

#include <cmath>
#include <format>
#include <limits>
#include <print>
#include <string>

#include "core/Biseccion.hpp"
#include "core/CifrasSignificativas.hpp"
#include "core/Metodo.hpp"
#include "core/NewtonRaphson.hpp"
#include "core/Resultado.hpp"
#include "ejercicios/Ejercicios.hpp"
#include "gui/VentanaPrincipal.hpp"

namespace {

using namespace biseccion;

int metodosEnConsola() {
    std::println("=== Catálogo de métodos ===");
    for (const auto& metodo : catalogoMetodos()) {
        const DescriptorMetodo d = metodo->descriptor();
        std::println("· {}  [{}]", d.nombre, d.clave);
        std::println("    {}", d.descripcion);
        std::println("    tipo de entrada : {}",
                     d.tipo == TipoResolucion::RaizIntervalo    ? "intervalo [a, b]"
                     : d.tipo == TipoResolucion::RaizPuntoInicial ? "punto inicial x₀"
                                                                  : "sistema de ecuaciones");
        std::println("    expresión auxiliar: {}",
                     d.requiereExpresionAuxiliar ? d.etiquetaAuxiliar : "(ninguna)");
        std::println("    columnas        :");
        for (std::size_t i = 0; i < d.columnas.size(); ++i) {
            std::println("      {} {}", i + 1, d.columnas[i].titulo);
        }
        std::println("    gráfico: {}   trazado asociado: {}",
                     d.muestraGrafico ? "sí" : "oculto",
                     d.muestraTrazado ? "sí" : "no");
    }
    return 0;
}

int verificarEnConsola() {
    std::println("=== Verificación matemática del método de bisección (E1–E4) ===");
    for (const Ejercicio& ejercicio : ejerciciosPredeterminados()) {
        Biseccion algoritmo(ejercicio.f);

        // Raíz de referencia: tolerancia relativa 0.5·10^(2-14) % y amplio margen.
        const auto alta = algoritmo.resolver(ejercicio.a, ejercicio.b,
                                             0.5 * std::pow(10.0, 2.0 - 14), 400);
        const double raiz = alta.has_value() ? alta->raiz()
                                             : std::numeric_limits<double>::quiet_NaN();
        std::println("{}", ejercicio.nombre);
        std::println("  f(x) = {}   sobre [{}, {}]", ejercicio.funcionTexto,
                     ejercicio.a, ejercicio.b);
        if (alta.has_value()) {
            std::println("  raíz  m = {:.15f}    f(m) = {:.3e}", raiz,
                         std::fabs(alta->iteraciones.back().fm));
        }

        std::println("  n cifras | iteraciones (criterio absoluto) | iteraciones (e_a < E_s)");
        for (const int n : {4, 5, 6}) {
            const int absolutas = Biseccion::iteracionesParaCifras(ejercicio.a, ejercicio.b, n);
            const double es = 0.5 * std::pow(10.0, 2.0 - n);
            const auto relativo = algoritmo.resolver(ejercicio.a, ejercicio.b, es, 400);
            const int relativas = relativo.has_value() ? relativo->iteracionesUsadas() : -1;
            std::println("  {:>6}  |           {:>6}          |        {:>6}", n, absolutas, relativas);
        }
    }
    return 0;
}

int imprimirTablasEnConsola() {
    std::println("=== Tablas de iteraciones (n = 6 cifras significativas) ===");
    for (const Ejercicio& ejercicio : ejerciciosPredeterminados()) {
        Biseccion algoritmo(ejercicio.f);
        const double es = 0.5 * std::pow(10.0, 2.0 - 6);
        const auto resultado = algoritmo.resolver(ejercicio.a, ejercicio.b, es, 400);
        if (!resultado) {
            continue;
        }
        std::println("\n### {}", ejercicio.nombre);
        std::println("{:>4} | {:>12} | {:>12} | {:>12} | {:>12} | {:>10}",
                     "k", "a", "b", "m", "f(m)", "e_a (%)");
        for (const Iteracion& it : resultado->iteraciones) {
            const std::string ea = it.eaPorcentaje ? std::format("{:.6f}", *it.eaPorcentaje)
                                                   : "—";
            std::println("{:>4} | {:>12.8f} | {:>12.8f} | {:>12.8f} | {:>12.8f} | {:>10}",
                         it.k, it.a, it.b, it.m, it.fm, ea);
        }
    }
    return 0;
}

// Compara los dos métodos sobre las mismas cuatro raíces. Muestra por qué
// Newton-Raphson necesita un punto inicial y una derivada, y a cambio converge
// en un puñado de iteraciones donde bisección necesita veinte.
int compararEnConsola() {
    std::println("=== Bisección frente a Newton-Raphson (n = 6, E_s = 0.005 %) ===");
    std::println("{:>34} | {:>10} | {:>10} | {:>16}",
                 "f(x)", "bisección", "Newton", "x (Newton)");
    for (const Ejercicio& ejercicio : ejerciciosPredeterminados()) {
        const double es = 0.5 * std::pow(10.0, 2.0 - 6);

        const Biseccion biseccion(ejercicio.f);
        const auto conBiseccion = biseccion.resolver(ejercicio.a, ejercicio.b, es, 400);

        // La derivada viene en el propio ejercicio (la app no la deduce: el
        // usuario la escribe, igual que en el formulario de Newton-Raphson).
        const NewtonRaphson newton(ejercicio.f, ejercicio.df);
        // Punto inicial razonable: el punto medio del intervalo, que para estos
        // cuatro ejercicios no anula f'(x₀).
        const auto conNewton = newton.resolver((ejercicio.a + ejercicio.b) / 2.0, es, 400);

        std::println("{:>34} | {:>10} | {:>10} | {:>16.12f}",
                     ejercicio.funcionTexto,
                     conBiseccion ? conBiseccion->iteracionesUsadas() : -1,
                     conNewton ? conNewton->iteracionesUsadas() : -1,
                     conNewton ? conNewton->raiz() : std::numeric_limits<double>::quiet_NaN());
    }
    std::println("\nMisma raíz, muchas menos iteraciones: esa es la ventaja de Newton,");
    std::println("a cambio de escribir f'(x) y elegir bien la iterada inicial x₀.");
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string modo = (argc > 1) ? std::string(argv[1]) : std::string();
    if (modo == "--verificar") {
        return verificarEnConsola();
    }
    if (modo == "--tablas") {
        return imprimirTablasEnConsola();
    }
    if (modo == "--metodos") {
        return metodosEnConsola();
    }
    if (modo == "--comparar") {
        return compararEnConsola();
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Métodos Numéricos"));
    QApplication::setApplicationVersion(QStringLiteral("1.1.0"));

    VentanaPrincipal ventana;
    ventana.show();

    // Autotest opcional (solo con BISECCION_AUTOTEST=1): recorre el mismo camino
    // que la interfaz con los dos métodos y guarda una captura de cada uno.
    if (qEnvironmentVariableIsSet("BISECCION_AUTOTEST")) {
        QTimer::singleShot(400, &ventana, &VentanaPrincipal::resolverPruebaBiseccion);
        QTimer::singleShot(1200, &ventana, [&ventana] {
            ventana.grab().save(QStringLiteral("/tmp/opencode/autotest_biseccion.png"));
            ventana.resolverPruebaNewton();
        });
        QTimer::singleShot(2200, &ventana, [&ventana] {
            ventana.grab().save(QStringLiteral("/tmp/opencode/autotest_newton.png"));
            QApplication::quit();
        });
    }

    return app.exec();
}