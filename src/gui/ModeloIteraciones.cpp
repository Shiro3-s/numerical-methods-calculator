// ModeloIteraciones.cpp
// -----------------------------------------------------------------------------
// Modelo de tabla dirigido por descriptor: los encabezados y los campos a leer
// dependen del método (bisección vs Newton-Raphson).
// -----------------------------------------------------------------------------
#include "ModeloIteraciones.hpp"

#include <QString>

#include <algorithm>
#include <optional>
#include <string>
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
    // El descriptor manda sobre el tipo: es él quien decide qué campos existen.
    tipo_ = descriptor.tipo;
    cifras_ = std::clamp(cifras, 1, 12);
    prepararColumnas(descriptor, resultado.dimension(), resultado.normaP);
    endResetModel();
}

void ModeloIteraciones::prepararColumnas(const DescriptorMetodo& descriptor, int dimension,
                                         int normaP) {
    columnas_.clear();
    columnas_.reserve(descriptor.columnas.size());

    // Una columna de sistema declara "x" UNA vez, pero un sistema de 5 ecuaciones
    // tiene cinco componentes que mostrar. Se expande aquí en x₁…x_n, de modo que
    // el descriptor no necesita saber la dimensión y la tabla se adapta sola.
    for (const ColumnaMetodo& declarada : descriptor.columnas) {
        if (declarada.campo != CampoIteracion::VectorX) {
            // La cabecera de la norma lleva el p que se usó de verdad. El
            // descriptor la declara en genérico porque no lo conoce: el p lo elige
            // el usuario en el panel de entrada, y poner «‖Δx‖∞» fijo mentiría en
            // cuanto lo cambiara.
            ColumnaMetodo col = declarada;
            if (col.campo == CampoIteracion::Norma && descriptor.tipo == TipoResolucion::Sistema) {
                col.titulo = "‖Δx‖" + std::to_string(std::max(1, normaP));
            }
            columnas_.push_back(col);
            continue;
        }
        for (int componente = 0; componente < dimension; ++componente) {
            ColumnaMetodo sub = declarada;
            sub.indice = componente;
            // Rótulo con subíndice Unicode (x₁, x₂…): se lee como la tabla del
            // libro de texto y cabe en la cabecera. Lo pone el núcleo, para que
            // coincida con el rótulo que usa la narrativa del mismo componente.
            sub.titulo = etiquetaVariable(static_cast<std::size_t>(componente));
            columnas_.push_back(sub);
        }
    }
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

    // Una columna de sistema se resuelve por componente, no por valorCampo:
    // un vector no cabe en un double, y `indice` dice cuál de sus componentes se
    // pide.
    std::optional<double> v;
    if (col.campo == CampoIteracion::VectorX) {
        v = (tipo_ == TipoResolucion::Sistema && col.indice >= 0)
                 ? valorComponente(iteracion, static_cast<std::size_t>(col.indice))
                 : std::nullopt;
    } else {
        v = valorCampo(iteracion, col.campo, tipo_);
    }

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
