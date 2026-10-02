// GraficoBiseccion.hpp
// -----------------------------------------------------------------------------
// Panel 2 · Gráfico interactivo (estilo GeoGebra). Dibuja f(x) y resalta la
// iteración seleccionada. QUÉ se resalta lo decide el descriptor del método:
//   · método de intervalo (bisección) → zona [a, b], guías en a y b, puntos a·b·m
//   · método de punto inicial (Newton) → el punto x_k y la RECTA TANGENTE, que
//     es justamente la idea gráfica del método
// En ambos casos, hacer clic sobre un marcador vuelve a la fila equivalente.
// Cuando el método no dibuja nada útil (un sistema lineal, p. ej.) el gráfico
// se oculta entero en vez de mostrar un panel vacío.
// -----------------------------------------------------------------------------
#pragma once

#include <qcustomplot.h>

#include <array>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "core/Metodo.hpp"
#include "core/Resultado.hpp"

namespace biseccion {

class GraficoBiseccion : public QCustomPlot {
    Q_OBJECT

public:
    explicit GraficoBiseccion(QWidget* padre = nullptr);

    // Carga el resultado y dibuja la curva f(x) sobre el rango alcanzado por
    // las iteraciones. 'df' es opcional: si el descriptor pide trazado (la
    // tangente de Newton) y se aporta derivada, se dibuja la recta en x_k.
    //
    // Recibe un shared_ptr para retener el resultado mientras esté dibujado:
    // la ventana principal puede re-resolver y reemplazar el suyo sin
    // dejar un puntero colgante en el gráfico.
    void cargarResultado(const std::shared_ptr<const Resultado>& resultado,
                         const DescriptorMetodo& descriptor,
                         const std::function<double(double)>& f,
                         const std::function<double(double)>& df,
                         int cifras,
                         double margenX = 0.4);

    // Resalta lo que corresponda de la iteración 'k' (intervalo o punto y tangente).
    void resaltarIteracion(int k);

    void limpiar();
    [[nodiscard]] bool tieneResultado() const;
    // Si la recta asociada (tangente de Newton-Raphson) está dibujada ahora.
    [[nodiscard]] bool muestraTrazado() const;

signals:
    void marcadorClickeado(int k);

protected:
    void mousePressEvent(QMouseEvent* evento) override;

private:
    void crearElementos();
    void reposicionarMarcadores();
    // Rango horizontal que abarcan las iteraciones del método actual.
    [[nodiscard]] std::pair<double, double> rangoX() const;

    std::function<double(double)> funcion_;  // copia de f(x)
    std::function<double(double)> derivada_;  // f'(x), solo si el método la usa
    DescriptorMetodo descriptor_;
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
    QCPItemLine* tangente_ = nullptr;      // recta tangente en x_k (Newton)
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