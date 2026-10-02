// main.cpp
// -----------------------------------------------------------------------------
// Punto de entrada de la aplicación. Modo gráfico por defecto y modos de
// consola para verificar la solución matemática sin abrir la interfaz:
//   ./Biseccion              → abre la interfaz gráfica
//   ./Biseccion --ayuda      → imprime esta lista y termina
//   ./Biseccion --metodos    → describe el catálogo de métodos disponible
//   ./Biseccion --verificar  → comprueba raíces e iteraciones (E1–E4)
//   ./Biseccion --tablas     → imprime las tablas completas de iteraciones
//   ./Biseccion --comparar   → contrasta bisección y Newton-Raphson en las
//                              mismas raíces (E1–E4)
//   ./Biseccion --sistema    → tabla de Jacobi del sistema lineal 3×3 de ejemplo
// -----------------------------------------------------------------------------
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QStandardPaths>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <format>
#include <functional>
#include <limits>
#include <memory>
#include <print>
#include <ranges>
#include <string>
#include <vector>

#include "core/Biseccion.hpp"
#include "core/CifrasSignificativas.hpp"
#include "core/Jacobi.hpp"
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

// Sistema de referencia de Jacobi: el 3×3 del enunciado, escrito en el ORDEN en
// que aparece allí —con solo la fila 2 dominante— para que el paso 1 del
// procedimiento tenga algo que reordenar. Con x⁰ = (0, 0, 1) y la P norma de
// orden 3, el error de la primera iteración es el ‖Δx‖₃ = 3.147019 que da el
// enunciado, y la solución exacta es (1, 2, 3).
int sistemaEnConsola() {
    const std::vector<std::vector<double>> matriz = {
        { 1.0, 1.0, 4.0 },
        { -2.0, 4.0, 1.0 },
        { 6.0, 3.0, -2.0 },
    };
    const std::vector<double> terminos = { 15.0, 9.0, 6.0 };
    const std::vector<double> inicial = { 0.0, 0.0, 1.0 };
    const double es = 0.5 * std::pow(10.0, 2.0 - 6);
    const int p = 3;
    const Jacobi jacobi;
    const auto resultado = jacobi.resolver(matriz, terminos, inicial, es, 150, {}, p);
    if (!resultado) {
        std::println("El sistema no se pudo resolver.");
        return 1;
    }

    std::println("=== Sistema lineal 3×3 · Jacobi (n = 6, E_s = {:g} %, p = {}) ===", es, p);
    std::println("x⁰ = (0, 0, 1)   ·   error = ‖Δx‖_p / ‖x^(k)‖_p · 100");
    std::println("\n--- Paso 1 · verificación de la convergencia ---");
    std::println("Como está escrito:");
    for (std::size_t i = 0; i < resultado->dominanciaOriginal.size(); ++i) {
        const DominanciaFila& d = resultado->dominanciaOriginal[i];
        std::println("  ecuación de {}: |{}| > {}   ->   {}",
                     nombreVariable(i), d.diagonal, d.sumaResto,
                     d.dominante() ? "dominante" : "NO dominante");
    }
    if (resultado->reordenamiento.intercambios.empty()) {
        std::println("  No hace falta mover nada.");
    } else {
        std::print("  Se intercambian");
        for (const auto& par : resultado->reordenamiento.intercambios) {
            std::print(" las ecuaciones {} y {}", par.first + 1, par.second + 1);
        }
        std::println(" (cada una con su término independiente):");
        for (std::size_t i = 0; i < resultado->dominanciaFinal.size(); ++i) {
            const DominanciaFila& d = resultado->dominanciaFinal[i];
            std::println("  ecuación de {}: |{}| > {}   ->   {}",
                         nombreVariable(i), d.diagonal, d.sumaResto,
                         d.dominante() ? "dominante" : "NO dominante");
        }
    }
    std::println("\n--- Paso 2 · despeje de una variable de cada ecuación ---");
    for (std::size_t i = 0; i < resultado->matriz.size(); ++i) {
        std::print("  {} = ({}", nombreVariable(i), resultado->terminos[i]);
        for (std::size_t j = 0; j < resultado->matriz.size(); ++j) {
            if (j != i) {
                std::print(" {} {}·{}", resultado->matriz[i][j] >= 0.0 ? "+" : "-",
                           std::fabs(resultado->matriz[i][j]), nombreVariable(j));
            }
        }
        std::println(") / {}", resultado->matriz[i][i]);
    }

    std::println("\n--- Paso 3 · tabla de iteraciones ---");
    std::println("{:>3} | {:>12} | {:>12} | {:>12} | {:>16} | {:>16}",
                 "k", "x", "y", "z", "‖Δx‖₃", "e_a (%)");
    for (const Iteracion& it : resultado->iteraciones) {
        std::println("{:>3} | {:>12} | {:>12} | {:>12} | {:>16} | {:>16}",
                     it.k,
                     formatearParaCifras(it.vectorX[0], 6),
                     formatearParaCifras(it.vectorX[1], 6),
                     formatearParaCifras(it.vectorX[2], 6),
                     it.norma ? formatearParaCifras(*it.norma, 6) : "—",
                     it.eaPorcentaje ? formatearParaCifras(*it.eaPorcentaje, 6) : "—");
    }

    std::println("\n--- Fórmula de solución (Cramer) ---");
    if (resultado->formulaSolucion) {
        const FormulaSolucion& f = *resultado->formulaSolucion;
        std::println("  D  = det(A) = {}", f.determinante);
        for (std::size_t i = 0; i < f.determinantes.size(); ++i) {
            std::println("  D{} = {}   ->   {} = D{}/D = {}",
                         nombreVariable(i), f.determinantes[i], nombreVariable(i),
                         nombreVariable(i), f.valores[i]);
        }
        std::print("  Solución exacta:");
        for (std::size_t i = 0; i < f.valores.size(); ++i) {
            // El «·» separa los componentes: sin él, «x = 1 y = 2 z = 3» se lee
            // como una frase y no como tres valores.
            std::print("{} {} = {}", (i == 0) ? "" : "  ·  ", nombreVariable(i),
                       f.valores[i]);
        }
        std::println("");
    } else {
        std::println("  det(A) = 0: no hay solución única, así que no hay fórmula.");
    }

    std::println("\nIteraciones: {}  ·  motivo de parada: {}",
                 resultado->iteracionesUsadas(),
                 resultado->motivo == MotivoParada::ToleranciaAlcanzada ? "e_a < E_s"
                     : resultado->motivo == MotivoParada::SolucionExacta ? "solución exacta"
                     : resultado->motivo == MotivoParada::Divergente ? "divergencia"
                     : resultado->motivo == MotivoParada::DiagonalNula ? "diagonal nula"
                     : resultado->motivo == MotivoParada::Interrumpido ? "interrumpido"
                                                                        : "límite de iteraciones");
    return 0;
}

}  // namespace

