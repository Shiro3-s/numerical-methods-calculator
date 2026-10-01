// PanelEntrada.cpp
// -----------------------------------------------------------------------------
// Implementación del panel de entrada de datos.
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

#include "core/Biseccion.hpp"
#include "ejercicios/Ejercicios.hpp"

namespace biseccion {

PanelEntrada::PanelEntrada(QWidget* padre) : QWidget(padre) {
    auto* caja = new QGroupBox(tr("Entrada de la función"), this);
    auto* formulario = new QFormLayout(caja);

    comboEjercicios_ = new QComboBox(caja);
    formulario->addRow(tr("Ejercicio:"), comboEjercicios_);

    campoFuncion_ = new QLineEdit(caja);
    campoFuncion_->setPlaceholderText(
        tr("p. ej. 5x - 2sin(x) + x^2  ·  sin, cos, asin, ln, log, abs…"));
    formulario->addRow(tr("f(x) ="), campoFuncion_);

    spinA_ = new QDoubleSpinBox(caja);
    spinA_->setRange(-1.0e6, 1.0e6);
    spinA_->setDecimals(6);
    spinB_ = new QDoubleSpinBox(caja);
    spinB_->setRange(-1.0e6, 1.0e6);
    spinB_->setDecimals(6);
    formulario->addRow(tr("a (extremo inferior):"), spinA_);
    formulario->addRow(tr("b (extremo superior):"), spinB_);

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

    // Conexiones.
    connect(comboEjercicios_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &PanelEntrada::aplicarEjercicio);
    connect(botonResolver_, &QPushButton::clicked, this, &PanelEntrada::modoResolver);
    connect(botonCancelar_, &QPushButton::clicked, this, &PanelEntrada::modoCancelar);
    connect(campoFuncion_, &QLineEdit::textEdited,
            this, &PanelEntrada::marcarFuncionPersonalizada);
    connect(spinCifras_, qOverload<int>(&QSpinBox::valueChanged),
            this, &PanelEntrada::actualizarInfo);
    connect(spinA_, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &PanelEntrada::actualizarInfo);
    connect(spinB_, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &PanelEntrada::actualizarInfo);
}

void PanelEntrada::cargarPresets() {
    comboEjercicios_->blockSignals(true);
    comboEjercicios_->clear();
    const auto& presets = ejerciciosPredeterminados();
    for (const auto& ejercicio : presets) {
        comboEjercicios_->addItem(QString::fromStdString(ejercicio.nombre));
    }
    // Entrada «Calculadora libre»: no corresponde a ningún preset; el usuario
    // digita f(x) y el intervalo por completo. Se coloca al final para no
    // alterar el comportamiento de arranque (E1 sigue siendo el primero).
    comboEjercicios_->addItem(tr("Calculadora libre"));
    comboEjercicios_->setCurrentIndex(0);
    comboEjercicios_->blockSignals(false);
    aplicarEjercicio(0);  // aplica el primer preset y actualiza la información
}

void PanelEntrada::aplicarEjercicio(int indice) {
    const auto& presets = ejerciciosPredeterminados();
    if (indice < 0) {
        return;
    }
    const auto total = static_cast<int>(presets.size());
    if (indice >= total) {
        aplicarModoLibre();  // últimos ítems del selector: «Calculadora libre»
        return;
    }
    const Ejercicio& ejercicio = presets[static_cast<std::size_t>(indice)];
    aplicarFuncion(QString::fromStdString(ejercicio.funcionTexto),
                   ejercicio.a, ejercicio.b, spinCifras_->value(),
                   QString::fromStdString(ejercicio.descripcion));
}

void PanelEntrada::aplicarFuncion(const QString& expresion, double a, double b,
                                  int cifras, const QString& descripcion) {
    campoFuncion_->setText(expresion);
    spinA_->setValue(a);
    spinB_->setValue(b);
    spinCifras_->setValue(cifras);
    etiquetaDescripcion_->setText(descripcion);
    funcionPersonalizada_ = false;  // es un preset: el intervalo proviene del enunciado
    modoLibre_ = false;             // un preset cancela el modo «Calculadora libre»
    actualizarInfo();
}

// «Calculadora libre»: el usuario digita f(x) y el intervalo [a, b] por completo;
// nada proviene de un preset, por lo que no se muestra el aviso de «f(x) editada»:
// en este modo editar a mano es el comportamiento esperado.
void PanelEntrada::aplicarModoLibre() {
    campoFuncion_->clear();
    campoFuncion_->setFocus();
    spinA_->setValue(0.0);
    spinB_->setValue(1.0);
    etiquetaDescripcion_->setText(
        tr("Calculadora libre: digite f(x) y el intervalo [a, b] a su gusto."));
    funcionPersonalizada_ = false;  // en modo libre editar a mano es lo normal
    modoLibre_ = true;
    actualizarInfo();
}

void PanelEntrada::marcarFuncionPersonalizada() {
    // Las ediciones del usuario en f(x) no actualizan [a, b]; el aviso visual
    // recuerda verificar que el intervalo aún contenga la raíz.
    if (!funcionPersonalizada_) {
        funcionPersonalizada_ = true;
        actualizarInfo();
    }
}

void PanelEntrada::modoResolver() {
    if (resolviendo_) {
        return;
    }
    Q_EMIT resolverSolicitado(campoFuncion_->text().trimmed(),
                              spinA_->value(), spinB_->value(),
                              spinCifras_->value());
    habilitarEjecucion(false);
}

void PanelEntrada::modoCancelar() {
    Q_EMIT cancelarSolicitado();
}

void PanelEntrada::habilitarEjecucion(bool habilitado) {
    resolviendo_ = !habilitado;
    botonResolver_->setEnabled(habilitado);
    botonCancelar_->setVisible(!habilitado);
    campoFuncion_->setEnabled(habilitado);
    comboEjercicios_->setEnabled(habilitado);
    spinA_->setEnabled(habilitado);
    spinB_->setEnabled(habilitado);
    if (habilitado) {
        actualizarInfo();
    }
}

void PanelEntrada::actualizarInfo() {
    const int n = spinCifras_->value();
    const double es = 0.5 * std::pow(10.0, 2.0 - n);  // E_s en porcentaje
    const int estimadas = Biseccion::iteracionesParaCifras(spinA_->value(), spinB_->value(), n);
    QString texto = tr("E_s = %1 % (n = %2 cifras significativas)\n"
                       "Iteraciones estimadas para %2 cifras exactas: k ≈ %3")
                        .arg(QString::number(es, 'g', 6))
                        .arg(n)
                        .arg(estimadas);
    if (funcionPersonalizada_) {
        texto += tr("\n⚠ f(x) editada a mano: revise que el intervalo [%1, %2] "
                    "siga conteniendo la raíz.")
                     .arg(spinA_->value())
                     .arg(spinB_->value());
    }
    etiquetaInfo_->setText(texto);
}

}  // namespace biseccion