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
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QStringList>
#include <QTableView>
#include <QTableWidget>
#include <QTimer>

#include <algorithm>
#include <functional>
#include <print>
#include <string>
#include <utility>
#include <vector>

#include "core/Metodo.hpp"
#include "gui/GraficoBiseccion.hpp"
#include "gui/PanelEntrada.hpp"
#include "gui/PanelProcedimiento.hpp"
#include "gui/TablaIteraciones.hpp"
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

// Comprueba que las celdas de datos muestran VALORES y no el guion de «campo
// vacío» del modelo.
//
// Los encabezados correctos no bastan como prueba: si `valorCampo` dejara de
// filtrar por tipo, las cabeceras seguirían siendo las correctas (las decide el
// descriptor) y la tabla mostraría «—» en todas las columnas de datos sin que
// ninguna prueba anterior lo notara.
void comprobarCeldasConValor(QWidget* raiz, const std::string& metodo) {
    const auto* vista = raiz->findChild<QTableView*>();
    comprobar(vista != nullptr && vista->model() != nullptr,
              std::string("Debe haber modelo de tabla para ") + metodo);
    if (!vista || !vista->model()) {
        return;
    }
    const auto* modelo = vista->model();
    if (modelo->rowCount() < 2) {
        comprobar(false, std::string("La tabla de ") + metodo + " debe tener 2+ filas");
        return;
    }
    // Fila 1: la segunda iteración, que ya tiene datos en todas las columnas
    // (la primera no tiene e_a por definición, así que ahí sí habría un «—»
    // legítimo y no serviría para distinguir nada).
    int conValor = 0;
    for (int c = 0; c < modelo->columnCount(); ++c) {
        const QString celda = modelo->data(modelo->index(1, c)).toString();
        if (!celda.isEmpty() && celda != QStringLiteral("—")) {
            ++conValor;
        }
    }
    comprobar(conValor == modelo->columnCount(),
              std::string("Las columnas de ") + metodo +
                  " deben mostrar valores, no campos vacíos (valorCampo debe filtrar por tipo)");
}

// Espera activa: sondea `listo` cada 25 ms hasta que sea cierto o venza el
// plazo, con un bucle de eventos anidado que deja que las señales encoladas
// (el resultado del hilo de cálculo) se entreguen con normalidad.
//
// Sustituye a esperar «tantos milisegundos y a ver qué pasa»: el botón Resolver
// queda deshabilitado mientras hay un cálculo en vuelo, y pulsar sobre un botón
// deshabilitado se ignora en silencio, así que un plazo fijo hace el test
// intermitente según cuánto tarde la resolución anterior.
bool esperarHasta(const std::function<bool()>& listo, int msMaximo) {
    QEventLoop bucle;
    QElapsedTimer reloj;
    reloj.start();
    QTimer sondeador;
    sondeador.setInterval(25);
    bool alcanzado = false;
    QObject::connect(&sondeador, &QTimer::timeout, &bucle, [&] {
        if (listo()) {
            alcanzado = true;
            sondeador.stop();
            bucle.quit();
        } else if (reloj.elapsed() > msMaximo) {
            sondeador.stop();
            bucle.quit();
        }
    });
    sondeador.start();
    bucle.exec();
    return alcanzado;
}

// Espera a que se vacíe la cola: entrega los eventos pendientes y devuelve. Se
// usa entre un cambio de la interfaz y la comprobación que depende de él (por
// ejemplo, tras `setCurrentIndex` hay que dejar que el panel se reconfigure).
void asentar() {
    QCoreApplication::processEvents(QEventLoop::AllEvents);
}

// Devuelve la rejilla del sistema, o nullptr si no existe.
[[nodiscard]] QTableWidget* rejillaSistema(QWidget* raiz) {
    for (auto* g : raiz->findChildren<QGroupBox*>()) {
        if (g->title().contains(QStringLiteral("A\u00b7x = b"))) {
            return g->findChild<QTableWidget*>();
        }
    }
    return nullptr;
}