// Los modos de consola se listan aquí, y no solo en el README, porque un flag
// mal escrito abría la interfaz sin decir nada: se perdía el trabajo de escribir
// la expresión y no había forma de recordar qué opciones había. `QCoreApplication`
// aún no existe cuando se llama a esta función, así que el texto va fijo.
int ayudaEnConsola(const std::string& modoRecibido) {
    // El aviso de error solo se imprime si el argumento NO es una de las opciones
    // documentadas. La lista va explícita, y no un «empieza por --», porque con
    // esa regla `--ayuda` acababa quejándose de ser un modo desconocido.
    if (!modoRecibido.empty()) {
        const bool conocido = (modoRecibido == "--ayuda") || (modoRecibido == "--help") ||
                              (modoRecibido == "-h") || (modoRecibido == "--metodos") ||
                              (modoRecibido == "--verificar") ||
                              (modoRecibido == "--tablas") ||
                              (modoRecibido == "--comparar") ||
                              (modoRecibido == "--sistema");
        if (!conocido) {
            if (modoRecibido.starts_with("--")) {
                std::println("Error: modo desconocido «{}».", modoRecibido);
            } else {
                std::println("Error: «{}» no es un modo. Se esperaba uno de los listados.",
                             modoRecibido);
            }
        }
    }
    std::println(
        "\n"
        "Métodos numéricos — calculadora interactiva (C++ / Qt 6)\n"
        "\n"
        "  Biseccion                Abre la interfaz gráfica.\n"
        "  Biseccion --ayuda        Muestra esta ayuda. Alias: --help\n"
        "  Biseccion --metodos      Describe el catálogo de métodos (descriptor de\n"
        "                           cada uno: columnas, gráfico, tipo de entrada).\n"
        "  Biseccion --verificar    Verificación matemática de los cuatro\n"
        "                           ejercicios: raíz, f(raíz) e iteraciones (E1–E4).\n"
        "  Biseccion --tablas       Tablas completas de iteraciones de los cuatro\n"
        "                           ejercicios, para las cifras 4, 5 y 6.\n"
        "  Biseccion --comparar     Bisección frente a Newton-Raphson en las mismas\n"
        "                           raíces, con el número de iteraciones de cada una.\n"
        "  Biseccion --sistema      Sistema lineal del enunciado resuelto con\n"
        "                           Jacobi: paso 1 (diagonal dominante e intercambio\n"
        "                           de filas), paso 2 (despeje), tabla de iteraciones y\n"
        "                           fórmula de solución de Cramer.\n"
        "\n"
        "Variables de entorno:\n"
        "  BISECCION_AUTOTEST=1     Recorre los tres métodos solo, sin interacción,\n"
        "                           y guarda una captura de cada uno en el directorio\n"
        "                           temporal del sistema. Requiere pantalla: con\n"
        "                           QT_QPA_PLATFORM=offscreen funciona igual.\n");
    return 0;
}

