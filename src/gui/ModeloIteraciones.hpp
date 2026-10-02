// ModeloIteraciones.hpp
// -----------------------------------------------------------------------------
// Modelo de tabla para las iteraciones del método (vista QTableView).
// El número de decimales mostrado se limita dinámicamente según las cifras
// significativas calculadas (paso 5).
// -----------------------------------------------------------------------------
#pragma once

#include <QAbstractTableModel>

#include <vector>

#include "core/Resultado.hpp"
#include "core/Metodo.hpp"

namespace biseccion {

class ModeloIteraciones : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit ModeloIteraciones(QObject* padre = nullptr);

    // Carga resultado con las columnas del descriptor del método.
    void setResultado(const Resultado& resultado, const DescriptorMetodo& descriptor, int cifras);
    void setResultado(Resultado resultado, int cifras);  // compatibilidad (usará bisección)
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
    // Expande las columnas "de familia" (hoy CampoIteracion::VectorX) en una
    // subcolumna por componente, usando la dimensión real del vector. Se llama
    // desde setResultado, antes de beginResetModel.
    void prepararColumnas(const DescriptorMetodo& descriptor, int dimension, int normaP);

    std::vector<Iteracion> iteraciones_;
    std::vector<ColumnaMetodo> columnas_;
    TipoResolucion tipo_ = TipoResolucion::RaizIntervalo;
    int cifras_ = 6;
};

}  // namespace biseccion