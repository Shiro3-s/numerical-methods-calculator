// GraficoBiseccion.cpp
// -----------------------------------------------------------------------------
// Implementación del gráfico interactivo basado en QCustomPlot.
// -----------------------------------------------------------------------------
#include "GraficoBiseccion.hpp"

#include <QMouseEvent>
#include <QOverload>

#include <algorithm>
#include <cmath>
#include <limits>

#include "core/Biseccion.hpp"

namespace biseccion {

namespace {
constexpr QColor COLOR_A(46, 139, 87);     // verde
constexpr QColor COLOR_B(255, 140, 0);     // naranja
constexpr QColor COLOR_M(20, 90, 200);     // azul
}  // namespace

GraficoBiseccion::GraficoBiseccion(QWidget* padre) : QCustomPlot(padre) {
    xAxis->setLabel(QStringLiteral("x"));
    yAxis->setLabel(QStringLiteral("f(x)"));
    xAxis->grid()->setSubGridVisible(true);
    yAxis->grid()->setSubGridVisible(true);
    setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);
    axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);

    crearElementos();

    // Al desplazar o ampliar, los marcadores deben seguir a la curva.
    connect(xAxis, qOverload<const QCPRange&>(&QCPAxis::rangeChanged), this,
            [this](const QCPRange&) { reposicionarMarcadores(); });
    connect(yAxis, qOverload<const QCPRange&>(&QCPAxis::rangeChanged), this,
            [this](const QCPRange&) { reposicionarMarcadores(); });
}

void GraficoBiseccion::crearElementos() {
    // Zona translúcida del intervalo [a, b] seleccionado.
    zona_ = new QCPItemRect(this);
    zona_->topLeft->setType(QCPItemPosition::ptPlotCoords);
    zona_->bottomRight->setType(QCPItemPosition::ptPlotCoords);
    zona_->setPen(QPen(QColor(70, 130, 220, 130), 1, Qt::DashLine));
    zona_->setBrush(QBrush(QColor(70, 130, 220, 40)));
    zona_->setVisible(false);

    // Guías verticales sobre a y b.
    lineaA_ = new QCPItemLine(this);
    lineaA_->start->setType(QCPItemPosition::ptPlotCoords);
    lineaA_->end->setType(QCPItemPosition::ptPlotCoords);
    lineaA_->setPen(QPen(COLOR_A.lighter(120), 1, Qt::DashLine));
    lineaA_->setVisible(false);

    lineaB_ = new QCPItemLine(this);
    lineaB_->start->setType(QCPItemPosition::ptPlotCoords);
    lineaB_->end->setType(QCPItemPosition::ptPlotCoords);
    lineaB_->setPen(QPen(COLOR_B.lighter(120), 1, Qt::DashLine));
    lineaB_->setVisible(false);

    // Tracers ubicados sobre la curva en a, b y m.
    const auto configurarTrazo = [this](QCPItemTracer* trazo, const QColor& color) {
        trazo->setStyle(QCPItemTracer::tsCircle);
        trazo->setSize(9);
        trazo->setPen(QPen(color));
        trazo->setBrush(QBrush(color.lighter(160)));
        trazo->setVisible(false);
    };
    trazoA_ = new QCPItemTracer(this);
    trazoB_ = new QCPItemTracer(this);
    trazoM_ = new QCPItemTracer(this);
    configurarTrazo(trazoA_, COLOR_A);
    configurarTrazo(trazoB_, COLOR_B);
    configurarTrazo(trazoM_, COLOR_M);

    // Etiquetas de los puntos.
    const auto configurarEtiqueta = [this](QCPItemText* etiqueta, const QString& texto,
                                           const QColor& color) {
        etiqueta->setText(texto);
        etiqueta->setColor(color);
        QFont fuente = font();
        fuente.setBold(true);
        fuente.setPointSize(10);
        etiqueta->setFont(fuente);
        etiqueta->position->setType(QCPItemPosition::ptPlotCoords);
        etiqueta->setVisible(false);
    };
    etiquetaA_ = new QCPItemText(this);
    etiquetaB_ = new QCPItemText(this);
    etiquetaM_ = new QCPItemText(this);
    configurarEtiqueta(etiquetaA_, QStringLiteral("a"), COLOR_A.darker(120));
    configurarEtiqueta(etiquetaB_, QStringLiteral("b"), COLOR_B.darker(120));
    configurarEtiqueta(etiquetaM_, QStringLiteral("m"), COLOR_M.darker(120));
}

