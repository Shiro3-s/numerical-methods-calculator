// GraficoBiseccion.cpp
// -----------------------------------------------------------------------------
// Implementación del gráfico interactivo basado en QCustomPlot.
// Todo lo que se resalta depende del descriptor del método, no del nombre del
// panel: bisección sombrea un intervalo, Newton-Raphson traza una tangente.
// -----------------------------------------------------------------------------
#include "GraficoBiseccion.hpp"

#include <QMouseEvent>
#include <QOverload>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "core/Metodo.hpp"
#include "core/Resultado.hpp"

namespace biseccion {

namespace {
constexpr QColor COLOR_A(46, 139, 87);     // verde
constexpr QColor COLOR_B(255, 140, 0);     // naranja
constexpr QColor COLOR_M(20, 90, 200);     // azul
constexpr QColor COLOR_TANGENTE(190, 30, 90);  // magenta: recta tangente

// Un método de punto inicial no rellena a, b, fa, fb: los deja en cero. Para
// saber si esos campos aplican de verdad hay que preguntar al descriptor, no
// mirar el valor (un cero legítimo es indistinguible de un campo sin usar).
constexpr bool usaIntervalo(TipoResolucion tipo) {
    return tipo == TipoResolucion::RaizIntervalo;
}
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

    // Recta tangente en x_k: la idea gráfica de Newton-Raphson. Se dibuja como
    // un segmento largo que atraviesa el punto (x_k, f(x_k)) con pendiente f'(x_k).
    tangente_ = new QCPItemLine(this);
    tangente_->start->setType(QCPItemPosition::ptPlotCoords);
    tangente_->end->setType(QCPItemPosition::ptPlotCoords);
    tangente_->setPen(QPen(COLOR_TANGENTE, 1.6));
    tangente_->setVisible(false);

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