int main(int argc, char** argv) {
    const std::string modo = (argc > 1) ? std::string(argv[1]) : std::string();

    // Cualquier argumento que no sea un modo conocido responde con la ayuda en
    // lugar de abrir la ventana. La lista es explícita y NO se deduce de «empieza
    // por --»: con esa regla más corta, `--vrf` (por `--verificar`) se colaba en la
    // GUI, que ignoraba lo escrito y además perdía el trabajo del usuario.
    const std::vector<std::string> modos = { "--ayuda",     "--help",    "-h",
                                             "--metodos",   "--verificar", "--tablas",
                                             "--comparar",  "--sistema" };

    if (argc > 1 && std::ranges::find(modos, modo) == modos.end()) {
        return ayudaEnConsola(modo);
    }
    if (modo == "--ayuda" || modo == "--help" || modo == "-h") {
        return ayudaEnConsola(modo);
    }
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
    if (modo == "--sistema") {
        return sistemaEnConsola();
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Métodos Numéricos"));
    QApplication::setApplicationVersion(QStringLiteral("1.1.0"));

    VentanaPrincipal ventana;
    ventana.show();

    // Autotest opcional (solo con BISECCION_AUTOTEST=1): recorre el mismo camino
    // que la interfaz con los tres métodos y guarda una captura de cada uno. Con
    // Jacobi además se selecciona en el combo, para que la captura muestre el
    // editor de matriz y no solo el resultado.
    //
    // Las capturas van al directorio temporal DEL SISTEMA y no a una ruta fija:
    // una ruta absoluta escrita en el código solo funciona en la máquina donde se
    // escribió, y QWidget::grab().save() falla en silencio si el directorio no
    // existe, así que en otro equipo el autotest «pasaba» sin dejar rastro.
    if (qEnvironmentVariableIsSet("BISECCION_AUTOTEST")) {
        // `destino` se copia DENTRO del lambda. Si se capturara por referencia,
        // `destino` moriría al cerrar este `if` y el temporizador leería memoria
        // liberada: ASan lo marca como stack-use-after-scope, y sin él el fallo
        // se manifestaba como un std::bad_alloc al arrancar.
        const QString destino = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                    .absoluteFilePath(QStringLiteral("metodos_numericos"));
        QDir().mkpath(destino);
        std::println("Capturas del autotest en: {}", destino.toStdString());

        // `guardar` se guarda en un shared_ptr porque los temporizadores lo
        // necesitan capturado, y un lambda con captura no se puede copiar. Cada
        // temporizador guarda además su propia copia del shared_ptr, así que
        // sobrevive a este ámbito igual que `destino`.
        const auto guardar = std::make_shared<std::function<void(const char*)>>(
            [&ventana, destino](const char* nombre) {
                if (!ventana.grab().save(destino + QLatin1Char('/') + QLatin1String(nombre))) {
                    std::println("Aviso: no se pudo guardar la captura «{}».", nombre);
                }
            });

        QTimer::singleShot(400, &ventana, &VentanaPrincipal::resolverPruebaBiseccion);
        QTimer::singleShot(1200, &ventana, [&ventana, guardar] {
            (*guardar)("autotest_biseccion.png");
            ventana.resolverPruebaNewton();
        });
        QTimer::singleShot(2200, &ventana, [&ventana, guardar] {
            (*guardar)("autotest_newton.png");
            if (auto* combo = ventana.findChild<QComboBox*>()) {
                combo->setCurrentIndex(2);
            }
            ventana.resolverPruebaJacobi();
        });
        // Solo captura `guardar`: `ventana` se pasa como contexto del temporizador
        // (para que muera con ella), pero dentro del lambda no se usa y capturar
        // algo de más es aviso de sobra en Clang.
        QTimer::singleShot(3200, &ventana, [guardar] {
            (*guardar)("autotest_jacobi.png");
            QApplication::quit();
        });
    }

    return app.exec();
}