// PanelEntrada.hpp
// -----------------------------------------------------------------------------
// Panel 1 · Entrada de la función: f(x), extremos [a, b], cifras significativas,
// presets de los cuatro ejercicios y control Resolver / Cancelar.
// -----------------------------------------------------------------------------
#pragma once

#include <QWidget>

#include <vector>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

namespace biseccion {

struct DescriptorMetodo;
struct Entrada;

class PanelEntrada : public QWidget {
    Q_OBJECT

public:
    explicit PanelEntrada(QWidget* padre = nullptr);

    // Carga el catálogo de métodos en el selector.
    void cargarMetodos();

    // Habilita o bloquea los controles según el estado de cálculo.
    void habilitarEjecucion(bool habilitado);

    // Devuelve el descriptor del método seleccionado actualmente.
    [[nodiscard]] DescriptorMetodo descriptorSeleccionado() const;

signals:
    // Señal dirigida por descriptor: envía la Entrada completa.
    void resolverSolicitado(const Entrada& entrada, const DescriptorMetodo& descriptor);
    void cancelarSolicitado();

private slots:
    void metodoCambiado(int indice);
    void modoResolver();
    void modoCancelar();
    void actualizarInfo();

private:
    void configurarParaDescriptor(const DescriptorMetodo& d);

    QComboBox* comboMetodo_ = nullptr;
    QLineEdit* campoFuncion_ = nullptr;
    QLineEdit* campoFuncionAux_ = nullptr;  // f'(x), g(x), etc.
    QLabel* etiquetaAux_ = nullptr;
    QDoubleSpinBox* spinA_ = nullptr;
    QDoubleSpinBox* spinB_ = nullptr;
    QDoubleSpinBox* spinX0_ = nullptr;
    QLabel* etiquetaA_ = nullptr;
    QLabel* etiquetaB_ = nullptr;
    QLabel* etiquetaX0_ = nullptr;
    QSpinBox* spinCifras_ = nullptr;
    QPushButton* botonResolver_ = nullptr;
    QPushButton* botonCancelar_ = nullptr;
    QLabel* etiquetaDescripcion_ = nullptr;
    QLabel* etiquetaInfo_ = nullptr;
    bool resolviendo_ = false;

    std::vector<DescriptorMetodo> descriptores_;
};

}  // namespace biseccion