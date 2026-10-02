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
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QOverload>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#include "core/Metodo.hpp"
#include "core/Biseccion.hpp"
#include "core/Jacobi.hpp"

namespace biseccion {

PanelEntrada::PanelEntrada(QWidget* padre) : QWidget(padre) {
    auto* caja = new QGroupBox(tr("Entrada de datos"), this);
    auto* formulario = new QFormLayout(caja);

    comboMetodo_ = new QComboBox(caja);
    formulario->addRow(tr("Método:"), comboMetodo_);

    campoFuncion_ = new QLineEdit(caja);
    campoFuncion_->setPlaceholderText(
        tr("p. ej. 5x - 2sin(x) + x^2  ·  sin, cos, asin, ln, log, abs…"));
    etiquetaFuncion_ = new QLabel(tr("f(x) ="), caja);
    formulario->addRow(etiquetaFuncion_, campoFuncion_);

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

    // ---- Sistema lineal (Jacobi) ----------------------------------------
    // Va en su propia caja, y no como filas del formulario de f(x), porque para
    // un sistema la expresión no existe: lo que se teclea es una matriz.
    cajaSistema_ = new QGroupBox(tr("Sistema de ecuaciones lineales  A·x = b"), this);
    {
        auto* disposicion = new QVBoxLayout(cajaSistema_);

        spinDimension_ = new QSpinBox(cajaSistema_);
        spinDimension_->setRange(2, 12);
        spinDimension_->setValue(3);
        spinDimension_->setSuffix(tr("  ecuaciones"));
        disposicion->addWidget(spinDimension_);

        // El orden p de la norma, con la fórmula escrita al lado. El enunciado
        // trabaja con «3 NORMA P=3» y no deja claro dónde sale ese 3; aquí el
        // campo lo declara y la fórmula de abajo lo usa en todo lo que sigue.
        auto* filaNorma = new QFormLayout;
        spinNormaP_ = new QSpinBox(cajaSistema_);
        spinNormaP_->setRange(1, 12);
        spinNormaP_->setValue(3);
        spinNormaP_->setToolTip(
            tr("Orden de la norma con la que se mide el error del sistema.\n"
               "‖Δx‖ₚ = (Σ |Δxᵢ|ᵖ)^(1/p)\n"
               "El enunciado trabaja con p = 3."));
        filaNorma->addRow(tr("Orden p de la norma:"), spinNormaP_);
        disposicion->addLayout(filaNorma);

        etiquetaDefinicionNorma_ = new QLabel(cajaSistema_);
        etiquetaDefinicionNorma_->setWordWrap(true);
        etiquetaDefinicionNorma_->setStyleSheet(QStringLiteral("color: #444; font-size: 11px;"));
        disposicion->addWidget(etiquetaDefinicionNorma_);

        tablaSistema_ = new QTableWidget(cajaSistema_);
        // n filas (ecuaciones) × n+2 columnas: los coeficientes de x, y, z… | b
        // | el valor inicial de la variable que despeja esa fila.
        tablaSistema_->setAlternatingRowColors(true);
        tablaSistema_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        // La cabecera vertical nombra la variable que despeja cada ecuación. Es lo
        // que hace legible la última columna: su valor inicial (0, 0, 1) quiere
        // decir «x arranca en 0, y en 0, z en 1», y solo se lee así si la fila
        // dice a qué variable pertenece.
        tablaSistema_->verticalHeader()->setVisible(true);
        tablaSistema_->verticalHeader()->setDefaultSectionSize(28);
        tablaSistema_->verticalHeader()->setMinimumSectionSize(20);
        // Un QTableWidget es un área de scroll, así que su minimumSizeHint es
        // diminuto: sin esto el layout lo encoge hasta una franja y las celdas
        // quedan sin superficie con la que hacer clic o escribir. El alto
        // mínimo lo fija `reconstruirSistema()` en cada cambio de dimensión; el
        // factor de estiramiento hace que la rejilla ocupe el espacio sobrante
        // en vez de dejar un hueco vacío debajo.
        tablaSistema_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        disposicion->addWidget(tablaSistema_, 1);

        auto* pista = new QLabel(
            tr("Cada fila es una ecuación y su cabecera lateral es la variable que "
               "despeja: la columna «x» es el coeficiente de x, la «y» el de y, etc. "
               "La última columna es el valor con el que empieza esa variable."),
            cajaSistema_);
        pista->setWordWrap(true);
        pista->setStyleSheet(QStringLiteral("color: #444; font-size: 11px;"));
        disposicion->addWidget(pista);
    }
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
    // La caja del sistema se queda con el espacio sobrante cuando el método es
    // de sistemas: es el `addStretch(1)` de más abajo quien se lo queda cuando no
    // (bisección y Newton no tienen rejilla que mostrar).
    diseno->addWidget(cajaSistema_, 1);
    diseno->addLayout(filaBotones);
    diseno->addWidget(etiquetaInfo_);
    diseno->addStretch(0);

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
    connect(spinDimension_, qOverload<int>(&QSpinBox::valueChanged),
            this, &PanelEntrada::dimensionCambiada);
    connect(spinNormaP_, qOverload<int>(&QSpinBox::valueChanged), this, [this] {
        actualizarDefinicionNorma();
        actualizarInfo();
    });
    connect(tablaSistema_, &QTableWidget::itemChanged,
            this, &PanelEntrada::actualizarInfo);

    reconstruirSistema();
    // La fórmula de la norma se escribe ya, con el p vigente, en vez de esperar a
    // que el usuario toque el campo. Este vive dentro de `cajaSistema_`, que ya se
    // oculta entero cuando el método no es de sistema, así que no necesita
    // visibilidad propia.
    actualizarDefinicionNorma();
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

    const bool sistema = (d.tipo == TipoResolucion::Sistema);

    // Un sistema no tiene expresión que teclear: se oculta la fila de f(x) y con
    // ella el resto del formulario queda reducido a lo que sí aplica.
    const bool expresion = d.requiereExpresion && !sistema;
    etiquetaFuncion_->setVisible(expresion);
    campoFuncion_->setVisible(expresion);

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

    // El editor de matriz solo aparece para un método de sistema.
    cajaSistema_->setVisible(sistema);

    // Mostrar u ocultar la rejilla cambia el alto que el panel necesita, y el
    // QSplitter que lo contiene no se entera solo.
    emit altoRequeridoCambiado();
}

void PanelEntrada::dimensionCambiada(int valor) {
    (void)valor;
    reconstruirSistema();
    // Más ecuaciones ⇒ más filas ⇒ el panel necesita más alto.
    emit altoRequeridoCambiado();
}

void PanelEntrada::reconstruirSistema() {
    const int n = spinDimension_->value();
    const int columnas = n + 2;  // coeficientes de x… | b | valor inicial

    // Se bloquean las señales porque rellenar la rejilla dispara itemChanged por
    // cada celda: sin el bloqueo, `actualizarInfo` correría n·(n+2) veces y
    // reconstruir el sistema se realimentaría a sí mismo.
    tablaSistema_->blockSignals(true);
    tablaSistema_->setRowCount(n);
    tablaSistema_->setColumnCount(columnas);

    // Las columnas se llaman por la variable de la que son el coeficiente —x, y,
    // z…— y no a₁, a₂, a₃. La cabecera lateral repite ese nombre en cada fila,
    // que es lo que hace falta para entender la última columna.
    QStringList cabecera;
    QStringList laterales;
    for (int j = 0; j < n; ++j) {
        const QString nombre = QString::fromStdString(
            nombreVariable(static_cast<std::size_t>(j)));
        cabecera << nombre;
        laterales << nombre;
    }
    cabecera << QStringLiteral("b");
    cabecera << QStringLiteral("x⁰");
    tablaSistema_->setHorizontalHeaderLabels(cabecera);
    tablaSistema_->setVerticalHeaderLabels(laterales);
    tablaSistema_->setToolTip(
        tr("Cada fila es una ecuación. La cabecera lateral es la variable que despeja:\n"
           "la fila «x» empieza en la ecuación que da x, y su última celda es x⁰."));

    // Ejemplo por defecto. Para 3 ecuaciones es el sistema del enunciado, escrito
    // en el ORDEN en que aparece allí —con solo la fila 2 dominante— para que el
    // paso 1 del procedimiento tenga algo que reordenar y se vea que funciona.
    // Para cualquier otra n se pone uno estrictamente dominante (diagonal n+1
    // contra n−1 repartidos fuera de ella), que resuelve en pocas iteraciones y
    // sirve de punto de partida listo para trastear.
    static const std::vector<std::vector<double>> kSistemaEnunciado = {
        { 1.0, 1.0, 4.0 }, { -2.0, 4.0, 1.0 }, { 6.0, 3.0, -2.0 },
    };
    static const std::vector<double> kTerminosEnunciado = { 15.0, 9.0, 6.0 };

    const bool enunciado = (n == 3);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < columnas; ++j) {
            auto* celda = new QTableWidgetItem;
            double valor = 0.0;
            if (j < n) {
                valor = enunciado ? kSistemaEnunciado[static_cast<std::size_t>(i)]
                                             [static_cast<std::size_t>(j)]
                                  : ((i == j) ? (n + 1.0) : 1.0);
            } else if (j == n) {
                valor = enunciado ? kTerminosEnunciado[static_cast<std::size_t>(i)]
                                  : static_cast<double>(i + 1);
            } else {
                // Valor inicial: el enunciado arranca en (0, 0, 1), y se mantiene
                // ese mismo patrón (todo cero menos el último) para que cambiar la
                // dimensión no deje una columna de ceros que parece un descuido.
                valor = (i == n - 1) ? 1.0 : 0.0;
            }
            celda->setText(QString::number(valor, 'g', 10));
            celda->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            tablaSistema_->setItem(i, j, celda);
        }
    }
    tablaSistema_->blockSignals(false);

    // Alto mínimo: cabecera + las filas, hasta un tope. Sin esto, un
    // QTableWidget (que es un área de scroll, con un minimumSizeHint diminuto)
    // se deja aplastar por el layout hasta una franja de unos pocos píxeles: las
    // celdas existen y son editables, pero no hay superficie con la que hacer
    // clic ni escribir.
    //
    // El tope importa: con 12 ecuaciones la rejilla entera mediría ~386 px y
    // ese `minimumHeight` se propagaría al minimumSizeHint del panel y de la
    // ventana, que podría no caber en pantallas normales. A partir de
    // `filasVisibles` se deja que la rejilla Use scroll.
    //
    // El alto de fila se toma de `rowHeight(0)` y no de `sizeHintForRow()`,
    // que es protegida en QTableView: una vez insertadas las filas,
    // `rowHeight(0)` es la altura real que Qt calcula para el estilo vigente.
    constexpr int filasVisibles = 8;
    const int filas = std::min(n, filasVisibles);
    const int altoCabecera = tablaSistema_->horizontalHeader()->height();
    const int altoFila = (n > 0) ? tablaSistema_->rowHeight(0) : 30;
    tablaSistema_->setMinimumHeight(altoCabecera + filas * altoFila +
                                    2 * tablaSistema_->frameWidth());
}

