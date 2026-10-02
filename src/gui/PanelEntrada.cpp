// PanelEntrada.cpp
// -----------------------------------------------------------------------------
// Panel 1 · Entrada: selector de método (no de ejercicios), campos dinámicos
// según el descriptor (intervalo [a,b] vs x0, expresión auxiliar f'(x)...) y
// señal dirigida por descriptor (Entrada + DescriptorMetodo).
// -----------------------------------------------------------------------------
#include "PanelEntrada.hpp"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QOverload>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

#include "core/Metodo.hpp"
#include "core/Biseccion.hpp"

namespace biseccion {

PanelEntrada::PanelEntrada(QWidget* padre) : QWidget(padre) {
    auto* caja = new QGroupBox(tr("Entrada de datos"), this);
    auto* formulario = new QFormLayout(caja);

    comboMetodo_ = new QComboBox(caja);
    formulario->addRow(tr("Método:"), comboMetodo_);

    campoFuncion_ = new QLineEdit(caja);
    campoFuncion_->setPlaceholderText(
        tr("p. ej. 5x - 2sin(x) + x^2  ·  sin, cos, asin, ln, log, abs…"));
    formulario->addRow(tr("f(x) ="), campoFuncion_);

    etiquetaAux_ = new QLabel(tr("f'(x) ="), caja);
    campoFuncionAux_ = new QLineEdit(caja);
    campoFuncionAux_->setPlaceholderText(tr("p. ej. 1 + sin(x)"));
    formulario->addRow(etiquetaAux_, campoFuncionAux_);

    etiquetaA_ = new QLabel(tr("a (extremo inferior):"), caja);
    spinA_ = new QDoubleSpinBox(caja);
    spinA_->setRange(-1.0e6, 1.0e6);
    spinA_->setDecimals(6);
    formulario->addRow(etiquetaA_, spinA_);

    etiquetaB_ = new QLabel(tr("b (extremo superior):"), caja);
    spinB_ = new QDoubleSpinBox(caja);
    spinB_->setRange(-1.0e6, 1.0e6);
    spinB_->setDecimals(6);
    formulario->addRow(etiquetaB_, spinB_);

    etiquetaX0_ = new QLabel(tr("x₀ (iterada inicial):"), caja);
    spinX0_ = new QDoubleSpinBox(caja);
    spinX0_->setRange(-1.0e6, 1.0e6);
    spinX0_->setDecimals(6);
    spinX0_->setValue(1.0);
    formulario->addRow(etiquetaX0_, spinX0_);

    spinCifras_ = new QSpinBox(caja);
    spinCifras_->setRange(1, 12);
    spinCifras_->setValue(6);
    formulario->addRow(tr("Cifras significativas (n):"), spinCifras_);

    etiquetaDescripcion_ = new QLabel(caja);
    etiquetaDescripcion_->setWordWrap(true);
    formulario->addRow(etiquetaDescripcion_);

    auto* filaBotones = new QHBoxLayout();
    botonResolver_ = new QPushButton(tr("Resolver"), caja);
    botonResolver_->setDefault(true);
    botonCancelar_ = new QPushButton(tr("Cancelar"), caja);
    botonCancelar_->setVisible(false);
    filaBotones->addWidget(botonResolver_);
    filaBotones->addWidget(botonCancelar_);

    etiquetaInfo_ = new QLabel(caja);
    etiquetaInfo_->setWordWrap(true);
    etiquetaInfo_->setStyleSheet(QStringLiteral("color: #444; font-size: 11px;"));

    auto* diseno = new QVBoxLayout(this);
    diseno->addWidget(caja);
    diseno->addLayout(filaBotones);
    diseno->addWidget(etiquetaInfo_);
    diseno->addStretch(1);

    connect(comboMetodo_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &PanelEntrada::metodoCambiado);
    connect(botonResolver_, &QPushButton::clicked, this, &PanelEntrada::modoResolver);
    connect(botonCancelar_, &QPushButton::clicked, this, &PanelEntrada::modoCancelar);
    connect(spinCifras_, qOverload<int>(&QSpinBox::valueChanged),
            this, &PanelEntrada::actualizarInfo);
    connect(spinA_, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &PanelEntrada::actualizarInfo);
    connect(spinB_, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &PanelEntrada::actualizarInfo);
    connect(spinX0_, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &PanelEntrada::actualizarInfo);
}

void PanelEntrada::cargarMetodos() {
    comboMetodo_->blockSignals(true);
    comboMetodo_->clear();
    descriptores_.clear();

    for (auto& m : catalogoMetodos()) {
        descriptores_.push_back(m->descriptor());
    }

    for (const auto& d : descriptores_) {
        comboMetodo_->addItem(QString::fromStdString(d.nombre));
    }

    comboMetodo_->setCurrentIndex(0);
    comboMetodo_->blockSignals(false);

    if (!descriptores_.empty()) {
        configurarParaDescriptor(descriptores_[0]);
    }
    actualizarInfo();
}

void PanelEntrada::metodoCambiado(int indice) {
    if (indice < 0 || static_cast<std::size_t>(indice) >= descriptores_.size()) {
        return;
    }
    configurarParaDescriptor(descriptores_[static_cast<std::size_t>(indice)]);
    actualizarInfo();
}

void PanelEntrada::configurarParaDescriptor(const DescriptorMetodo& d) {
    etiquetaDescripcion_->setText(QString::fromStdString(d.descripcion));

    const bool requiereAux = d.requiereExpresionAuxiliar;
    etiquetaAux_->setVisible(requiereAux);
    campoFuncionAux_->setVisible(requiereAux);
    if (requiereAux) {
        etiquetaAux_->setText(QString::fromStdString(d.etiquetaAuxiliar.empty() ? "f'(x) =" : (d.etiquetaAuxiliar + " =")));
        campoFuncionAux_->clear();
    }

    const bool intervalo = (d.tipo == TipoResolucion::RaizIntervalo);
    const bool punto = (d.tipo == TipoResolucion::RaizPuntoInicial);
    etiquetaA_->setVisible(intervalo);
    spinA_->setVisible(intervalo);
    etiquetaB_->setVisible(intervalo);
    spinB_->setVisible(intervalo);
    etiquetaX0_->setVisible(punto);
    spinX0_->setVisible(punto);

    // Sistema (futuro): por ahora no mostramos campos de matriz; gráfico oculto
    // queda preparado para cuando se implemente Jacobi.
}

DescriptorMetodo PanelEntrada::descriptorSeleccionado() const {
    const int i = comboMetodo_->currentIndex();
    if (i < 0 || static_cast<std::size_t>(i) >= descriptores_.size()) {
        return DescriptorMetodo{};
    }
    return descriptores_[static_cast<std::size_t>(i)];
}

void PanelEntrada::modoResolver() {
    if (resolviendo_) {
        return;
    }
    Entrada e;
    e.expresion = campoFuncion_->text().trimmed().toStdString();
    e.expresionAuxiliar = campoFuncionAux_->text().trimmed().toStdString();
    e.a = spinA_->value();
    e.b = spinB_->value();
    e.x0 = spinX0_->value();
    e.cifras = spinCifras_->value();
    Q_EMIT resolverSolicitado(e, descriptorSeleccionado());
}

void PanelEntrada::modoCancelar() {
    Q_EMIT cancelarSolicitado();
}

void PanelEntrada::habilitarEjecucion(bool habilitado) {
    resolviendo_ = !habilitado;
    botonResolver_->setEnabled(habilitado);
    botonCancelar_->setVisible(!habilitado);
    campoFuncion_->setEnabled(habilitado);
    campoFuncionAux_->setEnabled(habilitado);
    comboMetodo_->setEnabled(habilitado);
    spinA_->setEnabled(habilitado);
    spinB_->setEnabled(habilitado);
    spinX0_->setEnabled(habilitado);
    if (habilitado) {
        actualizarInfo();
    }
}

void PanelEntrada::actualizarInfo() {
    const int n = spinCifras_->value();
    const double es = 0.5 * std::pow(10.0, 2.0 - n);
    const auto d = descriptorSeleccionado();
    QString texto = tr("E_s = %1 % (n = %2 cifras significativas)")
                         .arg(QString::number(es, 'g', 6))
                         .arg(n);

    if (d.admiteEstimacionAbsoluta && d.tipo == TipoResolucion::RaizIntervalo) {
        const int estimadas = Biseccion::iteracionesParaCifras(spinA_->value(), spinB_->value(), n);
        texto += tr("\nIteraciones estimadas para %1 cifras exactas: k ≈ %2")
                     .arg(n)
                     .arg(estimadas);
    }
    etiquetaInfo_->setText(texto);
}

}  // namespace biseccion
