// PanelProcedimiento.cpp
// -----------------------------------------------------------------------------
// Vista paso a paso de la iteración seleccionada (sustitución aritmética).
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

void PanelProcedimiento::mostrarIteracion(const Iteracion& iteracion,
                                          const std::string& funcionTexto, int cifras) {
    const QString A = aTexto(iteracion.a, cifras);
    const QString B = aTexto(iteracion.b, cifras);
    const QString M = aTexto(iteracion.m, cifras);
    const QString fa = aTexto(iteracion.fa, cifras);
    const QString fb = aTexto(iteracion.fb, cifras);
    const QString fm = aTexto(iteracion.fm, cifras);
    const QString funcion = escaparHtml(QString::fromStdString(funcionTexto));

    QString html;
    html += QStringLiteral("<h3>Iteración k = %1 &nbsp;·&nbsp; f(x) = %2</h3>").arg(iteracion.k).arg(funcion);

    // Paso 1 — Verificación inicial.
    const double productoInicial = iteracion.fa * iteracion.fb;
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
                "f(a)\u00b7f(b) = %5 %6 0 → %7</p>")
                .arg(A, fa, B, fb, aTexto(productoInicial, cifras), relacion, conclusion);
    // El algoritmo solo se ejecuta si la verificación fue satisfactoria. Un
    // producto exactamente nulo no impide continuar: la raíz ya está localizada.
    if (productoInicial > 0.0) {
        html += QStringLiteral("<p><i>El método no puede continuar en este intervalo.</i></p>");
        setHtml(html);
        return;
    }

    // Paso 2 — Punto medio.
    html += QStringLiteral("<p><b>Paso 2 · Punto medio.</b><br>"
                           "m = (a + b)/2 = (%1 + %2)/2 = <b>%3</b></p>")
                .arg(A, B, M);

    // Paso 3 — Criterio de signo de f(a)·f(m).
    const double producto = iteracion.fa * iteracion.fm;
    QString paso3;
    if (producto < 0.0) {
        paso3 = QStringLiteral("f(a)\u00b7f(m) = %1 &lt; 0 → la raíz está en la mitad "
                               "<b>inferior</b>; nuevo b = m = %2.")
                    .arg(aTexto(producto, cifras), M);
    } else if (producto > 0.0) {
        paso3 = QStringLiteral("f(a)\u00b7f(m) = %1 &gt; 0 → la raíz está en la mitad "
                               "<b>superior</b>; nuevo a = m = %2.")
                    .arg(aTexto(producto, cifras), M);
    } else {
        paso3 = QStringLiteral("f(a)\u00b7f(m) = 0 → <b>raíz exacta</b> en m = %1.<br>"
                               "(f(m) = %2)")
                    .arg(M, fm);
    }
    html += QStringLiteral("<p><b>Paso 3 · Criterio de signo.</b><br>"
                           "f(m) = %1<br><b>%2</b></p>")
                .arg(fm, paso3);

    // Paso 4 — Error relativo porcentual.
    if (iteracion.eaPorcentaje.has_value()) {
        html += QStringLiteral("<p><b>Paso 4 · Error relativo porcentual.</b><br>"
                               "e\u2090 = |(m\u2096 − m\u2096\u208b\u2081)/m\u2096|\u00b7100 = "
                               "<b>%1 %</b><br>%2</p>")
                    .arg(aTexto(*iteracion.eaPorcentaje, cifras),
                         QStringLiteral("Se compara contra la tolerancia E_s."));
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

    setHtml(html);
}

void PanelProcedimiento::mostrarResumen(const Resultado& resultado,
                                        const std::string& funcionTexto, int cifras) {
    QString motivo;
    switch (resultado.motivo) {
        case MotivoParada::ToleranciaAlcanzada:
            motivo = tr("Se alcanzó la tolerancia del error relativo (e_a &lt; E_s).");
            break;
        case MotivoParada::RaizExacta:
            motivo = tr("Se encontró una raíz exacta (f(m) = 0).");
            break;
        case MotivoParada::Interrumpido:
            motivo = tr("El cálculo fue interrumpido por el usuario.");
            break;
        case MotivoParada::MaxIteraciones:
        default:
            motivo = tr("Se alcanzó el límite de iteraciones sin cumplir los criterios.");
            break;
    }

    QString html;
    html += QStringLiteral("<h3>Resultado — f(x) = %1</h3>").arg(
        escaparHtml(QString::fromStdString(funcionTexto)));
    html += QStringLiteral("<p>Iteraciones ejecutadas: <b>%1</b><br>%2</p>")
                .arg(resultado.iteracionesUsadas())
                .arg(motivo);
    if (!resultado.iteraciones.empty()) {
        const Iteracion& ultima = resultado.iteraciones.back();
        html += QStringLiteral(
                    "<p>Raíz aproximada: <b>m = %1</b><br>"
                    "f(m) = %2<br>"
                    "Intervalo final: [%3, %4]</p>")
                    .arg(aTexto(ultima.m, cifras), aTexto(ultima.fm, cifras),
                         aTexto(ultima.a, cifras), aTexto(ultima.b, cifras));
    }
    setHtml(html);
}

void PanelProcedimiento::mostrarInformacion(const QString& html) {
    setHtml(html);
}

}  // namespace biseccion