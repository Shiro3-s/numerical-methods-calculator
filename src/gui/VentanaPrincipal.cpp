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

namespace biseccion {

VentanaPrincipal::VentanaPrincipal(QWidget* padre) : QMainWindow(padre) {
    construirInterfaz();
    cargarEjercicios();
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

void VentanaPrincipal::cargarEjercicios() {
    panelEntrada_->cargarPresets();
}

void VentanaPrincipal::iniciarResolucion(const QString& expresion, double a, double b, int cifras) {
    if (calculando_) {
        return;
    }
    auto parseado = parsearFuncion(expresion.toStdString());
    if (!parseado) {
        procedimiento_->mostrarInformacion(
            QStringLiteral("<h3>Error de análisis</h3><p><b>%1</b></p>"
                           "<p>Revise la expresión f(x) e inténtelo de nuevo.</p>")
                .arg(QString::fromStdString(parseado.error().mensaje)));
        return;
    }

    Funcion funcion = std::move(*parseado);
    const int n = std::clamp(cifras, 1, 12);
    const double es = 0.5 * std::pow(10.0, 2.0 - n);  // E_s en porcentaje
    Biseccion algoritmo(funcion.f);
    const int maxIteraciones = std::max(300, Biseccion::iteracionesParaCifras(a, b, n) + 20);

    calculando_ = true;
    panelEntrada_->habilitarEjecucion(false);
    procedimiento_->mostrarInformacion(
        tr("<p><i>Resolviendo en [%1, %2] con n = %3 cifras significativas…</i></p>")
            .arg(a).arg(b).arg(n));

    const QPointer<QObject> guarda(this);
    hiloCalculo_ = std::jthread(
        [this, guarda, algoritmo = std::move(algoritmo), funcion, n, a, b, es, maxIteraciones](
            std::stop_token detener) {
            auto resultado = algoritmo.resolver(a, b, es, maxIteraciones, detener);
            std::shared_ptr<Resultado> puntero;
            if (resultado.has_value()) {
                puntero = std::make_shared<Resultado>(std::move(*resultado));
            }
            QMetaObject::invokeMethod(
                this,
                [this, guarda, puntero, funcion, n] {
                    if (guarda) {
                        aplicarResultado(puntero, funcion, n);
                    }
                },
                Qt::QueuedConnection);
        });
}

void VentanaPrincipal::cancelarResolucion() {
    if (hiloCalculo_.joinable()) {
        hiloCalculo_.request_stop();
    }
    statusBar()->showMessage(tr("Cancelando el cálculo…"));
}

void VentanaPrincipal::aplicarResultado(const std::shared_ptr<Resultado>& resultado,
                                        const Funcion& funcion, int cifras) {
    calculando_ = false;
    panelEntrada_->habilitarEjecucion(true);

    if (!resultado) {
        procedimiento_->mostrarInformacion(
            tr("<h3>Sin raíz garantizada</h3>"
               "<p>f(a)\u00b7f(b) ≥ 0 en el intervalo indicado, por lo que el método de "
               "bisección no puede aplicarse.<br>Pruebe con otros extremos o funciones.</p>"));
        statusBar()->showMessage(tr("No se pudo aplicar el método en el intervalo dado."));
        return;
    }

    ultimoFuncionTexto_ = funcion.texto;
    ultimasCifras_ = cifras;

    modelo_->setResultado(*resultado, cifras);
    grafico_->cargarResultado(resultado, funcion.f, cifras);

    const int primerK = resultado->iteraciones.empty() ? -1 : resultado->iteraciones.front().k;
    if (primerK > 0) {
        tabla_->seleccionarIteracion(primerK);  // activa gráfico y procedimiento
    }
    procedimiento_->mostrarResumen(*resultado, funcion.texto, cifras);
    statusBar()->showMessage(
        tr("Cálculo completado: %1 iteraciones.").arg(resultado->iteracionesUsadas()));
}

void VentanaPrincipal::manejarIteracionSeleccionada(int k) {
    if (const Iteracion* iteracion = modelo_->iteracionPorK(k)) {
        grafico_->resaltarIteracion(k);
        procedimiento_->mostrarIteracion(*iteracion, ultimoFuncionTexto_, ultimasCifras_);
    }
}

void VentanaPrincipal::manejarMarcadorClickeado(int k) {
    tabla_->seleccionarIteracion(k);
}

}  // namespace biseccion