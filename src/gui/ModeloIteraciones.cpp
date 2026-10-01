// ModeloIteraciones.cpp
// -----------------------------------------------------------------------------
// Implementación del modelo de tabla con formato dinámico de decimales.
// -----------------------------------------------------------------------------
#include "ModeloIteraciones.hpp"

#include <QString>

#include <algorithm>
#include <utility>

#include "core/CifrasSignificativas.hpp"

namespace biseccion {

ModeloIteraciones::ModeloIteraciones(QObject* padre) : QAbstractTableModel(padre) {}

void ModeloIteraciones::setResultado(Resultado resultado, int cifras) {
    beginResetModel();
    iteraciones_ = std::move(resultado.iteraciones);
    cifras_ = std::clamp(cifras, 1, 12);
    endResetModel();
}

void ModeloIteraciones::limpiar() {
    beginResetModel();
    iteraciones_.clear();
    endResetModel();
}

bool ModeloIteraciones::vacio() const {
    return iteraciones_.empty();
}

const Iteracion* ModeloIteraciones::iteracionPorK(int k) const {
    const auto resultado = std::find_if(
        iteraciones_.cbegin(), iteraciones_.cend(),
        [k](const Iteracion& iteracion) { return iteracion.k == k; });
    return resultado == iteraciones_.cend() ? nullptr : &(*resultado);
}

int ModeloIteraciones::kEnFila(int fila) const {
    if (fila < 0 || fila >= static_cast<int>(iteraciones_.size())) {
        return -1;
    }
    return iteraciones_[static_cast<std::size_t>(fila)].k;
}

int ModeloIteraciones::filaParaK(int k) const {
    for (std::size_t i = 0; i < iteraciones_.size(); ++i) {
        if (iteraciones_[i].k == k) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int ModeloIteraciones::rowCount(const QModelIndex& padre) const {
    return padre.isValid() ? 0 : static_cast<int>(iteraciones_.size());
}

int ModeloIteraciones::columnCount(const QModelIndex& padre) const {
    return padre.isValid() ? 0 : NumColumnas;
}

QVariant ModeloIteraciones::headerData(int seccion, Qt::Orientation orientacion, int rol) const {
    if (orientacion != Qt::Horizontal || rol != Qt::DisplayRole) {
        return QVariant();
    }
    switch (seccion) {
        case ColK: return tr("k");
        case ColA: return tr("a");
        case ColB: return tr("b");
        case ColM: return tr("m");
        case ColFm: return tr("f(m)");
        case ColEa: return tr("e\u2090 (%)");  // e subíndice a
        default: return QVariant();
    }
}

QVariant ModeloIteraciones::data(const QModelIndex& indice, int rol) const {
    if (!indice.isValid() ||
        indice.row() < 0 || indice.row() >= static_cast<int>(iteraciones_.size())) {
        return QVariant();
    }
    const Iteracion& iteracion = iteraciones_[static_cast<std::size_t>(indice.row())];

    if (rol == Qt::DisplayRole) {
        switch (indice.column()) {
            case ColK:
                return iteracion.k;
            case ColA:
                return QString::fromStdString(formatearParaCifras(iteracion.a, cifras_));
            case ColB:
                return QString::fromStdString(formatearParaCifras(iteracion.b, cifras_));
            case ColM:
                return QString::fromStdString(formatearParaCifras(iteracion.m, cifras_));
            case ColFm:
                return QString::fromStdString(formatearParaCifras(iteracion.fm, cifras_));
            case ColEa:
                return iteracion.eaPorcentaje
                           ? QString::fromStdString(
                                 formatearParaCifras(*iteracion.eaPorcentaje, cifras_))
                           : tr("\u2014");  // em dash: no hay e_a en la primera iteración
            default:
                return QVariant();
        }
    }

    if (rol == Qt::ToolTipRole) {
        // Precisión completa reservada para inspección (no contaminar la vista).
        switch (indice.column()) {
            case ColA: return QString::number(iteracion.a, 'g', 17);
            case ColB: return QString::number(iteracion.b, 'g', 17);
            case ColM: return QString::number(iteracion.m, 'g', 17);
            case ColFm: return QString::number(iteracion.fm, 'g', 17);
            case ColEa:
                return iteracion.eaPorcentaje ? QString::number(*iteracion.eaPorcentaje, 'g', 17)
                                              : tr("e_a no se calcula en la primera iteración");
            default: return QVariant();
        }
    }
    return QVariant();
}

Qt::ItemFlags ModeloIteraciones::flags(const QModelIndex& indice) const {
    return QAbstractTableModel::flags(indice) | Qt::ItemIsSelectable;
}

}  // namespace biseccion