void PanelEntrada::actualizarDefinicionNorma() {
    const int p = spinNormaP_->value();
    // La fórmula con el p sustituido, que es la definición operativa de la que
    // habla el enunciado. El «con p = 3» es lo que pedía el paso de dejar claro
    // dónde se define P: no es un 3 suelto en un enunciado, es el valor de un
    // campo que el usuario puede cambiar y que todo lo demás lee.
    etiquetaDefinicionNorma_->setText(
        tr("Norma del error:  ‖Δx‖ₚ = (Σ |Δxᵢ|^p)^(1/p)  con p = %1  ·  "
           "e_a = ‖Δx‖ₚ / ‖x⁽ᵏ⁾‖ₚ · 100").arg(QString::number(p)));
}

void PanelEntrada::cargarSistemaDeEjemplo() {
    spinDimension_->setValue(3);
    reconstruirSistema();
}

bool PanelEntrada::sistemaDominante() const {
    Entrada e;
    volcarSistemaEnEntrada(e);
    return Jacobi::esDiagonalmenteDominante(e.matriz);
}

Reordenamiento PanelEntrada::reordenamientoPrevisto() const {
    Entrada e;
    volcarSistemaEnEntrada(e);
    // Se opera sobre copias: es una previsualización y no debe tocar la rejilla.
    std::vector<std::vector<double>> matriz = e.matriz;
    std::vector<double> terminos = e.terminos;
    return Jacobi::reordenarParaDominancia(matriz, terminos);
}