void GraficoBiseccion::cargarResultado(const std::shared_ptr<const Resultado>& puntero,
                                       const std::function<double(double)>& f,
                                       int cifras, double margenX) {
    // Retener el shared_ptr mantiene el Resultado vivo mientras esté dibujado:
    // VentanaPrincipal re-resuelve y reemplaza su propio shared_ptr sin dejar
    // un puntero colgante en el gráfico (evita heap-use-after-free al hacer
    // clic sobre la tabla o los marcadores).
    if (!puntero) {
        limpiar();
        return;
    }
    resultado_ = puntero;
    funcion_ = f;
    cifras_ = cifras;
    seleccionK_ = -1;
    const Resultado& resultado = *puntero;

    clearGraphs();
    clearItems();
    crearElementos();

    if (resultado.iteraciones.empty()) {
        limpiar();
        return;
    }

    // Rango horizontal: extremos alcanzados por las iteraciones + margen.
    const double xMinimo = resultado.iteraciones.front().a;
    const double xMaximo = resultado.iteraciones.back().b;
    const double pad = margenX * std::max(1.0, std::fabs(xMaximo - xMinimo));

    // Muestreo de la curva (se descartan valores no finitos).
    graficoF_ = addGraph();
    graficoF_->setPen(QPen(QColor(20, 90, 200), 2.2));
    QVector<double> xs;
    QVector<double> ys;
    double yMinimo = std::numeric_limits<double>::infinity();
    double yMaximo = -std::numeric_limits<double>::infinity();
    constexpr int muestras = 900;
    for (int i = 0; i <= muestras; ++i) {
        const double x = (xMinimo - pad) + (double(i) / double(muestras)) * ((xMaximo + pad) - (xMinimo - pad));
        const double y = f(x);
        if (std::isfinite(y)) {
            xs.append(x);
            ys.append(y);
            yMinimo = std::min(yMinimo, y);
            yMaximo = std::max(yMaximo, y);
        }
    }
    graficoF_->setData(xs, ys);

    if (yMinimo > yMaximo) {  // curva sin puntos finitos: rango por defecto
        yMinimo = -1.0;
        yMaximo = 1.0;
    }
    const double rangoY = std::max(0.35 * (yMaximo - yMinimo), 1e-6);
    xAxis->setRange(xMinimo - pad, xMaximo + pad);
    yAxis->setRange(yMinimo - rangoY, yMaximo + rangoY);

    // Registro de marcadores clicables de todas las iteraciones.
    marcadores_.clear();
    for (const Iteracion& iteracion : resultado.iteraciones) {
        marcadores_.push_back({iteracion.a, iteracion.fa, double(iteracion.k)});
        marcadores_.push_back({iteracion.b, iteracion.fb, double(iteracion.k)});
        marcadores_.push_back({iteracion.m, iteracion.fm, double(iteracion.k)});
    }

    resaltarIteracion(resultado.iteraciones.front().k);
    replot();
}

