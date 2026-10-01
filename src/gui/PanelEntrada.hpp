// PanelEntrada.hpp
// -----------------------------------------------------------------------------
// Panel 1 · Entrada de la función: f(x), extremos [a, b], cifras significativas,
// presets de los cuatro ejercicios y control Resolver / Cancelar.
// -----------------------------------------------------------------------------
#pragma once

#include <QWidget>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

namespace biseccion {

class PanelEntrada : public QWidget {
    Q_OBJECT

public:
    explicit PanelEntrada(QWidget* padre = nullptr);

    // Carga los presets E1–E4 en el selector y aplica el primero.
    void cargarPresets();

    // Habilita o bloquea los controles según el estado de cálculo.
    void habilitarEjecucion(bool habilitado);

signals:
    void resolverSolicitado(const QString& expresion, double a, double b, int cifras);
    void cancelarSolicitado();

private slots:
    void aplicarEjercicio(int indice);
    void modoResolver();
    void modoCancelar();
    void actualizarInfo();
    void marcarFuncionPersonalizada();

private:
    void aplicarFuncion(const QString& expresion, double a, double b, int cifras,
                        const QString& descripcion);
    void aplicarModoLibre();

    QComboBox* comboEjercicios_ = nullptr;
    QLineEdit* campoFuncion_ = nullptr;
    QDoubleSpinBox* spinA_ = nullptr;
    QDoubleSpinBox* spinB_ = nullptr;
    QSpinBox* spinCifras_ = nullptr;
    QPushButton* botonResolver_ = nullptr;
    QPushButton* botonCancelar_ = nullptr;
    QLabel* etiquetaDescripcion_ = nullptr;
    QLabel* etiquetaInfo_ = nullptr;
    bool resolviendo_ = false;
// Se activa cuando el usuario edita f(x) a mano: recuerda que el intervalo
    // [a, b] puede necesitar revisión para seguir conteniendo la raíz.
    bool funcionPersonalizada_ = false;
    // «Calculadora libre»: el usuario digita f(x) y el intervalo por completo;
    // no conserva ninguna información de un preset.
    bool modoLibre_ = false;
};

}  // namespace biseccion