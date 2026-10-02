// VentanaPrincipal.cpp
// -----------------------------------------------------------------------------
// Integración de los cuatro paneles y coordinación del cálculo en segundo plano.
// -----------------------------------------------------------------------------
#include "VentanaPrincipal.hpp"

#include <QMetaObject>
#include <QSplitter>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <utility>

#include "GraficoBiseccion.hpp"
#include "ModeloIteraciones.hpp"
#include "PanelEntrada.hpp"
#include "PanelProcedimiento.hpp"
#include "TablaIteraciones.hpp"
#include "core/CifrasSignificativas.hpp"
#include "core/NewtonRaphson.hpp"

namespace biseccion {

VentanaPrincipal::VentanaPrincipal(QWidget* padre) : QMainWindow(padre) {
    construirInterfaz();
    cargarMetodos();
    setWindowTitle(tr("Método de Bisección — Resolución interactiva de ecuaciones"));
    resize(1280, 820);
}

void VentanaPrincipal::construirInterfaz() {
    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* divisionPrincipal = new QSplitter(Qt::Horizontal, central);
    auto* panelIzquierdo = new QSplitter(Qt::Vertical, divisionPrincipal);
    auto* panelDerecho = new QSplitter(Qt::Vertical, divisionPrincipal);

    // Panel 1 (entrada) y Panel 4 (procedimiento).
    panelEntrada_ = new PanelEntrada(panelIzquierdo);
    procedimiento_ = new PanelProcedimiento(panelIzquierdo);
    panelIzquierdo->addWidget(panelEntrada_);
    panelIzquierdo->addWidget(procedimiento_);
    panelIzquierdo->setStretchFactor(0, 0);
    panelIzquierdo->setStretchFactor(1, 1);
    panelIzquierdo->setMinimumWidth(340);

    // Panel 2 (gráfico) y Panel 3 (tabla de iteraciones).
    grafico_ = new GraficoBiseccion(panelDerecho);
    tabla_ = new TablaIteraciones(panelDerecho);
    modelo_ = new ModeloIteraciones(tabla_);
    tabla_->vincularModelo(modelo_);
    panelDerecho->addWidget(grafico_);
    panelDerecho->addWidget(tabla_);
    panelDerecho->setStretchFactor(0, 3);
    panelDerecho->setStretchFactor(1, 2);

    divisionPrincipal->addWidget(panelIzquierdo);
    divisionPrincipal->addWidget(panelDerecho);
    divisionPrincipal->setStretchFactor(0, 0);
    divisionPrincipal->setStretchFactor(1, 1);
    divisionPrincipal->setChildrenCollapsible(true);

    auto* diseno = new QVBoxLayout(central);
    diseno->setContentsMargins(4, 4, 4, 4);
    diseno->addWidget(divisionPrincipal);

    // Enlace Tabla ⇄ Gráfico ⇄ Procedimiento (concepto GeoGebra).
    connect(panelEntrada_, &PanelEntrada::resolverSolicitado,
            this, &VentanaPrincipal::iniciarResolucion);
    connect(panelEntrada_, &PanelEntrada::cancelarSolicitado,
            this, &VentanaPrincipal::cancelarResolucion);
    connect(tabla_, &TablaIteraciones::iteracionSeleccionada,
            this, &VentanaPrincipal::manejarIteracionSeleccionada);
    connect(grafico_, &GraficoBiseccion::marcadorClickeado,
            this, &VentanaPrincipal::manejarMarcadorClickeado);

    statusBar()->showMessage(tr("Listo. Escriba f(x), ajuste el intervalo o elija un "
                                "ejercicio predefinido."));
}

void VentanaPrincipal::cargarMetodos() {
    panelEntrada_->cargarMetodos();
}
void VentanaPrincipal::iniciarResolucion(const Entrada& entrada, const DescriptorMetodo& descriptor) {
    if (calculando_) {
        return;
    }

    // Se analiza f(x) en este hilo (rápido, y permite informar del error con
    // posición y carácter), pero la ITERACIÓN va en segundo plano.
    auto analisisF = parsearFuncion(entrada.expresion);
    if (!analisisF) {
        mostrarErrorAnalisis(analisisF.error().mensaje,
                             tr("Revise la expresión f(x) e inténtelo de nuevo."));
        return;
    }
    if (descriptor.requiereExpresionAuxiliar) {
        if (entrada.expresionAuxiliar.empty()) {
            procedimiento_->mostrarInformacion(
                tr("<h3>Falta un dato</h3><p>El método <b>%1</b> necesita la expresión %2.</p>")
                    .arg(escapar(QString::fromStdString(descriptor.nombre)),
                         escapar(QString::fromStdString(descriptor.etiquetaAuxiliar))));
            statusBar()->showMessage(tr("Falta la expresión auxiliar."));
            return;
        }
        auto analisisAux = parsearFuncion(entrada.expresionAuxiliar);
        if (!analisisAux) {
            mostrarErrorAnalisis(analisisAux.error().mensaje,
                                 tr("Revise la expresión auxiliar e inténtelo de nuevo."));
            return;
        }
    }

    // Construcción tipada: cada método configura sus propios evaluadores.
    auto algoritmo = crearDesdeEntrada(entrada, descriptor);
    if (!algoritmo) {
        mostrarErrorAnalisis({}, tr("No se pudo preparar el método seleccionado."));
        return;
    }

    const std::string funcionTexto = analisisF->texto;
    ultimoTextoF_ = funcionTexto;
    const int n = std::clamp(entrada.cifras, 1, 12);
    ultimasCifrasPendientes_ = n;

    calculando_ = true;
    panelEntrada_->habilitarEjecucion(false);
    procedimiento_->mostrarInformacion(
        tr("<p><i>Resolviendo con %1… (n = %2 cifras significativas)</i></p>")
            .arg(escapar(QString::fromStdString(descriptor.nombre)))
            .arg(n));

    const QPointer<QObject> guarda(this);
    hiloCalculo_ = std::jthread(
        [this, guarda, algoritmo = std::move(algoritmo), entrada, descriptor](
            std::stop_token detener) {
            auto resultado = algoritmo->resolver(entrada, detener);
            std::shared_ptr<Resultado> puntero;
            std::optional<ErrorMetodo> error;
            if (resultado.has_value()) {
                puntero = std::make_shared<Resultado>(std::move(*resultado));
            } else {
                error = resultado.error();
            }
            QMetaObject::invokeMethod(
                this,
                [this, guarda, puntero, error, entrada, descriptor] {
                    if (guarda) {
                        aplicarResultado(puntero, error, entrada, descriptor);
                    }
                },
                Qt::QueuedConnection);
        });
}

void VentanaPrincipal::mostrarErrorAnalisis(const std::string& mensaje,
                                            const QString& consejo) {
    QString html = QStringLiteral("<h3>Error de análisis</h3>");
    if (!mensaje.empty()) {
        html += QStringLiteral("<p><b>%1</b></p>")
                    .arg(escapar(QString::fromStdString(mensaje)));
    }
    html += QStringLiteral("<p>%1</p>").arg(consejo);
    procedimiento_->mostrarInformacion(html);
    statusBar()->showMessage(tr("No se pudo analizar la expresión."));
}

QString VentanaPrincipal::escapar(QString texto) const {
    return PanelProcedimiento::escaparHtml(std::move(texto));
}

void VentanaPrincipal::cancelarResolucion() {
    if (calculando_ && hiloCalculo_.joinable()) {
        hiloCalculo_.request_stop();
        statusBar()->showMessage(tr("Cancelando el cálculo…"));
        return;
    }
    // No había nada en curso: devolver los controles a su estado normal para
    // que la interfaz nunca pueda quedar bloqueada sin salida.
    panelEntrada_->habilitarEjecucion(true);
    statusBar()->showMessage(tr("No hay ningún cálculo en curso."));
}

// Reevalúa f(x) en los dos extremos del intervalo. Cuando la bisección falla por
// «sin cambio de signo», ver los dos números es lo que dice cuál de las tres
// cosas tecleadas (la expresión, a o b) es la que está mal.
QString VentanaPrincipal::valoresEnExtremos(const Entrada& entrada) const {
    const auto analisis = parsearFuncion(entrada.expresion);
    if (!analisis) {
        return {};
    }
    const int n = std::clamp(entrada.cifras, 1, 12);
    const auto comoTexto = [this, n](double x) {
        return escapar(QString::fromStdString(formatearParaCifras(x, n)));
    };
    QString html = tr("<p><code>f(%1) = %2</code><br><code>f(%3) = %4</code></p>");
    html.replace(QStringLiteral("%1"), escapar(QString::number(entrada.a, 'g', 8)));
    html.replace(QStringLiteral("%2"), comoTexto(analisis->f(entrada.a)));
    html.replace(QStringLiteral("%3"), escapar(QString::number(entrada.b, 'g', 8)));
    html.replace(QStringLiteral("%4"), comoTexto(analisis->f(entrada.b)));
    return html;
}

void VentanaPrincipal::aplicarResultado(const std::shared_ptr<Resultado>& resultado,
                                        std::optional<ErrorMetodo> error,
                                        const Entrada& entrada,
                                        const DescriptorMetodo& descriptor) {
    calculando_ = false;
    panelEntrada_->habilitarEjecucion(true);
    const int cifras = std::clamp(ultimasCifrasPendientes_, 1, 12);
    const std::string& funcionTexto = ultimoTextoF_;

    if (!resultado) {
        QString html = QStringLiteral("<h3>No se pudo resolver</h3>");
        switch (error.value_or(ErrorMetodo::ExpresionInvalida)) {
            case ErrorMetodo::SinCambioDeSigno: {
                // Mostrar los dos valores es lo que deja claro cuál de las tres
                // cosas tecleadas (f, a, b) es la que está mal.
                html += tr("<p>f(a) y f(b) tienen el mismo signo en el intervalo indicado, "
                           "así que el método de bisección no puede aplicarse: no se "
                           "garantiza ninguna raíz en él.</p>");
                html += valoresEnExtremos(entrada);
                html += tr("<p>Pruebe con otros extremos.</p>");
                break;
            }
            case ErrorMetodo::ExpresionInvalida:
                html += tr("<p>Alguna de las expresiones no se pudo analizar.</p>");
                break;
            case ErrorMetodo::DimensionInvalida:
                html += tr("<p>El sistema no tiene dimensiones coherentes.</p>");
                break;
        }
        procedimiento_->mostrarInformacion(html);
        grafico_->limpiar();
        modelo_->limpiar();
        statusBar()->showMessage(tr("No se pudo aplicar el método."));
        return;
    }

    ultimoFuncionTexto_ = funcionTexto;
    ultimasCifras_ = cifras;
    ultimoDescriptor_ = descriptor;

    modelo_->setResultado(*resultado, descriptor, cifras);

    // El gráfico solo se dibuja si hay una curva que mostrar. La derivada se
    // analiza otra vez aquí (barato) porque el gráfico la necesita para trazar
    // la tangente de Newton; si el método no la pide, no se toca.
    const bool hayCurva = descriptor.muestraGrafico && !resultado->iteraciones.empty();
    std::function<double(double)> f;
    std::function<double(double)> df;
    if (hayCurva) {
        auto pf = parsearFuncion(funcionTexto);
        if (pf) {
            f = pf->f;
            if (descriptor.muestraTrazado) {
                auto paux = parsearFuncion(entrada.expresionAuxiliar);
                if (paux) {
                    df = paux->f;
                }
            }
        }
    }
    if (f) {
        grafico_->cargarResultado(resultado, descriptor, f, df, cifras);
    } else {
        // Un sistema lineal, o una curva no finita: mejor sin gráfico que con
        // un panel vacío ocupando media ventana.
        grafico_->limpiar();
    }

    const int primerK = resultado->iteraciones.empty() ? -1 : resultado->iteraciones.front().k;
    if (primerK > 0) {
        tabla_->seleccionarIteracion(primerK);  // activa gráfico y procedimiento
    }
    procedimiento_->mostrarResumen(*resultado, funcionTexto, cifras, descriptor);
    statusBar()->showMessage(
        tr("Cálculo completado: %1 iteraciones.").arg(resultado->iteracionesUsadas()));
}

void VentanaPrincipal::manejarIteracionSeleccionada(int k) {
    if (const Iteracion* iteracion = modelo_->iteracionPorK(k)) {
        grafico_->resaltarIteracion(k);
        procedimiento_->mostrarIteracion(*iteracion, ultimoFuncionTexto_, ultimasCifras_,
                                      ultimoDescriptor_);
    }
}

void VentanaPrincipal::manejarMarcadorClickeado(int k) {
    tabla_->seleccionarIteracion(k);
}

// Puntos de entrada para las pruebas y para el autotest por consola: disparan
// la resolución con una entrada válida y típica de cada método, sin depender
// de que el usuario haya rellenado los campos.
void VentanaPrincipal::resolverPruebaBiseccion() {
    Entrada e;
    e.expresion = "x - cos(x)";
    e.a = 0.0;
    e.b = 1.0;
    e.cifras = 6;
    const Biseccion b(std::function<double(double)>{});
    iniciarResolucion(e, b.descriptor());
}

void VentanaPrincipal::resolverPruebaNewton() {
    Entrada e;
    e.expresion = "x - cos(x)";
    e.expresionAuxiliar = "1 + sin(x)";
    e.x0 = 1.0;
    e.cifras = 6;
    const NewtonRaphson n(std::function<double(double)>{}, std::function<double(double)>{});
    iniciarResolucion(e, n.descriptor());
}

}  // namespace biseccion