// GraficoBiseccion.hpp
// -----------------------------------------------------------------------------
// Panel 2 · Gráfico interactivo (estilo GeoGebra). Dibuja f(x), resalta el
// intervalo [a, b] de la iteración seleccionada con sus puntos a, b y m, y
// permite hacer clic sobre los marcadores para volver a la fila equivalente.
// -----------------------------------------------------------------------------
#pragma once

#include <qcustomplot.h>

#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace biseccion {

class Resultado;

class GraficoBiseccion : public QCustomPlot {
    Q_OBJECT

public:
    explicit GraficoBiseccion(QWidget* padre = nullptr);

    // Carga el resultado y dibuja la curva f(x) sobre el rango de las
    // iteraciones. También registra los marcadores clicables.
    // Recibe un shared_ptr para retener el resultado mientras esté dibujado:
    // la ventana principal puede re-resolver y reemplazar el suyo sin
    // dejar un puntero colgante en el gráfico.
    void cargarResultado(const std::shared_ptr<const Resultado>& resultado,
                         const std::function<double(double)>& f,
                         int cifras,
                         double margenX = 0.4);

    // Resalta el intervalo [a, b] y los puntos a, b, m de la iteración 'k'.
    void resaltarIteracion(int k);

    void limpiar();
    [[nodiscard]] bool tieneResultado() const;

signals:
    void marcadorClickeado(int k);

protected:
    void mousePressEvent(QMouseEvent* evento) override;

private:
    void crearElementos();
    void ocultarResaltado();
    void reposicionarMarcadores();

    std::function<double(double)> funcion_;  // copia para (re)evaluar f(b), etc.
    // El gráfico retiene el resultado con shared_ptr para que siga vivo mientras
    // esté dibujado: VentanaPrincipal re-resuelve y reemplaza su propio shared_ptr
    // sin dejar un puntero colgante (evita heap-use-after-free al hacer clic).
    std::shared_ptr<const Resultado> resultado_;
    int cifras_ = 6;
    int seleccionK_ = -1;

    QCPGraph* graficoF_ = nullptr;
    QCPItemRect* zona_ = nullptr;          // sombreado del intervalo [a, b]
    QCPItemLine* lineaA_ = nullptr;        // guía vertical en a
    QCPItemLine* lineaB_ = nullptr;        // guía vertical en b
    QCPItemTracer* trazoA_ = nullptr;
    QCPItemTracer* trazoB_ = nullptr;
    QCPItemTracer* trazoM_ = nullptr;
    QCPItemText* etiquetaA_ = nullptr;
    QCPItemText* etiquetaB_ = nullptr;
    QCPItemText* etiquetaM_ = nullptr;

    // Marcadores de todas las iteraciones como {x, y, k} para el clic.
    std::vector<std::array<double, 3>> marcadores_;
};

}  // namespace biseccion