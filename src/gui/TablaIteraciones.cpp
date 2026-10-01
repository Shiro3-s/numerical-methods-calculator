// TablaIteraciones.cpp
// -----------------------------------------------------------------------------
// Implementación de la tabla; la selección de fila dispara el resto de paneles.
// -----------------------------------------------------------------------------
#include "TablaIteraciones.hpp"

#include <QHeaderView>

#include "ModeloIteraciones.hpp"

namespace biseccion {

TablaIteraciones::TablaIteraciones(QWidget* padre) : QTableView(padre) {
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    horizontalHeader()->setStretchLastSection(true);
}

void TablaIteraciones::vincularModelo(ModeloIteraciones* modelo) {
    modelo_ = modelo;
    setModel(modelo);
}

void TablaIteraciones::seleccionarIteracion(int k) {
    if (!modelo_) {
        return;
    }
    const int fila = modelo_->filaParaK(k);
    if (fila < 0) {
        return;
    }
    selectRow(fila);
    scrollTo(modelo_->index(fila, 0));
}

void TablaIteraciones::selectionChanged(const QItemSelection& seleccionada,
                                        const QItemSelection& anterior) {
    QTableView::selectionChanged(seleccionada, anterior);
    if (!modelo_) {
        return;
    }
    const QModelIndex indice = currentIndex();
    if (indice.isValid()) {
        const int k = modelo_->kEnFila(indice.row());
        if (k > 0) {
            Q_EMIT iteracionSeleccionada(k);
            return;
        }
    }
    Q_EMIT sinSeleccion();
}

}  // namespace biseccion