void GraficoBiseccion::resaltarIteracion(int k) {
    if (!resultado_) {
        return;
    }
    const auto pos = std::find_if(resultado_->iteraciones.cbegin(), resultado_->iteraciones.cend(),
                                  [k](const Iteracion& iteracion) { return iteracion.k == k; });
    if (pos == resultado_->iteraciones.cend()) {
        return;
    }
    seleccionK_ = k;
    const Iteracion& iteracion = *pos;

    const double yMinimo = yAxis->range().lower;
    const double yMaximo = yAxis->range().upper;
    const double desplazamiento = 0.08 * (yMaximo - yMinimo);

    // Intervalo [a, b] resaltado.
    zona_->topLeft->setCoords(iteracion.a, yMaximo);
    zona_->bottomRight->setCoords(iteracion.b, yMinimo);
    zona_->setVisible(true);

    lineaA_->start->setCoords(iteracion.a, yMinimo);
    lineaA_->end->setCoords(iteracion.a, yMaximo);
    lineaA_->setVisible(true);
    lineaB_->start->setCoords(iteracion.b, yMinimo);
    lineaB_->end->setCoords(iteracion.b, yMaximo);
    lineaB_->setVisible(true);

    // Puntos sobre la curva (si está cargada y tiene datos).
    const bool curvaDisponible = graficoF_ != nullptr && !graficoF_->data()->isEmpty();
    trazoA_->setVisible(curvaDisponible);
    trazoB_->setVisible(curvaDisponible);
    trazoM_->setVisible(curvaDisponible);
    etiquetaA_->setVisible(curvaDisponible);
    etiquetaB_->setVisible(curvaDisponible);
    etiquetaM_->setVisible(curvaDisponible);
    if (curvaDisponible) {
        trazoA_->setGraph(graficoF_);
        trazoA_->setGraphKey(iteracion.a);
        trazoB_->setGraph(graficoF_);
        trazoB_->setGraphKey(iteracion.b);
        trazoM_->setGraph(graficoF_);
        trazoM_->setGraphKey(iteracion.m);

        etiquetaA_->position->setCoords(iteracion.a, iteracion.fa + desplazamiento);
        etiquetaB_->position->setCoords(iteracion.b, iteracion.fb + desplazamiento);
        etiquetaM_->position->setCoords(iteracion.m, iteracion.fm + desplazamiento);
    }

    replot();
}

void GraficoBiseccion::reposicionarMarcadores() {
    if (resultado_ && seleccionK_ > 0) {
        resaltarIteracion(seleccionK_);
    }
}

void GraficoBiseccion::ocultarResaltado() {
    zona_->setVisible(false);
    lineaA_->setVisible(false);
    lineaB_->setVisible(false);
    trazoA_->setVisible(false);
    trazoB_->setVisible(false);
    trazoM_->setVisible(false);
    etiquetaA_->setVisible(false);
    etiquetaB_->setVisible(false);
    etiquetaM_->setVisible(false);
}

bool GraficoBiseccion::tieneResultado() const {
    return resultado_ != nullptr;
}

void GraficoBiseccion::limpiar() {
    clearGraphs();
    clearItems();
    crearElementos();
    graficoF_ = nullptr;
    resultado_ = nullptr;
    funcion_ = {};
    marcadores_.clear();
    seleccionK_ = -1;
}

void GraficoBiseccion::mousePressEvent(QMouseEvent* evento) {
    QCustomPlot::mousePressEvent(evento);
    if (!resultado_ || marcadores_.empty()) {
        return;
    }
    // Punto del clic en píxeles (búsqueda de marcadores cercanos).
    double mejorDistancia = 13.0;
    int mejorK = -1;
    for (const auto& marcador : marcadores_) {
        const double dx = xAxis->coordToPixel(marcador[0]) - evento->position().x();
        const double dy = yAxis->coordToPixel(marcador[1]) - evento->position().y();
        const double d = std::hypot(dx, dy);
        if (d < mejorDistancia) {
            mejorDistancia = d;
            mejorK = static_cast<int>(marcador[2]);
        }
    }
    if (mejorK > 0) {
        Q_EMIT marcadorClickeado(mejorK);
    }
}

}  // namespace biseccion