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
#include <optional>
#include <stop_token>
#include <string>
#include <thread>

#include "core/Biseccion.hpp"
#include "core/Funcion.hpp"
#include "core/Metodo.hpp"

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

    // Ejecuciones predefinidas (autotest y pruebas de interfaz offscreen):
    // saltan el formulario y lanzan el método con una entrada válida.
    Q_INVOKABLE void resolverPruebaBiseccion();
    Q_INVOKABLE void resolverPruebaNewton();
    Q_INVOKABLE void resolverPruebaJacobi();

private slots:
    void iniciarResolucion(const Entrada& entrada, const DescriptorMetodo& descriptor);
    void cancelarResolucion();
    void aplicarResultado(const std::shared_ptr<Resultado>& resultado,
                          std::optional<ErrorMetodo> error,
                          const Entrada& entrada,
                          const DescriptorMetodo& descriptor);
    void manejarIteracionSeleccionada(int k);
    void manejarMarcadorClickeado(int k);
    // Reparte de nuevo el alto del panel izquierdo cuando el panel de entrada
    // crece o mengua (aparece la rejilla del sistema o sube el número de
    // ecuaciones). Un QSplitter solo reparte una vez, al construirse.
    void ajustarAltoPanelEntrada();

private:
    void construirInterfaz();
    void cargarMetodos();
    void mostrarErrorAnalisis(const std::string& mensaje, const QString& consejo);
    [[nodiscard]] QString escapar(QString texto) const;
    [[nodiscard]] QString valoresEnExtremos(const Entrada& entrada) const;

    PanelEntrada* panelEntrada_ = nullptr;
    // Splitter vertical izquierdo: reparte el alto entre entrada y procedimiento.
    QSplitter* divisionIzquierda_ = nullptr;
    GraficoBiseccion* grafico_ = nullptr;
    TablaIteraciones* tabla_ = nullptr;
    PanelProcedimiento* procedimiento_ = nullptr;
    ModeloIteraciones* modelo_ = nullptr;

    std::jthread hiloCalculo_;  // resolución en segundo plano
    bool calculando_ = false;

    std::string ultimoFuncionTexto_;
    int ultimasCifras_ = 6;
    DescriptorMetodo ultimoDescriptor_;
    // Último resultado mostrado. Se retiene (además de en la tabla) porque la
    // narración de una iteración necesita el sistema completo cuando el método es
    // de sistemas: con solo la fila no hay forma de escribir el despeje.
    std::shared_ptr<const Resultado> ultimoResultado_;
    // Texto de f(x) tal como quedó tras el análisis (espacios normalizados):
    // lo usan el resumen, el panel procedimental y el re-análisis del gráfico.
    std::string ultimoTextoF_;
    // 'cifras' viaja al aplicar el resultado a través de un lambda que corre en
    // el hilo de la interfaz; este miembro lo lleva hasta allí.
    int ultimasCifrasPendientes_ = 6;
};

}  // namespace biseccion