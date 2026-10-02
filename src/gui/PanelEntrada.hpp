// PanelEntrada.hpp
// -----------------------------------------------------------------------------
// Panel 1 · Entrada: selector de método (no de ejercicios), campos dinámicos
// según el descriptor (intervalo [a,b] vs x₀ vs matriz del sistema, expresión
// auxiliar f'(x)…) y señal dirigida por descriptor (Entrada + DescriptorMetodo).
// -----------------------------------------------------------------------------
#pragma once

#include <QWidget>

#include <vector>

class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;

namespace biseccion {

struct DescriptorMetodo;
struct Entrada;
struct Reordenamiento;

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

    // Rellena la matriz y la iterada inicial con el sistema 3×3 dominante clásico,
    // como punto de partida listo para resolver. Lo usan las pruebas y el
    // autotest por consola; el usuario la teclea o la edita a mano.
    void cargarSistemaDeEjemplo();

signals:
    // Señal dirigida por descriptor: envía la Entrada completa.
    void resolverSolicitado(const Entrada& entrada, const DescriptorMetodo& descriptor);
    void cancelarSolicitado();

    // El alto que el panel necesita cambia según el método: al pasar a un método
    // de sistema aparece la rejilla, y al subir el número de ecuaciones crece.
    // Un QSplitter reparte los tamaños una sola vez, así que sin esta señal la
    // ventana se queda con el reparto inicial y aplasta la rejilla.
    void altoRequeridoCambiado();

private slots:
    void metodoCambiado(int indice);
    void modoResolver();
    void modoCancelar();
    void actualizarInfo();
    void dimensionCambiada(int valor);

private:
    void configurarParaDescriptor(const DescriptorMetodo& d);
    // Reconstruye la rejilla al cambiar n, y vuelve a pintar el ejemplo.
    void reconstruirSistema();
    // Lee la rejilla a la Entrada (matriz, términos, iterada inicial).
    void volcarSistemaEnEntrada(Entrada& e) const;
    // ¿Es estrictamente diagonalmente dominante lo que hay ahora en la rejilla?
    [[nodiscard]] bool sistemaDominante() const;
    // Qué intercambios de filas harían falta para llegar a la dominante, sin
    // tocar la rejilla. Es el paso 1 del procedimiento, anticipado: el usuario ve
    // qué va a pasar antes de pulsar Resolver, no después.
    [[nodiscard]] Reordenamiento reordenamientoPrevisto() const;
    // Escribe la definición de la norma con el p que hay elegido, que es donde el
    // enunciado pide que P quede definido.
    void actualizarDefinicionNorma();

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
    QLabel* etiquetaFuncion_ = nullptr;

    // Sistema lineal (método de Jacobi). Se oculta entero mientras el método
    // seleccionado no sea de sistema.
    QGroupBox* cajaSistema_ = nullptr;
    QSpinBox* spinDimension_ = nullptr;
    // Orden p de la norma del error. Es el punto donde el enunciado pide que P
    // quede definido, así que es un campo visible con su fórmula al lado, no un
    // valor enterrado en el código.
    QSpinBox* spinNormaP_ = nullptr;
    QLabel* etiquetaDefinicionNorma_ = nullptr;
    QTableWidget* tablaSistema_ = nullptr;

    bool resolviendo_ = false;

    std::vector<DescriptorMetodo> descriptores_;
};

}  // namespace biseccion