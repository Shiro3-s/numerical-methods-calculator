// ModeloIteraciones.cpp
// -----------------------------------------------------------------------------
// Modelo de tabla dirigido por descriptor: los encabezados y los campos a leer
// dependen del método (bisección vs Newton-Raphson).
// -----------------------------------------------------------------------------
#include "ModeloIteraciones.hpp"

#include <QString>

#include <algorithm>
#include <utility>

#include "core/Biseccion.hpp"
#include "core/CifrasSignificativas.hpp"

namespace biseccion {

ModeloIteraciones::ModeloIteraciones(QObject* padre) : QAbstractTableModel(padre) {}

void ModeloIteraciones::setResultado(const Resultado& resultado,
                                     const DescriptorMetodo& descriptor,
                                     int cifras) {
    beginResetModel();
    iteraciones_ = resultado.iteraciones;
    columnas_ = descriptor.columnas;
    // El descriptor manda sobre el tipo: es él quien decide qué campos existen.
    tipo_ = descriptor.tipo;
    cifras_ = std::clamp(cifras, 1, 12);
    endResetModel();
}

void ModeloIteraciones::setResultado(Resultado resultado, int cifras) {
    const Biseccion b(std::function<double(double)>{});
    setResultado(resultado, b.descriptor(), cifras);
}

void ModeloIteraciones::limpiar() {
    beginResetModel();
    iteraciones_.clear();
    columnas_.clear();
    tipo_ = TipoResolucion::RaizIntervalo;
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
    return padre.isValid() ? 0 : static_cast<int>(columnas_.size());
}

QVariant ModeloIteraciones::headerData(int seccion, Qt::Orientation orientacion, int rol) const {
    if (orientacion != Qt::Horizontal || rol != Qt::DisplayRole) {
        return QVariant();
    }
    if (seccion < 0 || static_cast<std::size_t>(seccion) >= columnas_.size()) {
        return QVariant();
    }
    return QString::fromStdString(columnas_[static_cast<std::size_t>(seccion)].titulo);
}

QVariant ModeloIteraciones::data(const QModelIndex& indice, int rol) const {
    if (!indice.isValid() ||
        indice.row() < 0 || indice.row() >= static_cast<int>(iteraciones_.size()) ||
        indice.column() < 0 || static_cast<std::size_t>(indice.column()) >= columnas_.size()) {
        return QVariant();
    }
    const Iteracion& iteracion = iteraciones_[static_cast<std::size_t>(indice.row())];
    const auto& col = columnas_[static_cast<std::size_t>(indice.column())];
    const auto v = valorCampo(iteracion, col.campo, tipo_);

    if (rol == Qt::DisplayRole) {
        if (!v.has_value()) {
            return tr("—");
        }
        const double val = *v;
        // K se muestra entero
        if (col.campo == CampoIteracion::K) {
            return static_cast<int>(std::lround(val));
        }
        return QString::fromStdString(formatearParaCifras(val, cifras_));
    }

    if (rol == Qt::ToolTipRole) {
        if (!v.has_value()) {
            return tr("no definido para este método");
        }
        return QString::number(*v, 'g', 17);
    }
    return QVariant();
}

Qt::ItemFlags ModeloIteraciones::flags(const QModelIndex& indice) const {
    return QAbstractTableModel::flags(indice) | Qt::ItemIsSelectable;
}

}  // namespace biseccion