    // Etiquetas de los puntos. La del punto aproximado toma el nombre que da
    // el descriptor: «m» en bisección, «x» en Newton-Raphson.
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
    const QString nombreRaiz =
        QString::fromStdString(descriptor_.etiquetaRaiz.empty() ? "m" : descriptor_.etiquetaRaiz);
    configurarEtiqueta(etiquetaM_, nombreRaiz, COLOR_M.darker(120));
}

std::pair<double, double> GraficoBiseccion::rangoX() const {
    if (!resultado_) {
        return {-1.0, 1.0};
    }
    const bool intervalo = usaIntervalo(descriptor_.tipo);
    double minimo = std::numeric_limits<double>::infinity();
    double maximo = -std::numeric_limits<double>::infinity();

    const auto considerar = [&minimo, &maximo](double v) {
        if (std::isfinite(v)) {
            minimo = std::min(minimo, v);
            maximo = std::max(maximo, v);
        }
    };

    for (const Iteracion& it : resultado_->iteraciones) {
        if (intervalo) {
            considerar(it.a);
            considerar(it.b);
        }
        if (it.x) {
            considerar(*it.x);
        } else {
            considerar(it.m);
        }
    }
    // Sin nada que medir (o todo no finito): rango neutro antes de muestrear.
    if (!(std::isfinite(minimo) && std::isfinite(maximo)) || maximo <= minimo) {
        return {-1.0, 1.0};
    }
    return {minimo, maximo};
}

void GraficoBiseccion::cargarResultado(const std::shared_ptr<const Resultado>& puntero,
                                       const DescriptorMetodo& descriptor,
                                       const std::function<double(double)>& f,
                                       const std::function<double(double)>& df,
                                       int cifras, double margenX) {
    // Retener el shared_ptr mantiene el Resultado vivo mientras esté dibujado:
    // VentanaPrincipal re-resuelve y reemplaza su propio shared_ptr sin dejar
    // un puntero colgante en el gráfico (evita heap-use-after-free al hacer
    // clic sobre la tabla o los marcadores).
    if (!puntero || !f) {
        limpiar();
        return;
    }
    resultado_ = puntero;
    funcion_ = f;
    derivada_ = df;
    descriptor_ = descriptor;
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
    const auto [xMinimo, xMaximo] = rangoX();
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
        const double x = xMinimo - pad
                         + (static_cast<double>(i) / muestras) * ((xMaximo + pad) - (xMinimo - pad));
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

    // Registro de marcadores clicables de todas las iteraciones. Solo se anotan
    // los puntos que el método realmente visitó: en Newton-Raphson a y b no
    // existen, y registrar (0, 0) haría clicables dos puntos falsos.
    marcadores_.clear();
    const bool intervalo = usaIntervalo(descriptor_.tipo);
    for (const Iteracion& iteracion : resultado.iteraciones) {
        if (intervalo) {
            marcadores_.push_back({iteracion.a, iteracion.fa, static_cast<double>(iteracion.k)});
            marcadores_.push_back({iteracion.b, iteracion.fb, static_cast<double>(iteracion.k)});
        }
        const double px = iteracion.x.value_or(iteracion.m);
        const double fpx = iteracion.fx.value_or(iteracion.fm);
        if (std::isfinite(px) && std::isfinite(fpx)) {
            marcadores_.push_back({px, fpx, static_cast<double>(iteracion.k)});
        }
    }

    // Mostrar el gráfico solo si hay algo que dibujar (curva o datos finitos)
    setVisible(true);
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
    const double semianchoX = 0.5 * (xAxis->range().upper - xAxis->range().lower);

    const bool intervalo = usaIntervalo(descriptor_.tipo);
    const bool curvaDisponible = graficoF_ != nullptr && !graficoF_->data()->isEmpty();

    // ---- Intervalo [a, b]: solo tiene sentido en un método de intervalo ----
    zona_->setVisible(intervalo);
    lineaA_->setVisible(intervalo);
    lineaB_->setVisible(intervalo);
    if (intervalo) {
        zona_->topLeft->setCoords(iteracion.a, yMaximo);
        zona_->bottomRight->setCoords(iteracion.b, yMinimo);
        lineaA_->start->setCoords(iteracion.a, yMinimo);
        lineaA_->end->setCoords(iteracion.a, yMaximo);
        lineaB_->start->setCoords(iteracion.b, yMinimo);
        lineaB_->end->setCoords(iteracion.b, yMaximo);
    }

    // ---- Recta tangente: el equivalente en Newton-Raphson ----
    // y − f(x_k) = f'(x_k)·(x − x_k), recortada al ancho visible del gráfico.
    const bool puedeTrazar = descriptor_.muestraTrazado && derivada_ && iteracion.x
                             && iteracion.fx && iteracion.fdx && std::isfinite(*iteracion.fdx);
    tangente_->setVisible(puedeTrazar);
    if (puedeTrazar) {
        const double x0 = *iteracion.x;
        const double y0 = *iteracion.fx;
        const double m = *iteracion.fdx;
        tangente_->start->setCoords(x0 - semianchoX, y0 - m * semianchoX);
        tangente_->end->setCoords(x0 + semianchoX, y0 + m * semianchoX);
    }

    // ---- Puntos sobre la curva ----
    const double px = iteracion.x.value_or(iteracion.m);
    const double fpx = iteracion.fx.value_or(iteracion.fm);
    trazoA_->setVisible(curvaDisponible && intervalo);
    trazoB_->setVisible(curvaDisponible && intervalo);
    trazoM_->setVisible(curvaDisponible && std::isfinite(px));
    etiquetaA_->setVisible(trazoA_->visible());
    etiquetaB_->setVisible(trazoB_->visible());
    etiquetaM_->setVisible(trazoM_->visible());
    if (trazoA_->visible()) {
        trazoA_->setGraph(graficoF_);
        trazoA_->setGraphKey(iteracion.a);
        etiquetaA_->position->setCoords(iteracion.a, iteracion.fa + desplazamiento);
    }
    if (trazoB_->visible()) {
        trazoB_->setGraph(graficoF_);
        trazoB_->setGraphKey(iteracion.b);
        etiquetaB_->position->setCoords(iteracion.b, iteracion.fb + desplazamiento);
    }
    if (trazoM_->visible()) {
        trazoM_->setGraph(graficoF_);
        trazoM_->setGraphKey(px);
        etiquetaM_->position->setCoords(px, fpx + desplazamiento);
    }

    replot();
}

void GraficoBiseccion::reposicionarMarcadores() {
    if (resultado_ && seleccionK_ > 0) {
        resaltarIteracion(seleccionK_);
    }
}

bool GraficoBiseccion::tieneResultado() const {
    return resultado_ != nullptr;
}

bool GraficoBiseccion::muestraTrazado() const {
    return tangente_ != nullptr && tangente_->visible();
}

void GraficoBiseccion::limpiar() {
    clearGraphs();
    clearItems();
    crearElementos();
    graficoF_ = nullptr;
    resultado_ = nullptr;
    funcion_ = {};
    derivada_ = {};
    marcadores_.clear();
    seleccionK_ = -1;
    // Sin curva que mostrar, el panel se oculta: es mejor que la tabla ocupe
    // todo el espacio disponible que dejar un lienzo vacío con dos ejes.
    setVisible(false);
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