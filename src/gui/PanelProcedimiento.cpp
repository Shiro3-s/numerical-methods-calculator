// PanelProcedimiento.cpp
// -----------------------------------------------------------------------------
// Vista paso a paso de la iteración seleccionada (suministro aritmético).
// Los pasos narrativos son específicos del método: bisección tiene cinco pasos
// obligatorios, Newton-Raphson tiene los suyos. Ambos se generan a partir del
// descriptor, de modo que añadir un método no obliga a tocar este archivo salvo
// para darle su narrativa propia.
// -----------------------------------------------------------------------------
#include "PanelProcedimiento.hpp"

#include <QString>

#include <cmath>
#include <format>

#include "core/CifrasSignificativas.hpp"

namespace biseccion {

PanelProcedimiento::PanelProcedimiento(QWidget* padre) : QTextBrowser(padre) {
    setOpenExternalLinks(false);
    setReadOnly(true);
}

QString PanelProcedimiento::aTexto(double valor, int cifras) {
    return QString::fromStdString(formatearParaCifras(valor, cifras));
}

QString PanelProcedimiento::escaparHtml(QString texto) {
    texto.replace(QStringLiteral("&"), QStringLiteral("&amp;"));
    texto.replace(QStringLiteral("<"), QStringLiteral("&lt;"));
    texto.replace(QStringLiteral(">"), QStringLiteral("&gt;"));
    return texto;
}

QString PanelProcedimiento::narrarMotivo(MotivoParada motivo) {
    switch (motivo) {
        case MotivoParada::ToleranciaAlcanzada:
            return tr("Se alcanzó la tolerancia del error relativo (e_a &lt; E_s).");
        case MotivoParada::RaizExacta:
            return tr("Se encontró una raíz exacta (el valor de f en el punto obtenido es 0).");
        case MotivoParada::DerivadaNula:
            return tr("La derivada se anuló (f'(x) = 0), de modo que no existe el paso "
                      "x - f(x)/f'(x). Pruebe con otra iterada inicial.");
        case MotivoParada::Divergente:
            return tr("Las iteraciones divergieron: los valores se dispararon sin converger. "
                      "Pruebe con otra iterada inicial.");
        case MotivoParada::Interrumpido:
            return tr("El cálculo fue interrumpido por el usuario.");
        case MotivoParada::MaxIteraciones:
        default:
            return tr("Se alcanzó el límite de iteraciones sin cumplir los criterios.");
    }
}

QString PanelProcedimiento::narrarBiseccion(const Iteracion& it, const QString& funcion,
                                            int cifras) const {
    const QString A = aTexto(it.a, cifras);
    const QString B = aTexto(it.b, cifras);
    const QString M = aTexto(it.m, cifras);
    const QString fa = aTexto(it.fa, cifras);
    const QString fb = aTexto(it.fb, cifras);
    const QString fm = aTexto(it.fm, cifras);

    QString html;
    html += QStringLiteral("<h3>Iteración k = %1 &nbsp;·&nbsp; f(x) = %2</h3>")
                .arg(it.k).arg(funcion);

    // Paso 1 — Verificación inicial.
    const double productoInicial = it.fa * it.fb;
    const QString relacion =
        (productoInicial < 0.0)  ? QStringLiteral("&lt;")
        : (productoInicial > 0.0) ? QStringLiteral("&gt;")
                                  : QStringLiteral("=");
    const QString conclusion =
        (productoInicial < 0.0)  ? tr("existe al menos una raíz en [a, b].")
        : (productoInicial > 0.0) ? tr("no se garantiza la existencia de una raíz.")
                                  : tr("la raíz es exacta y cae en un extremo de [a, b].");
    html += QStringLiteral(
                "<p><b>Paso 1 · Verificación inicial.</b><br>"
                "f(a) = f(%1) = %2<br>"
                "f(b) = f(%3) = %4<br>"
                "f(a)\u00b7f(b) = %5 %6 0 &rarr; %7</p>")
                .arg(A, fa, B, fb, aTexto(productoInicial, cifras), relacion, conclusion);
    if (productoInicial > 0.0) {
        html += QStringLiteral("<p><i>El método no puede continuar en este intervalo.</i></p>");
        return html;
    }

    // Paso 2 — Punto medio.
    html += QStringLiteral("<p><b>Paso 2 · Punto medio.</b><br>"
                           "m = (a + b)/2 = (%1 + %2)/2 = <b>%3</b></p>")
                .arg(A, B, M);

    // Paso 3 — Criterio de signo de f(a)·f(m).
    const double producto = it.fa * it.fm;
    QString paso3;
    if (producto < 0.0) {
        paso3 = QStringLiteral("f(a)\u00b7f(m) = %1 &lt; 0 &rarr; la raíz está en la mitad "
                               "<b>inferior</b>; nuevo b = m = %2.")
                    .arg(aTexto(producto, cifras), M);
    } else if (producto > 0.0) {
        paso3 = QStringLiteral("f(a)\u00b7f(m) = %1 &gt; 0 &rarr; la raíz está en la mitad "
                               "<b>superior</b>; nuevo a = m = %2.")
                    .arg(aTexto(producto, cifras), M);
    } else {
        paso3 = QStringLiteral("f(a)\u00b7f(m) = 0 &rarr; <b>raíz exacta</b> en m = %1.<br>"
                               "(f(m) = %2)")
                    .arg(M, fm);
    }
    html += QStringLiteral("<p><b>Paso 3 · Criterio de signo.</b><br>"
                           "f(m) = %1<br><b>%2</b></p>")
                .arg(fm, paso3);

    // Paso 4 — Error relativo porcentual.
    if (it.eaPorcentaje.has_value()) {
        html += QStringLiteral("<p><b>Paso 4 · Error relativo porcentual.</b><br>"
                               "e\u2090 = |(m\u2096 − m\u2096\u208b\u2081)/m\u2096|\u00b7100 = "
                               "<b>%1 %</b><br>Se compara contra la tolerancia E_s.</p>")
                    .arg(aTexto(*it.eaPorcentaje, cifras));
    } else {
        html += QStringLiteral(
                    "<p><b>Paso 4 · Error relativo porcentual.</b><br>"
                    "e\u2090 = |(m\u2096 − m\u2096\u208b\u2081)/m\u2096|\u00b7100 = "
                    "<i>no se calcula en la primera iteración</i> (no existe m anterior).</p>");
    }

    // Paso 5 — Cifras significativas y presentación.
    const double es = 0.5 * std::pow(10.0, 2.0 - cifras);
    html += QStringLiteral("<p><b>Paso 5 · Presentación.</b><br>"
                           "Se muestran n = %1 cifras significativas (E_s = %2 %); "
                           "los decimales se limitan dinámicamente.</p>")
                .arg(cifras)
                .arg(aTexto(es, cifras));

    return html;
}

QString PanelProcedimiento::narrarNewton(const Iteracion& it, const QString& funcion,
                                         int cifras) const {
    const QString x = it.x ? aTexto(*it.x, cifras) : QStringLiteral("—");
    const QString fx = it.fx ? aTexto(*it.fx, cifras) : QStringLiteral("—");
    const QString fdx = it.fdx ? aTexto(*it.fdx, cifras) : QStringLiteral("—");

    QString html;
    html += QStringLiteral("<h3>Iteración k = %1 &nbsp;·&nbsp; f(x) = %2</h3>")
                .arg(it.k).arg(funcion);

    // Paso 1 — Iterada actual y residuo.
    html += QStringLiteral("<p><b>Paso 1 · Iterada actual.</b><br>"
                           "x\u2096 = %1<br>"
                           "f(x\u2096) = %2</p>")
                .arg(x, fx);

    // Paso 2 — Derivada en el punto.
    html += QStringLiteral("<p><b>Paso 2 · Derivada en el punto.</b><br>"
                           "f'(x\u2096) = %1</p>")
                .arg(fdx);

    // Paso 3 — El paso de Newton (o su imposibilidad).
    if (it.paso.has_value() && it.fx.has_value() && it.fdx.has_value()) {
        html += QStringLiteral("<p><b>Paso 3 · Paso de Newton.</b><br>"
                               "x\u2096\u208a\u2081 = x\u2096 − f(x\u2096)/f'(x\u2096)<br>"
                               "= %1 − (%2)/(%3)<br>"
                               "= <b>%4</b><br>"
                               "paso = −f/f' = %5</p>")
                    .arg(x, fx, fdx, aTexto(it.x.value_or(0.0) + *it.paso, cifras),
                         aTexto(*it.paso, cifras));
    } else if (it.fdx.has_value() && *it.fdx == 0.0) {
        html += QStringLiteral("<p><b>Paso 3 · Paso de Newton.</b><br>"
                               "f'(x\u2096) = 0 &rarr; el paso no existe: el método se detiene.</p>");
    } else {
        html += QStringLiteral("<p><b>Paso 3 · Paso de Newton.</b><br>"
                               "<i>No se calculó el paso en esta iteración.</i></p>");
    }

    // Paso 4 — Error relativo porcentual.
    if (it.eaPorcentaje.has_value()) {
        html += QStringLiteral("<p><b>Paso 4 · Error relativo porcentual.</b><br>"
                               "e\u2090 = |(x\u2096 − x\u2096\u208b\u2081)/x\u2096|\u00b7100 = "
                               "<b>%1 %</b><br>Se compara contra la tolerancia E_s.</p>")
                    .arg(aTexto(*it.eaPorcentaje, cifras));
    } else {
        html += QStringLiteral(
                    "<p><b>Paso 4 · Error relativo porcentual.</b><br>"
                    "e\u2090 = |(x\u2096 − x\u2096\u208b\u2081)/x\u2096|\u00b7100 = "
                    "<i>no se calcula en la primera iteración</i> (no existe x anterior).</p>");
    }

    // Paso 5 — Presentación.
    const double es = 0.5 * std::pow(10.0, 2.0 - cifras);
    html += QStringLiteral("<p><b>Paso 5 · Presentación.</b><br>"
                           "Se muestran n = %1 cifras significativas (E_s = %2 %).</p>")
                .arg(cifras)
                .arg(aTexto(es, cifras));

    return html;
}

void PanelProcedimiento::mostrarIteracion(const Iteracion& iteracion,
                                          const std::string& funcionTexto, int cifras,
                                          const DescriptorMetodo& descriptor) {
    const QString funcion = escaparHtml(QString::fromStdString(funcionTexto));
    setHtml((descriptor.tipo == TipoResolucion::RaizPuntoInicial)
                ? narrarNewton(iteracion, funcion, cifras)
                : narrarBiseccion(iteracion, funcion, cifras));
}

void PanelProcedimiento::mostrarResumen(const Resultado& resultado,
                                        const std::string& funcionTexto, int cifras,
                                        const DescriptorMetodo& descriptor) {
    QString html;
    html += QStringLiteral("<h3>Resultado — f(x) = %1</h3>").arg(
        escaparHtml(QString::fromStdString(funcionTexto)));
    html += QStringLiteral("<p><b>Método:</b> %1<br>"
                           "Iteraciones ejecutadas: <b>%2</b><br>%3</p>")
                .arg(escaparHtml(QString::fromStdString(descriptor.nombre)),
                     QString::number(resultado.iteracionesUsadas()),
                     narrarMotivo(resultado.motivo));

    if (!resultado.iteraciones.empty()) {
        const Iteracion& ultima = resultado.iteraciones.back();
        if (descriptor.tipo == TipoResolucion::RaizPuntoInicial) {
            const QString x = ultima.x ? aTexto(*ultima.x, cifras) : QStringLiteral("—");
            const QString fx = ultima.fx ? aTexto(*ultima.fx, cifras) : QStringLiteral("—");
            html += QStringLiteral("<p>Raíz aproximada: <b>x = %1</b><br>f(x) = %2</p>")
                        .arg(x, fx);
        } else {
            html += QStringLiteral(
                        "<p>Raíz aproximada: <b>m = %1</b><br>"
                        "f(m) = %2<br>"
                        "Intervalo final: [%3, %4]</p>")
                        .arg(aTexto(ultima.m, cifras), aTexto(ultima.fm, cifras),
                             aTexto(ultima.a, cifras), aTexto(ultima.b, cifras));
        }
    }
    setHtml(html);
}

void PanelProcedimiento::mostrarInformacion(const QString& html) {
    setHtml(html);
}

}  // namespace biseccion