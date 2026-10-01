// VentanaPrincipal.hpp
// -----------------------------------------------------------------------------
// Ventana principal que integra los cuatro paneles:
//   1. Entrada   2. Gráfico interactivo   3. Tabla de iteraciones   4. Detalle
// Gestiona la resolución en segundo plano (std::jthread + stop_token).
// -----------------------------------------------------------------------------
#pragma once

#include <QMainWindow>
#include <QPointer>

#include <memory>
#include <stop_token>
#include <string>
#include <thread>

#include "core/Biseccion.hpp"
#include "core/Funcion.hpp"

class QSplitter;

namespace biseccion {

class GraficoBiseccion;
class ModeloIteraciones;
class PanelEntrada;
class PanelProcedimiento;
class TablaIteraciones;

class VentanaPrincipal : public QMainWindow {
    Q_OBJECT

public:
    explicit VentanaPrincipal(QWidget* padre = nullptr);

private slots:
    void iniciarResolucion(const QString& expresion, double a, double b, int cifras);
    void cancelarResolucion();
    void aplicarResultado(const std::shared_ptr<Resultado>& resultado,
                          const Funcion& funcion, int cifras);
    void manejarIteracionSeleccionada(int k);
    void manejarMarcadorClickeado(int k);

private:
    void construirInterfaz();
    void cargarEjercicios();

    PanelEntrada* panelEntrada_ = nullptr;
    GraficoBiseccion* grafico_ = nullptr;
    TablaIteraciones* tabla_ = nullptr;
    PanelProcedimiento* procedimiento_ = nullptr;
    ModeloIteraciones* modelo_ = nullptr;

    std::jthread hiloCalculo_;  // resolución en segundo plano
    bool calculando_ = false;

    std::string ultimoFuncionTexto_;
    int ultimasCifras_ = 6;
};

}  // namespace biseccion