void PanelEntrada::volcarSistemaEnEntrada(Entrada& e) const {
    const int n = spinDimension_->value();
    e.dimension = n;
    e.matriz.assign(static_cast<std::size_t>(n),
                    std::vector<double>(static_cast<std::size_t>(n), 0.0));
    e.terminos.assign(static_cast<std::size_t>(n), 0.0);
    e.vectorInicial.assign(static_cast<std::size_t>(n), 0.0);

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (const QTableWidgetItem* celda = tablaSistema_->item(i, j)) {
                // Una celda en blanco o no numérica se lee como 0, no como NaN:
                // devolver NaN aquí envenenaría la matriz entera.
                e.matriz[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] =
                    celda->text().toDouble();
            }
        }
        if (const QTableWidgetItem* celda = tablaSistema_->item(i, n)) {
            e.terminos[static_cast<std::size_t>(i)] = celda->text().toDouble();
        }
        if (const QTableWidgetItem* celda = tablaSistema_->item(i, n + 1)) {
            e.vectorInicial[static_cast<std::size_t>(i)] = celda->text().toDouble();
        }
    }
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
    // El p de la norma. Viaja siempre, y solo lo leen los métodos de sistema.
    e.normaP = spinNormaP_->value();
    // Para un método de sistema la expresión no aplica; lo que viaja es la
    // matriz. Volcarla siempre es inofensivo para los demás métodos, que la
    // ignoran por su TipoResolucion.
    volcarSistemaEnEntrada(e);
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
    spinDimension_->setEnabled(habilitado);
    spinNormaP_->setEnabled(habilitado);
    tablaSistema_->setEnabled(habilitado);
    if (habilitado) {
        actualizarInfo();
    }
}

