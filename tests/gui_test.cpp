// gui_test.cpp
// -----------------------------------------------------------------------------
// Pruebas de la GUI bajo QPA offscreen. Comprueban que el diseño dirigido por
// descriptor llegue hasta la interfaz: el selector ofrece métodos, la tabla
// cambia de columnas según el método elegido, el gráfico aparece cuando hay
// curva que dibujar y, sobre todo, que un error de análisis no deje el
// formulario bloqueado (un bug ya corregido antes).
// -----------------------------------------------------------------------------
#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>
#include <QTableView>
#include <QTimer>

#include <print>
#include <string>

#include "core/Metodo.hpp"
#include "gui/GraficoBiseccion.hpp"
#include "gui/PanelProcedimiento.hpp"
#include "gui/VentanaPrincipal.hpp"

using namespace biseccion;

namespace {

int fallos = 0;

void comprobar(bool condicion, const std::string& mensaje) {
    if (!condicion) {
        std::println("FALLO: {}", mensaje);
        ++fallos;
    }
}

[[nodiscard]] QPushButton* boton(QWidget* raiz, const QString& texto) {
    for (auto* b : raiz->findChildren<QPushButton*>()) {
        if (b->text() == texto) {
            return b;
        }
    }
    return nullptr;
}

[[nodiscard]] GraficoBiseccion* grafico(QWidget* raiz) {
    return raiz->findChild<GraficoBiseccion*>();
}

[[nodiscard]] QLineEdit* campoPorPlaceholder(QWidget* raiz, const QString& trozo) {
    for (auto* campo : raiz->findChildren<QLineEdit*>()) {
        if (campo->placeholderText().contains(trozo)) {
            return campo;
        }
    }
    return nullptr;
}

// Localiza un QDoubleSpinBox por la etiqueta de su fila del formulario. Buscar
// «el segundo spinbox que aparece» sería frágil en cuanto se añada un campo.
[[nodiscard]] QDoubleSpinBox* spinPorEtiqueta(QWidget* raiz, const QString& texto) {
    for (auto* spin : raiz->findChildren<QDoubleSpinBox*>()) {
        if (const auto* layout = spin->parentWidget()
                                    ? qobject_cast<QFormLayout*>(spin->parentWidget()->layout())
                                    : nullptr) {
            const auto* etiqueta =
                qobject_cast<const QLabel*>(layout->labelForField(spin));
            if (etiqueta && etiqueta->text().contains(texto)) {
                return spin;
            }
        }
    }
    return nullptr;
}

// Texto plano del panel procedimental: es un QTextBrowser que recibe HTML, así
// que se comparan los valores concretos, no las etiquetas.
[[nodiscard]] QString textoProcedimiento(QWidget* raiz) {
    const auto* panel = raiz->findChild<PanelProcedimiento*>();
    return panel ? panel->toPlainText() : QString();
}

// Encabezados de la tabla tal como los ve el usuario: deben cambiar de un
// método a otro, que es justo lo que promete el diseño por descriptor.
[[nodiscard]] QStringList encabezados(QWidget* raiz) {
    QStringList titulos;
    const auto* vista = raiz->findChild<QTableView*>();
    if (!vista || !vista->model()) {
        return titulos;
    }
    for (int c = 0; c < vista->model()->columnCount(); ++c) {
        titulos << vista->model()->headerData(c, Qt::Horizontal).toString();
    }
    return titulos;
}

}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    VentanaPrincipal ventana;
    ventana.show();

    // ---------- 1. El selector ofrece métodos, no ejercicios ----------
    auto* combo = ventana.findChild<QComboBox*>();
    comprobar(combo != nullptr, "Debe existir el combo de métodos");
    if (combo) {
        comprobar(combo->count() == 2,
                  "El combo debe listar 2 métodos (bisección + Newton-Raphson)");
        comprobar(combo->itemText(0) == QStringLiteral("Bisección"),
                  "El primer método debe ser Bisección");
        comprobar(combo->itemText(1) == QStringLiteral("Newton-Raphson"),
                  "El segundo método debe ser Newton-Raphson");
    }

    // ---------- 2. Bisección: se resuelve y la tabla toma SUS columnas ---
    ventana.resolverPruebaBiseccion();

    QTimer::singleShot(1200, &app, [&] {
        const QStringList titulos = encabezados(&ventana);
        comprobar(titulos == (QStringList{QStringLiteral("k"), QStringLiteral("a"),
                                         QStringLiteral("b"), QStringLiteral("m"),
                                         QStringLiteral("f(m)"), QStringLiteral("e_a (%)")}),
                  "Bisección debe mostrar las columnas k|a|b|m|f(m)|e_a");
        if (auto* g = grafico(&ventana)) {
            comprobar(g->isVisible(), "Bisección debe mostrar el gráfico");
            comprobar(!g->muestraTrazado(),
                      "Bisección no debe trazar ninguna recta: no hay derivada");
        }
    });

    // ---------- 3. Newton-Raphson: columnas propias, con su punto x ------
    QTimer::singleShot(1800, &app, [&] { ventana.resolverPruebaNewton(); });

    QTimer::singleShot(2600, &app, [&] {
        const QStringList titulos = encabezados(&ventana);
        comprobar(titulos == (QStringList{QStringLiteral("k"), QStringLiteral("x"),
                                         QStringLiteral("f(x)"), QStringLiteral("f'(x)"),
                                         QStringLiteral("paso"), QStringLiteral("e_a (%)")}),
                  "Newton-Raphson debe mostrar las columnas k|x|f(x)|f'(x)|paso|e_a");
        if (auto* g = grafico(&ventana)) {
            comprobar(g->isVisible(),
                      "Newton-Raphson también dibuja f(x) y la tangente");
            comprobar(g->tieneResultado(), "El gráfico debe retener el resultado de Newton");
            comprobar(g->muestraTrazado(),
                      "Newton-Raphson debe dibujar la recta tangente en x_k");
        }
    });

    // ---------- 4. Un error de análisis no bloquea la interfaz ----------
    QTimer::singleShot(3000, &app, [&] {
        auto* campo = campoPorPlaceholder(&ventana, QStringLiteral("sin(x)"));
        comprobar(campo != nullptr, "Debe existir el campo de f(x)");
        if (campo) {
            campo->setText(QStringLiteral("5x - 2sin(x) + 3x^2 + ((("));  // inválida
        }
        if (auto* resolver = boton(&ventana, QStringLiteral("Resolver"))) {
            resolver->click();
        }
    });

    // ---------- 5. ...y el formulario vuelve a estar utilizable ---------
    QTimer::singleShot(3800, &app, [&] {
        auto* resolver = boton(&ventana, QStringLiteral("Resolver"));
        comprobar(resolver != nullptr, "Debe existir el botón Resolver");
        if (resolver) {
            comprobar(resolver->isEnabled(),
                      "Un error de análisis no debe dejar la interfaz bloqueada");
            comprobar(resolver->isVisible(),
                      "Tras el error debe volver a verse Resolver (no Cancelar)");
        }
    });

    // ---------- 6. «Sin cambio de signo» explica cuáles son los valores --
    // Es el error más fácil de cometer al teclear a mano, así que el mensaje
    // tiene que enseñar f(a) y f(b): sin ellos no se sabe qué número está mal.
    QTimer::singleShot(4100, &app, [&] {
        // Volver a bisección: el paso 3 dejó el selector en Newton-Raphson.
        if (combo) {
            combo->setCurrentIndex(0);
        }
        if (auto* funcion = campoPorPlaceholder(&ventana, QStringLiteral("sin(x)"))) {
            funcion->setText(QStringLiteral("x - cos(x)"));  // válida
        }
        if (auto* a = spinPorEtiqueta(&ventana, QStringLiteral("inferior"))) {
            a->setValue(2.0);
        }
        if (auto* b = spinPorEtiqueta(&ventana, QStringLiteral("superior"))) {
            b->setValue(3.0);  // f(2) y f(3) son ambos negativos
        }
        if (auto* resolver = boton(&ventana, QStringLiteral("Resolver"))) {
            resolver->click();
        }
    });

    QTimer::singleShot(4800, &app, [&] {
        const QString texto = textoProcedimiento(&ventana);
        comprobar(texto.contains(QStringLiteral("mismo signo")),
                  "Un intervalo sin cambio de signo debe avisar de ello");
        comprobar(texto.contains(QStringLiteral("f(2)")) && texto.contains(QStringLiteral("f(3)")),
                  "El aviso debe mostrar los valores de f en los dos extremos");
        // f(2) = 2 - cos(2) = 2.41615…  y  f(3) = 3 - cos(3) = 3.98999…
        comprobar(texto.contains(QStringLiteral("2.41615"))
                      && texto.contains(QStringLiteral("3.98999")),
                  "El aviso debe mostrar los valores reales de f en a y en b");
        if (auto* g = grafico(&ventana)) {
            comprobar(!g->tieneResultado(),
                      "Un fallo debe limpiar el gráfico, no dejar la curva anterior");
        }
    });

    // Cierre garantizado: si algo se queda colgado, el test termina igual en vez
    // de bloquearse en el bucle de eventos.
    QTimer::singleShot(5200, &app, &QApplication::quit);

    const int codigo = app.exec();

    std::println("gui_test: {} fallos", fallos);
    return (fallos == 0 && codigo == 0) ? 0 : 1;
}