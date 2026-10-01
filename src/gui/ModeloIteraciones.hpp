// ModeloIteraciones.hpp
// -----------------------------------------------------------------------------
// Modelo de tabla para las iteraciones del método (vista QTableView).
// El número de decimales mostrado se limita dinámicamente según las cifras
// significativas calculadas (paso 5).
// -----------------------------------------------------------------------------
#pragma once

#include <QAbstractTableModel>

#include <vector>

#include "core/Biseccion.hpp"

namespace biseccion {

class ModeloIteraciones : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Columna {
        ColK = 0,
        ColA,
        ColB,
        ColM,
        ColFm,
        ColEa,
        NumColumnas,
    };

    explicit ModeloIteraciones(QObject* padre = nullptr);

    void setResultado(Resultado resultado, int cifras);
    void limpiar();
    [[nodiscard]] bool vacio() const;

    [[nodiscard]] const Iteracion* iteracionPorK(int k) const;
    [[nodiscard]] int kEnFila(int fila) const;
    [[nodiscard]] int filaParaK(int k) const;

    // Reimplementaciones de QAbstractTableModel.
    int rowCount(const QModelIndex& padre = QModelIndex()) const override;
    int columnCount(const QModelIndex& padre = QModelIndex()) const override;
    QVariant headerData(int seccion, Qt::Orientation orientacion,
                        int rol = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex& indice, int rol = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& indice) const override;

private:
    std::vector<Iteracion> iteraciones_;
    int cifras_ = 6;
};

}  // namespace biseccion