void PanelEntrada::actualizarInfo() {
    const int n = spinCifras_->value();
    const double es = 0.5 * std::pow(10.0, 2.0 - n);
    const auto d = descriptorSeleccionado();

    if (d.tipo == TipoResolucion::Sistema) {
        // El criterio de parada de un sistema es ‖Δx‖ₚ/‖x‖ₚ, no el de una raíz, así
        // que la estimación de iteraciones de bisección no aplica. Lo que sí
        // interesa es el paso 1 del procedimiento: qué se va a hacer con las filas
        // para conseguir la diagonal dominante, dicho ANTES de resolver.
        QString texto = tr("E_s = %1 % (n = %2 cifras significativas)")
                             .arg(QString::number(es, 'g', 6))
                             .arg(n);
        texto += tr("\nError: ‖Δx‖%1 con p = %2.")
                     .arg(spinNormaP_->value())
                     .arg(spinNormaP_->value());

        const Reordenamiento previsto = reordenamientoPrevisto();
        if (sistemaDominante()) {
            texto += tr("\nYa es diagonalmente dominante: Jacobi converge y no hay "
                        "que mover ninguna ecuación.");
        } else if (previsto.intercambios.empty()) {
            texto += tr("\n<font color='#a05000'><b>Atención:</b> el sistema NO es "
                        "diagonalmente dominante y no hay forma de ordenarlo así. "
                        "Jacobi puede converger igualmente, pero no hay garantía.</font>");
        } else {
            QString intercambios;
            for (std::size_t i = 0; i < previsto.intercambios.size(); ++i) {
                if (i > 0) {
                    intercambios += tr(" y ");
                }
                intercambios += tr("las filas %1 y %2")
                                    .arg(previsto.intercambios[i].first + 1)
                                    .arg(previsto.intercambios[i].second + 1);
            }
            texto += tr("\nNo es dominante. Al resolver se intercambiarán %1 para "
                        "conseguirlo.")
                         .arg(intercambios);
            if (!previsto.aplicado) {
                texto += tr(" <font color='#a05000'>(con esos intercambios tampoco se "
                            "consigue del todo)</font>");
            }
        }
        etiquetaInfo_->setText(texto);
        return;
    }

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