// Escribe un valor en una celda de la rejilla como haría el usuario: abre el
// editor sobre la celda, teclea y cambia de celda para confirmar. Devuelve el
// texto que quedó tras el ciclo completo de edición.
QString teclear(QTableWidget* rejilla, int fila, int columna, const QString& texto) {
    rejilla->setCurrentCell(fila, columna);
    rejilla->edit(rejilla->model()->index(fila, columna));
    // El editor recién abierto es el que tiene el foco. No se busca con
    // findChild<QLineEdit*>(): el editor de la celda anterior sigue siendo hijo
    // de la vista durante un tiempo después de cerrarse, así que findChild
    // devolvería uno viejo y la escritura se perdería en el limbo.
    auto* editor = qobject_cast<QLineEdit*>(rejilla->focusWidget());
    comprobar(editor != nullptr,
              std::string("La celda (") + std::to_string(fila) + "," +
                  std::to_string(columna) + ") debe abrir un editor de texto");
    if (editor) {
        editor->selectAll();
        editor->insert(texto);
    }
    // Cambiar de celda cierra el editor y dispara setModelData, que es lo que
    // hace el usuario al pulsar Enter o al hacer clic en otra casilla.
    rejilla->setCurrentCell((fila + 1) % rejilla->rowCount(), columna);
    return rejilla->item(fila, columna)->text();
}

}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    VentanaPrincipal ventana;
    ventana.show();

    // ---------- Cómo se encadenan los pasos ----------
    //
    // Los pasos van en un vector y se ejecutan UNO TRAS OTRO, cada uno cuando el
    // anterior ha terminado su cálculo. Antes se encadenaban con plazos fijos
    // (`QTimer::singleShot(1200, ...)`, `singleShot(1800, ...)`), lo que hacía la
    // prueba intermitente: si una resolución tardaba más de lo previsto —una
    // máquina cargada, un antivirus, otro proceso— el paso siguiente se
    // ejecutaba contra un estado a medias y comprobaba columnas que aún no
    // estaban. Un plazo fijo es una apuesta sobre la velocidad; esperar a que
    // el botón Resolver vuelva a habilitarse es una condición sobre el estado.
    std::vector<std::function<void()>> pasos;

    // Ejecuta el paso `i` y, cuando termina, el siguiente. Cada paso corre
    // dentro de un bucle de eventos anidado (esperarHasta), así que puede leer
    // el resultado de un cálculo en cuanto llega de verdad.
    // El botón Resolver se deshabilita mientras hay un cálculo en vuelo y se
    // rehabilita cuando el resultado vuelve a la interfaz. Esperar a que esté
    // habilitado es, por tanto, la forma de saber que el paso anterior terminó:
    // si el cálculo ni siquiera llegó a empezar (una expresión que no parsea,
    // por ejemplo), ya está habilitado y la espera no cuesta nada.
    const auto esperarCalculo = [&ventana] {
        auto* resolver = boton(&ventana, QStringLiteral("Resolver"));
        if (!resolver) {
            return false;
        }
        return esperarHasta([resolver] { return resolver->isEnabled(); }, 5000);
    };

    // `pasos` se captura POR REFERENCIA, y no por valor: este lambda se crea
    // antes de que los pasos se hayan añadido, así que una captura por valor se
    // llevaría una copia VACÍA y el guion terminaría al instante sin comprobar
    // nada (que es exactamente lo que pasó: 0 fallos en 0,13 s).
    //
    // El lambda a sí sí va en `shared_ptr`, porque un `std::function` con captura
    // no se puede copiar y hace falta compartirlo entre los temporizadores que
    // lo invocan.
    const auto ejecutar = std::make_shared<std::function<void(std::size_t)>>();
    *ejecutar = [&app, &pasos, ejecutar, esperarCalculo](std::size_t indice) {
        if (indice >= pasos.size()) {
            QApplication::quit();
            return;
        }
        // La espera va ANTES del paso, no después: cada comprobación lee el
        // resultado que dejó el paso anterior. Ponerla después dejaba al primer
        // paso mirando un cálculo disparado fuera de la secuencia.
        if (indice == 0) {
            // Que la ventana se asiente antes de medir nada: los tamaños que
            // comprueba el paso 10 (la altura de la rejilla) los decide el
            // layout, y el layout no corre hasta que entra el bucle de eventos.
            asentar();
        }
        esperarCalculo();
        pasos[indice]();
        QTimer::singleShot(0, &app, [ejecutar, indice] { (*ejecutar)(indice + 1); });
    };

    // ---------- 1. El selector ofrece métodos, no ejercicios ----------
    auto* combo = ventana.findChild<QComboBox*>();
    comprobar(combo != nullptr, "Debe existir el combo de métodos");
    if (combo) {
        comprobar(combo->count() == 3,
                  "El combo debe listar 3 métodos (bisección, Newton-Raphson, Jacobi)");
        comprobar(combo->itemText(0) == QStringLiteral("Bisección"),
                  "El primer método debe ser Bisección");
        comprobar(combo->itemText(1) == QStringLiteral("Newton-Raphson"),
                  "El segundo método debe ser Newton-Raphson");
        comprobar(combo->itemText(2) == QStringLiteral("Jacobi"),
                  "El tercer método debe ser Jacobi");
    }

    // ---------- 2. Bisección: se resuelve y la tabla toma SUS columnas ---
    ventana.resolverPruebaBiseccion();

    pasos.push_back([&] {
        const QStringList titulos = encabezados(&ventana);
        comprobar(titulos == (QStringList{QStringLiteral("k"), QStringLiteral("a"),
                                         QStringLiteral("b"), QStringLiteral("m"),
                                         QStringLiteral("f(m)"), QStringLiteral("e_a (%)")}),
                  "Bisección debe mostrar las columnas k|a|b|m|f(m)|e_a");
        comprobarCeldasConValor(&ventana, "Bisección");
        if (auto* g = grafico(&ventana)) {
            comprobar(g->isVisible(), "Bisección debe mostrar el gráfico");
            comprobar(!g->muestraTrazado(),
                      "Bisección no debe trazar ninguna recta: no hay derivada");
        }
    });

    // ---------- 3. Newton-Raphson: columnas propias, con su punto x ------
    pasos.push_back([&] { ventana.resolverPruebaNewton(); });

    pasos.push_back([&] {
        const QStringList titulos = encabezados(&ventana);
        comprobar(titulos == (QStringList{QStringLiteral("k"), QStringLiteral("x"),
                                         QStringLiteral("f(x)"), QStringLiteral("f'(x)"),
                                         QStringLiteral("paso"), QStringLiteral("e_a (%)")}),
                  "Newton-Raphson debe mostrar las columnas k|x|f(x)|f'(x)|paso|e_a");
        comprobarCeldasConValor(&ventana, "Newton-Raphson");
        if (auto* g = grafico(&ventana)) {
            comprobar(g->isVisible(),
                      "Newton-Raphson también dibuja f(x) y la tangente");
            comprobar(g->tieneResultado(), "El gráfico debe retener el resultado de Newton");
            comprobar(g->muestraTrazado(),
                      "Newton-Raphson debe dibujar la recta tangente en x_k");
        }
    });

    // ---------- 4. Un error de análisis no bloquea la interfaz ----------
    pasos.push_back([&] {
        auto* campo = campoPorPlaceholder(&ventana, QStringLiteral("sin(x)"));
        comprobar(campo != nullptr, "Debe existir el campo de f(x)");
        if (campo) {
            campo->setText(QStringLiteral("5x - 2sin(x) + 3x^2 + ((("));  // inválida
        }
        asentar();  // el texto tecleado debe llegar al panel antes del clic
        if (auto* resolver = boton(&ventana, QStringLiteral("Resolver"))) {
            resolver->click();
        }
    });

    // ---------- 5. ...y el formulario vuelve a estar utilizable ---------
    pasos.push_back([&] {
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
    pasos.push_back([&] {
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
        // `combo->setCurrentIndex` reconfigura el panel entero, así que el clic
        // solo se registra sobre el formulario ya reconfigurado.
        asentar();
        if (auto* resolver = boton(&ventana, QStringLiteral("Resolver"))) {
            resolver->click();
        }
    });

    pasos.push_back([&] {
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

    // ---------- 7. Jacobi: sistema, columnas expandidas y gráfico oculto ----
    // Es el caso que más comprueba del diseño por descriptor: la columna "x" se
    // declara una vez y la tabla la expande en x₁…xₙ según la dimensión, el
    // gráfico desaparece porque no hay curva, y el panel narra el despeje.
    pasos.push_back([&] {
        if (combo) {
            combo->setCurrentIndex(2);
        }
        asentar();  // el panel de sistema debe existir antes de resolver
        ventana.resolverPruebaJacobi();
    });

    pasos.push_back([&] {
        // El formulario de sistema debe estar visible solo con Jacobi.
        bool hayCajaSistema = false;
        for (auto* g : ventana.findChildren<QGroupBox*>()) {
            if (g->title().contains(QStringLiteral("A·x = b"))) {
                hayCajaSistema = true;
                comprobar(g->isVisible(), "El editor de matriz debe verse con Jacobi");
            }
        }
        comprobar(hayCajaSistema, "Debe existir la caja del sistema A·x = b");

        const QStringList titulos = encabezados(&ventana);
        // La columna de la norma lleva el p que se usó de verdad: el descriptor la
        // declara en genérico porque no lo conoce, y aquí sale con p = 3, que es el
        // del enunciado.
        comprobar(titulos == (QStringList{QStringLiteral("k"), QStringLiteral("x₁"),
                                         QStringLiteral("x₂"), QStringLiteral("x₃"),
                                         QStringLiteral("‖Δx‖3"),
                                         QStringLiteral("e_a (%)")}),
                  "Jacobi debe expandir la columna x en x₁|x₂|x₃ y mostrar ‖Δx‖3 con el p usado");

        if (auto* g = grafico(&ventana)) {
            comprobar(!g->isVisible(),
                      "Un sistema no tiene curva: el gráfico debe ocultarse");
            comprobar(!g->tieneResultado(), "El gráfico no debe retener nada del sistema");
        }

        const QString texto = textoProcedimiento(&ventana);
        comprobar(texto.contains(QStringLiteral("diagonalmente dominante")),
                  "El resumen debe informar de la condición de convergencia");
        comprobar(texto.contains(QStringLiteral("Vector solución")),
                  "El resumen debe presentar la solución como vector");
        // El sistema precargado es el del enunciado, cuya solución exacta es
        // (1, 2, 3). Con 6 cifras significativas los tres salen con 5 decimales.
        comprobar(texto.contains(QStringLiteral("1.00000")) &&
                      texto.contains(QStringLiteral("2.00000")) &&
                      texto.contains(QStringLiteral("3.00000")),
                  "El resumen debe mostrar las tres componentes de la solución");
        comprobar(texto.contains(QStringLiteral("Residuo")),
                  "El resumen debe medir el residuo ‖b − A·x‖∞");

        // ---- Lo que el enunciado pide que aparezca -------------------------
        // (1) el intercambio de filas del paso 1, (2) la definición de P y
        // (3) la fórmula de solución.
        comprobar(texto.contains(QStringLiteral("intercambiaron las ecuaciones 1 y 3")),
                  "El resumen debe contar qué ecuaciones se intercambiaron para "
                  "conseguir la diagonal dominante");
        comprobar(texto.contains(QStringLiteral("Fórmula de solución")),
                  "El resumen debe incluir la fórmula de solución del sistema");
        comprobar(texto.contains(QStringLiteral("Dx/D")) && texto.contains(QStringLiteral("Dz/D")),
                  "La fórmula de solución debe escribirse como x = Dx/D, y = Dy/D, z = Dz/D");
        comprobar(texto.contains(QStringLiteral("3.00000")),
                  "La fórmula debe dar la solución exacta (1, 2, 3)");
    });

    // ---------- 8. La tabla de sistema NO debe mostrar ceros falsos -------
    // Si valorCampo no filtrara por tipo, las columnas del sistema caerían en los
    // campos de intervalo (a, b, m...) y se verían como 0.0, que es un dato
    // seemingly válido en vez de una ausencia.
    pasos.push_back([&] {
        const auto* vista = ventana.findChild<QTableView*>();
        comprobar(vista != nullptr && vista->model() != nullptr, "Debe haber modelo de tabla");
        if (!vista || !vista->model()) {
            return;
        }
        const auto* modelo = vista->model();
        // La fila 0 es k = 0: la iterada inicial, que el enunciado imprime como
        // primera línea de la tabla y que aquí no puede mostrar «—» en la columna
        // k por un fallo de índice.
        if (modelo->rowCount() >= 2) {
            const QModelIndex c0 = modelo->index(0, 0);
            comprobar(modelo->data(c0).toString() == QStringLiteral("0"),
                      "La tabla de Jacobi debe empezar en k = 0, con la iterada inicial");
            // La fila 1 (k = 1) es la primera con error: el enunciado le pone
            // ‖Δx‖₃ = 3.147019, y ese es el número que sale.
            const QModelIndex c1 = modelo->index(1, 1);
            comprobar(modelo->data(c1).toString() == QStringLiteral("1.33333"),
                      "x₁ en k=1 debe ser el primer paso calculado (4/3 con el enunciado)");
            const QModelIndex c4 = modelo->index(1, 4);
            comprobar(modelo->data(c4).toString().startsWith(QStringLiteral("3.147")),
                      "‖Δx‖₃ en k=1 debe ser 3.147019, el valor del enunciado");
        } else {
            comprobar(false, "La tabla de Jacobi debe tener al menos 2 filas");
        }
    });

    // ---------- 9. La narración de una iteración, paso a paso -------------
    // El resumen dice el resultado; al seleccionar una fila, el panel tiene que
    // enseñar el PROCEDIMIENTO en los cuatro pasos del enunciado. Se elige k = 1
    // porque es la primera iteración con error: la 0 no tiene con qué compararse,
    // y es donde el ‖Δx‖₃ del enunciado aparece.
    pasos.push_back([&] {
        auto* tabla = ventana.findChild<TablaIteraciones*>();
        comprobar(tabla != nullptr, "Debe existir la tabla de iteraciones");
        if (!tabla) {
            return;
        }
        tabla->seleccionarIteracion(1);
        const QString texto = textoProcedimiento(&ventana);

        comprobar(texto.contains(QStringLiteral("Paso 1")),
                  "La narración debe abrir con el paso 1 del procedimiento");
        comprobar(texto.contains(QStringLiteral("Paso 2")),
                  "La narración debe incluir el paso 2, el despeje de cada variable");
        comprobar(texto.contains(QStringLiteral("Paso 3")),
                  "La narración debe incluir el paso 3, la sustitución");
        comprobar(texto.contains(QStringLiteral("Paso 4")),
                  "La narración debe incluir el paso 4, el error");

        // El despeje va nombrado por variable, no por a₁, a₂, a₃: cada fila
        // despeja la variable que dice su nombre. Se comprueba el desarrollo
        // entero, que es lo que el enunciado pide escribir.
        comprobar(texto.contains(QString::fromUtf8(
                      "x\n=  (6.00000 + 3.00000 \u00b7 y \u2212 2.00000 \u00b7 z) / 6.00000")) &&
                      texto.contains(QString::fromUtf8(
                          "y\n=  (9.00000 \u2212 2.00000 \u00b7 x + 1.00000 \u00b7 z) "
                          "/ 4.00000")) &&
                      texto.contains(QString::fromUtf8(
                          "z\n=  (15.0000 + 1.00000 \u00b7 x + 1.00000 \u00b7 y) / 4.00000")),
                  "El paso 2 debe despejar x, y y z con la matriz ya reordenada");

        // El paso 3 tiene que decir de qué iterada se sustituye: es lo que
        // distingue a Jacobi de Gauss-Seidel.
        comprobar(texto.contains(QStringLiteral("(0)")),
                  "El paso 3 debe mostrar la sustitución de la iterada anterior, x^(0)");

        // El paso 4 muestra los tres errores del enunciado, y dice con qué p.
        comprobar(texto.contains(QStringLiteral("Error absoluto")),
                  "El paso 4 debe mostrar el error absoluto por componente");
        comprobar(texto.contains(QStringLiteral("Error relativo")),
                  "El paso 4 debe mostrar el error relativo por componente");
        comprobar(texto.contains(QStringLiteral("p = 3")),
                  "El paso 4 debe declarar el orden p con el que se midió la norma");
        // El 3.147019 del enunciado tiene que estar a la vista en k = 1.
        comprobar(texto.contains(QStringLiteral("3.147")),
                  "La narración de k = 1 debe mostrar ‖Δx‖₃ = 3.147019");
    });

    // ---------- 10. La rejilla del sistema debe ser USABLE, no decorativa ----
    // Regresión de un bug real: la rejilla se creaba con las celdas correctas
    // pero el layout la aplastaba a 24 px de alto (un QTableWidget es un área de
    // scroll, así que su minimumSizeHint es diminuto y el QSplitter se lo queda
    // todo). La casilla «editable» pasaba y el usuario no podía escribir nada.
    // Ahora se comprueba que hay superficie real y que lo tecleado llega al
    // resultado, que es lo que el usuario hace de verdad.
    pasos.push_back([&] {
        auto* rejilla = rejillaSistema(&ventana);
        comprobar(rejilla != nullptr, "Debe existir la rejilla del sistema");
        if (!rejilla) {
            return;
        }
        comprobar(rejilla->isVisible(), "La rejilla del sistema debe verse con Jacobi");
        comprobar(rejilla->isEnabled(), "La rejilla del sistema debe estar habilitada");
        comprobar(rejilla->rowCount() == 3 && rejilla->columnCount() == 5,
                  "La rejilla por defecto debe ser 3 ecuaciones × (x y z b x⁰)");

        // ---- Las columnas se llaman por variable, no a₁ a₂ a₃ --------------
        // Es un cambio pedido explícitamente: las columnas SON los coeficientes
        // de x, de y y de z, y llamarlas a₁, a₂, a₃ escondía justo lo que el
        // ejercicio quiere ver.
        QStringList cabecera;
        for (int c = 0; c < rejilla->columnCount(); ++c) {
            const auto* item = rejilla->horizontalHeaderItem(c);
            cabecera << (item ? item->text() : QString());
        }
        comprobar(cabecera == (QStringList{QStringLiteral("x"), QStringLiteral("y"),
                                          QStringLiteral("z"), QStringLiteral("b"),
                                          QStringLiteral("x⁰")}),
                  "Las cabeceras de la rejilla deben ser x|y|z|b|x⁰, no a₁|a₂|a₃|b|x⁰");

        // La cabecera lateral repite la variable de cada fila: es lo que hace
        // legible la última columna, porque «(0, 0, 1)» solo significa «x en 0, y
        // en 0, z en 1» si cada fila dice a qué variable pertenece.
        QStringList laterales;
        for (int f = 0; f < rejilla->rowCount(); ++f) {
            const auto* item = rejilla->verticalHeaderItem(f);
            laterales << (item ? item->text() : QString());
        }
        comprobar(laterales == (QStringList{QStringLiteral("x"), QStringLiteral("y"),
                                            QStringLiteral("z")}),
                  "La cabecera lateral debe nombrar la variable que despeja cada ecuación");

        // ---- El orden p de la norma, y dónde queda definido -----------------
        // El enunciado trabaja con «3 NORMA P=3» y no dice de dónde sale el 3.
        // Aquí hay un campo visible que lo declara, y una fórmula escrita debajo
        // con el p sustituido.
        QSpinBox* spinNorma = nullptr;
        for (auto* s : ventana.findChildren<QSpinBox*>()) {
            if (s->toolTip().contains(QStringLiteral("Orden de la norma"))) {
                spinNorma = s;
                break;
            }
        }
        comprobar(spinNorma != nullptr, "Debe existir el campo del orden p de la norma");
        if (spinNorma) {
            comprobar(spinNorma->value() == 3,
                      "El orden p por defecto debe ser 3, el del enunciado");
            comprobar(spinNorma->isVisible(),
                      "El campo del orden p debe verse al elegir Jacobi");
        }

        bool hayDefinicion = false;
        for (const auto* etiqueta : ventana.findChildren<QLabel*>()) {
            if (etiqueta->text().contains(QStringLiteral("Norma del error")) &&
                etiqueta->text().contains(QStringLiteral("|Δx"))) {
                hayDefinicion = true;
                // La definición lleva el p ya sustituido, no una letra suelta:
                // «con p = 3» es lo que deja claro de dónde sale ese 3.
                comprobar(etiqueta->text().contains(QStringLiteral("con p = 3")),
                          "La definición de la norma debe decir con qué valor de p se calculó");
            }
        }
        comprobar(hayDefinicion,
                  "Debe escribirse la definición de la norma con el p vigente");

        // ---- El paso 1 anticipado: qué se va a hacer con las filas ----------
        // El enunciado pide intercambiar las posiciones de las filas hasta que
        // todas cumplan la condición. El panel lo dice ANTES de resolver, para
        // que el usuario vea el paso y no solo su efecto.
        bool diceIntercambio = false;
        for (const auto* etiqueta : ventana.findChildren<QLabel*>()) {
            if (etiqueta->text().contains(QStringLiteral("No es dominante"))) {
                diceIntercambio = true;
                comprobar(etiqueta->text().contains(QStringLiteral("filas 1 y 3")),
                          "El aviso debe decir qué filas se van a intercambiar");
            }
        }
        comprobar(diceIntercambio,
                  "El panel debe anticipar el reordenamiento del paso 1");

        // Cada celda debe tener altura suficiente para teclear en ella.
        const int altoFila = rejilla->rowHeight(0);
        comprobar(altoFila >= 20,
                  std::string("Las filas deben tener altura utilizable, no ") +
                      std::to_string(altoFila) + " px");

        // Y la rejilla debe tener altura para mostrar su contenido, no una franja.
        // El tope de 8 filas visibles sin scroll es el que fija
        // `PanelEntrada::reconstruirSistema()`.
        constexpr int filasSinScroll = 8;
        const int filasVisibles = std::min(rejilla->rowCount(), filasSinScroll);
        const int altoNecesario =
            rejilla->horizontalHeader()->height() + filasVisibles * altoFila;
        comprobar(rejilla->height() >= altoNecesario,
                  std::string("La rejilla debe mostrar ") +
                      std::to_string(filasVisibles) + " filas: mide " +
                      std::to_string(rejilla->height()) + " px y necesita " +
                      std::to_string(altoNecesario) + " px");

        // Las celdas deben existir y ser editables, no solo estar pintadas.
        comprobar(rejilla->item(0, 0) != nullptr, "La celda (0,0) debe existir");
        if (!rejilla->item(0, 0)) {
            return;
        }
        bool todasEditables = true;
        for (int f = 0; f < rejilla->rowCount(); ++f) {
            for (int c = 0; c < rejilla->columnCount(); ++c) {
                const auto* celda = rejilla->item(f, c);
                if (!celda || !(celda->flags() & Qt::ItemIsEditable)) {
                    todasEditables = false;
                }
            }
        }
        comprobar(todasEditables, "Todas las celdas del sistema deben ser editables");
    });

    // ---------- 11. Teclear un sistema 2×2 propio y resolverlo -------------
    // Se espera a que el botón esté habilitado antes de pulsarlo: si aún hay un
    // cálculo en vuelo (el autotest 3×3 de antes) el click se pierde en
    // silencio y el resumen seguiría mostrando el ejemplo por defecto.
    pasos.push_back([&] {
        auto* rejilla = rejillaSistema(&ventana);
        if (!rejilla) {
            return;
        }
        QSpinBox* spinEcuaciones = nullptr;
        for (auto* s : ventana.findChildren<QSpinBox*>()) {
            if (s->suffix().contains(QStringLiteral("ecuaciones"))) {
                spinEcuaciones = s;
                break;
            }
        }
        comprobar(spinEcuaciones != nullptr, "Debe existir el selector de nº de ecuaciones");
        if (!spinEcuaciones) {
            return;
        }
        spinEcuaciones->setValue(2);

        // [[2,1],[1,3]] | [3,5] | x⁰ = 0  ⇒  x = (0.8, 1.4)
        teclear(rejilla, 0, 0, QStringLiteral("2"));
        teclear(rejilla, 0, 1, QStringLiteral("1"));
        teclear(rejilla, 0, 2, QStringLiteral("3"));
        teclear(rejilla, 1, 0, QStringLiteral("1"));
        teclear(rejilla, 1, 1, QStringLiteral("3"));
        teclear(rejilla, 1, 2, QStringLiteral("5"));
        comprobar(rejilla->item(0, 0)->text() == QStringLiteral("2"),
                  "El valor tecleado en la celda (0,0) debe quedar escrito");
        comprobar(rejilla->item(1, 2)->text() == QStringLiteral("5"),
                  "El valor tecleado en la celda (1,2) —el término b₂— debe quedar escrito");

        auto* resolver = boton(&ventana, QStringLiteral("Resolver"));
        comprobar(resolver != nullptr, "No se encuentra el botón Resolver");
        if (!resolver) {
            return;
        }
        const bool listo = esperarHasta([resolver] { return resolver->isEnabled(); }, 4000);
        comprobar(listo, "El botón Resolver debe rehabilitarse tras el cálculo anterior");
        if (!listo) {
            return;
        }
        resolver->click();

        // Y se espera al resultado del sistema tecleado, sin depender de un plazo.
        const bool resuelto = esperarHasta(
            [&ventana] {
                return textoProcedimiento(&ventana).contains(QStringLiteral("2×2"));
            }, 4000);
        comprobar(resuelto, "El resumen debe actualizarse al resolver el sistema tecleado");

        const QString texto = textoProcedimiento(&ventana);
        // Con 6 cifras significativas: 0.8 sale como 0.800000 y 1.4 como 1.40000.
        comprobar(texto.contains(QStringLiteral("0.800000")) &&
                      texto.contains(QStringLiteral("1.40000")),
                  "La solución debe ser la del sistema tecleado (0.8, 1.4), no la "
                  "del ejemplo por defecto");
        comprobar(texto.contains(QStringLiteral("2×2")),
                  "El resumen debe describir el sistema tecleado como 2×2");
    });

    // Red de seguridad: si algún paso se quedara colgado en su propia espera,
    // la prueba termina igual en vez de bloquear el bucle de eventos para
    // siempre. Es un tope, no el mecanismo normal: el guion acaba solo.
    QTimer::singleShot(30000, &app, &QApplication::quit);

    QTimer::singleShot(0, &app, [ejecutar] { (*ejecutar)(0); });

    const int codigo = app.exec();

    std::println("gui_test: {} fallos", fallos);
    return (fallos == 0 && codigo == 0) ? 0 : 1;
}