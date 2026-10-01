// TablaIteraciones.hpp
// -----------------------------------------------------------------------------
// Panel 3 · Tabla de iteraciones. Al seleccionar una fila se emite la señal
// iteracionSeleccionada(k) que activa el gráfico y el panel procedimental.
// -----------------------------------------------------------------------------
#pragma once

#include <QTableView>

namespace biseccion {

class ModeloIteraciones;

class TablaIteraciones : public QTableView {
    Q_OBJECT

public:
    explicit TablaIteraciones(QWidget* padre = nullptr);

    void vincularModelo(ModeloIteraciones* modelo);

    // Selecciona la fila de la iteración 'k' (usado al hacer clic en el gráfico).
    void seleccionarIteracion(int k);

signals:
    void iteracionSeleccionada(int k);
    void sinSeleccion();

protected:
    void selectionChanged(const QItemSelection& seleccionada,
                          const QItemSelection& anterior) override;

private:
    ModeloIteraciones* modelo_ = nullptr;
};

}  // namespace biseccion