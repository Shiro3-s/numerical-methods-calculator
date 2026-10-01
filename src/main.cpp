// main.cpp
// -----------------------------------------------------------------------------
// Punto de entrada de la aplicación. Modo gráfico por defecto y modos de
// consola para verificar la solución matemática:
//   ./Biseccion            → abre la interfaz gráfica
//   ./Biseccion --verificar → comprueba raíces e iteraciones (E1–E4)
//   ./Biseccion --tablas   → imprime las tablas completas de iteraciones
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
#include "ejercicios/Ejercicios.hpp"
#include "gui/VentanaPrincipal.hpp"

namespace {

using namespace biseccion;

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
            const int absoluas = Biseccion::iteracionesParaCifras(ejercicio.a, ejercicio.b, n);
            const double es = 0.5 * std::pow(10.0, 2.0 - n);
            const auto relativo = algoritmo.resolver(ejercicio.a, ejercicio.b, es, 400);
            const int relativas = relativo.has_value() ? relativo->iteracionesUsadas() : -1;
            std::println("  {:>6}  |           {:>6}          |        {:>6}", n, absoluas, relativas);
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

}  // namespace

int main(int argc, char** argv) {
    const std::string modo = (argc > 1) ? std::string(argv[1]) : std::string();
    if (modo == "--verificar") {
        return verificarEnConsola();
    }
    if (modo == "--tablas") {
        return imprimirTablasEnConsola();
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Bisección"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    VentanaPrincipal ventana;
    ventana.show();

    // Autotest opcional (solo con BISECCION_AUTOTEST=1): resuelve el Ejercicio 1
    // mediante el mismo camino que la interfaz y cierra la aplicación.
    if (qEnvironmentVariableIsSet("BISECCION_AUTOTEST")) {
        QTimer::singleShot(400, &ventana, [&ventana] {
            QMetaObject::invokeMethod(
                &ventana, "iniciarResolucion",
                Q_ARG(QString, QStringLiteral("x - cos(x)")),
                Q_ARG(double, 0.0), Q_ARG(double, 1.0), Q_ARG(int, 6));
        });
        QTimer::singleShot(1400, &ventana, [&ventana] {
            ventana.grab().save(QStringLiteral("/tmp/opencode/biseccion_autotest.png"));
        });
        QTimer::singleShot(1600, &ventana, &QApplication::quit);
    }

    return app.exec();
}