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
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <cstddef>
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

QString PanelProcedimiento::etiquetaVariable(int indice) {
    // Delega en el núcleo: la tabla y esta narrativa deben nombrar igual a cada
    // componente, o el usuario vería «x₁» en la columna y «x(1)» en la explicación.
    return QString::fromStdString(biseccion::etiquetaVariable(static_cast<std::size_t>(indice)));
}

QString PanelProcedimiento::nombreVariable(int indice) {
    return QString::fromStdString(biseccion::nombreVariable(static_cast<std::size_t>(indice)));
}

QString PanelProcedimiento::etiquetaNorma(int p) {
    return QStringLiteral("‖Δx‖") + QString::number(p);
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
        case MotivoParada::DiagonalNula:
            return tr("Alguna diagonal del sistema es cero, de modo que la variable "
                      "correspondiente no se puede despejar, y no hay forma de "
                      "reordenar las ecuaciones para evitarlo. Revise el coeficiente "
                      "de esa variable.");
        case MotivoParada::SolucionExacta:
            return tr("El vector obtenido satisface el sistema exactamente: el residuo "
                      "b - A\u00b7x es nulo en todas sus componentes.");
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

// Narrativa del método de Jacobi, en los cuatro pasos del enunciado:
//
//   1. Verificar la convergencia   (diagonal dominante, reordenando filas)
//   2. Despejar una variable de cada ecuación
//   3. Sustituir la iterada anterior en esos despejes
//   4. Calcular el error (absoluto, relativo y la P norma de orden p)
//
// El orden importa: el paso 1 actúa sobre el sistema entero, así que se narra una
// sola vez y no se repite en cada iteración. Y los pasos 2 y 3 usan la matriz YA
// reordenada —la que de verdad se iteró— mientras el paso 1 enseña la que escribió
// el usuario, porque comparar las dos es justamente lo que deja ver el cambio.
QString PanelProcedimiento::narrarJacobi(const Resultado& resultado, const Iteracion& it,
                                         int cifras) const {
    const std::size_t n = resultado.matriz.size();
    const QString norma = etiquetaNorma(resultado.normaP);
    QString html;
    html += QStringLiteral("<h3>Iteración k = %1 &nbsp;·&nbsp; sistema de %2 ecuaciones</h3>")
                .arg(it.k)
                .arg(static_cast<int>(n));

    // ---- Paso 1 · Verificación de la convergencia ------------------------
    {
        QString filas;
        for (const DominanciaFila& d : resultado.dominanciaOriginal) {
            const QString nombre = nombreVariable(static_cast<int>(d.indice));
            filas += QStringLiteral("<tr><td>Ecuación de %1</td><td>|%2| = %3</td>"
                                    "<td>Σ|resto| = %4</td><td>%5</td></tr>")
                         .arg(nombre,
                              aTexto(d.diagonal, cifras),
                              aTexto(d.diagonal, cifras),
                              aTexto(d.sumaResto, cifras),
                              d.dominante() ? tr("<b>dominante</b>")
                                            : tr("<font color='#a05000'>no dominante</font>"));
        }
        html += tr("<p><b>Paso 1 · Verificación de la convergencia.</b><br>"
                   "Una ecuación es dominante si el valor absoluto de su coeficiente "
                   "de la variable que despeja supera la suma de los valores "
                   "absolutos de los demás. Como está escrito:</p>"
                   "<table cellspacing=\"4\">%1</table>").arg(filas);

        const auto& intercambios = resultado.reordenamiento.intercambios;
        if (intercambios.empty()) {
            html += resultado.dominanteEstricto
                        ? tr("<p>Todas cumplen la condición: <b>no hay que mover ninguna "
                             "ecuación</b> y la convergencia de Jacobi está garantizada.</p>")
                        : tr("<p><font color='#a05000'><b>No todas cumplen la condición</b>, y "
                             "tampoco hay forma de ordenarlas para que la cumplan. Jacobi "
                             "puede converger igualmente, pero sin garantía.</font></p>");
        } else {
            QString lista;
            for (std::size_t i = 0; i < intercambios.size(); ++i) {
                if (i > 0) {
                    lista += tr(" y ");
                }
                lista += tr("las ecuaciones %1 y %2")
                             .arg(static_cast<int>(intercambios[i].first) + 1)
                             .arg(static_cast<int>(intercambios[i].second) + 1);
            }
            html += tr("<p>Se intercambian %1 —moviendo cada ecuación con su término "
                       "independiente— y quedan así:</p>")
                        .arg(lista);
            QString finales;
            for (const DominanciaFila& d : resultado.dominanciaFinal) {
                const QString nombre = nombreVariable(static_cast<int>(d.indice));
                finales += QStringLiteral("<tr><td>Ec. de %1</td><td>|%2| = %3</td>"
                                          "<td>Σ|resto| = %4</td><td>%5</td></tr>")
                               .arg(nombre,
                                    aTexto(d.diagonal, cifras),
                                    aTexto(d.diagonal, cifras),
                                    aTexto(d.sumaResto, cifras),
                                    d.dominante()
                                        ? tr("<b>dominante</b>")
                                        : tr("<font color='#a05000'>no dominante</font>"));
            }
            html += QStringLiteral("<table cellspacing=\"4\">%1</table>").arg(finales);
            html += resultado.dominanteEstricto
                        ? tr("<p>Ahora las tres cumplen: la convergencia está garantizada. "
                             "Mover las ecuaciones no cambia la solución, solo el orden en "
                             "que se escriben.</p>")
                        : tr("<p><font color='#a05000'>Con esos intercambios tampoco se "
                             "consigue del todo. Jacobi puede converger igualmente, pero sin "
                             "garantía.</font></p>");
        }
    }

    // ---- Paso 2 · Despeje de una variable de cada ecuación ---------------
    {
        QStringList desarrollo;
        for (std::size_t i = 0; i < n && i < resultado.matriz.size(); ++i) {
            // El numerador se arma con el signo de cada coeficiente ya absorbido:
            // concatenar «+» a un coeficiente negativo daría «+ −1.0», que se lee mal.
            QString numerador = aTexto(resultado.terminos[i], cifras);
            for (std::size_t j = 0; j < n; ++j) {
                if (j == i) {
                    continue;
                }
                const double coeficiente = resultado.matriz[i][j];
                numerador += (coeficiente >= 0.0) ? QStringLiteral(" + ")
                                                  : QStringLiteral(" &minus; ");
                numerador += aTexto(std::fabs(coeficiente), cifras) +
                             QStringLiteral(" &middot; ") +
                             nombreVariable(static_cast<int>(j));
            }
            // Una diagonal negativa invertiría el signo de todo el cociente. Se
            // absorbe en el numerador para no dejar un «/ −4.0» que parece un error.
            const double diagonal = resultado.matriz[i][i];
            if (diagonal < 0.0) {
                numerador = QStringLiteral("&minus;(%1)").arg(numerador);
            }
            desarrollo << QStringLiteral("<tr><td>%1</td><td>= &nbsp;(%2) / %3</td></tr>")
                               .arg(nombreVariable(static_cast<int>(i)),
                                    numerador,
                                    aTexto(std::fabs(diagonal), cifras));
        }
        if (!desarrollo.isEmpty()) {
            html += QStringLiteral(
                "<p><b>Paso 2 · Despeje de una variable de cada ecuación.</b><br>"
                "Queda cada variable sola, dividiendo por su coeficiente "
                "diagonal:</p><table cellspacing=\"4\">%1</table>")
                        .arg(desarrollo.join(QString()));
        }
    }

    // ---- Paso 3 · La iteración concreta ---------------------------------
    {
        QString vectorActual = QStringLiteral("(");
        for (std::size_t i = 0; i < it.vectorX.size(); ++i) {
            if (i > 0) {
                vectorActual += QStringLiteral(", ");
            }
            vectorActual += aTexto(it.vectorX[i], cifras);
        }
        vectorActual += QStringLiteral(")");

        if (it.k == 0) {
            // La iterada inicial no viene de ninguna sustitución: es el punto de
            // partida que escribió el usuario, y el primero que hay que colocar en
            // la tabla.
            html += tr("<p><b>Paso 3 · Iterada inicial.</b><br>"
                       "x<sup>(0)</sup> = <b>%1</b><br>"
                       "Es el vector con el que arranca el método, el que el usuario "
                       "escribió en la última columna de cada ecuación. Todavía no se ha "
                       "sustituido nada: al llevarlo a los despejes del paso 2 se obtiene "
                       "x<sup>(1)</sup>.</p>").arg(vectorActual);
        } else {
            // La sustitución que PRODUJO este vector: la iterada k−1 en los
            // despejes. Es lo que hace el enunciado en su paso 3.
            QStringList sustituciones;
            for (std::size_t i = 0; i < n && i < it.vectorX.size(); ++i) {
                QString numerador = aTexto(resultado.terminos[i], cifras);
                for (std::size_t j = 0; j < n; ++j) {
                    if (j == i) {
                        continue;
                    }
                    const double coeficiente = resultado.matriz[i][j];
                    numerador += (coeficiente >= 0.0) ? QStringLiteral(" &minus; ")
                                                      : QStringLiteral(" + ");
                    numerador += aTexto(std::fabs(coeficiente), cifras) +
                                 QStringLiteral("&middot;") +
                                 aTexto(std::fabs(it.vectorX[j]), cifras);
                }
                const double diagonal = resultado.matriz[i][i];
                const QString signo = (diagonal < 0.0) ? QStringLiteral("&minus;(")
                                                       : QString();
                const QString cierre = (diagonal < 0.0) ? QStringLiteral(")")
                                                        : QString();
                // El lado izquierdo lleva x⁽ᵏ⁾, no x⁽ᵏ⁻¹⁾: lo que sale
                // de esta sustitución ES la iterada k. Los valores que entran en el
                // numerador son los de x⁽ᵏ⁻¹⁾, que es lo que dice el texto de
                // arriba. Confundir ambos lados haría creer que el paso calcula
                // justo la iterada que consume.
                //
                // `cierre` cierra el paréntesis que `signo` abre cuando la diagonal
                // es negativa: sin él, el cociente quedaría descompensado y el
                // paréntesis se comería el resto de la fila.
                sustituciones << QStringLiteral("<tr><td>%1<sup>(%2)</sup></td>"
                                               "<td>= &nbsp;%3%4 / %5%6</td>"
                                               "<td>= <b>%7</b></td></tr>")
                                      .arg(nombreVariable(static_cast<int>(i)),
                                           QString::number(it.k),
                                           signo,
                                           numerador,
                                           aTexto(std::fabs(diagonal), cifras),
                                           cierre,
                                           aTexto(it.vectorX[i], cifras));
            }
            html += tr("<p><b>Paso 3 · Sustitución de la iteración anterior.</b><br>"
                       "Todos los términos usan x<sup>(%1)</sup>, la iterada "
                       "<i>anterior</i>: ninguna componente usa el valor recién "
                       "calculado. Así se distingue de Gauss-Seidel.</p>"
                       "<p>x<sup>(%2)</sup> = %3</p>"
                       "<table cellspacing=\"4\">%4</table>")
                        .arg(it.k - 1)
                        .arg(it.k)
                        .arg(vectorActual)
                        .arg(sustituciones.join(QString()));
        }
    }

    // ---- Paso 4 · Los tres errores --------------------------------------
    {
        if (it.k == 0) {
            html += tr("<p><b>Paso 4 · Error.</b><br>"
                       "<i>No se calcula en la iteración 0</i>: no existe x<sup>(−1)</sup> "
                       "anterior con el que comparar. El primer error de la tabla es el de "
                       "la iteración 1.</p>");
        } else {
            html += QStringLiteral("<p><b>Paso 4 · Error.</b><br>"
                                   "Se mide con la norma de orden <b>p = %1</b>: "
                                   "%2<sub>%1</sub> = (Σ |x<sup>(%3)</sup> &minus; "
                                   "x<sup>(%4)</sup>|<sup>%1</sup>)<sup>1/%1</sup> = <b>%5</b>"
                                   "<br>e<sub>a</sub> = %2<sub>%1</sub> / ‖x<sup>(%3)</sup>‖"
                                   "<sub>%1</sub> &middot; 100 = <b>%6 %</b>"
                                   "<br>Se compara contra la tolerancia E<sub>s</sub>.</p>")
                        .arg(QString::number(resultado.normaP))
                        .arg(QStringLiteral("‖Δx"))
                        .arg(it.k)
                        .arg(it.k - 1)
                        .arg(aTexto(*it.norma, cifras), aTexto(*it.eaPorcentaje, cifras));

            // 4b y 4c — Los otros dos errores del enunciado, por componente.
            QStringList absolutos;
            QStringList relativos;
            for (std::size_t i = 0; i < it.errorAbsoluto.size() && i < it.vectorX.size(); ++i) {
                absolutos << QStringLiteral("<tr><td>%1</td><td>= %2</td></tr>")
                                 .arg(nombreVariable(static_cast<int>(i)),
                                      aTexto(it.errorAbsoluto[i], cifras));
                QString valorRelativo = it.errorRelativo.size() > i
                                            ? aTexto(it.errorRelativo[i], cifras)
                                            : QStringLiteral("—");
                QString pie = (it.errorRelativo.size() > i)
                                  ? QStringLiteral("%")
                                  : QString();
                relativos << QStringLiteral("<tr><td>%1</td><td>= %2%3</td></tr>")
                                 .arg(nombreVariable(static_cast<int>(i)), valorRelativo, pie);
            }
            if (!absolutos.isEmpty()) {
                html += tr("<p><i>Error absoluto</i> |x<sup>(%1)</sup> &minus; "
                           "x<sup>(%2)</sup>|, variable a variable:</p>"
                           "<table cellspacing=\"4\">%3</table>"
                           "<p><i>Error relativo</i> |Δx| / |x<sup>(%1)</sup>| &middot; 100, "
                           "variable a variable:</p><table cellspacing=\"4\">%4</table>")
                            .arg(it.k)
                            .arg(it.k - 1)
                            .arg(absolutos.join(QString()))
                            .arg(relativos.join(QString()));
            }
        }
    }

    return html;
}

void PanelProcedimiento::mostrarIteracion(const Resultado& resultado, const Iteracion& it,
                                          const std::string& funcionTexto, int cifras,
                                          const DescriptorMetodo& descriptor) {
    const QString funcion = escaparHtml(QString::fromStdString(funcionTexto));
    if (descriptor.tipo == TipoResolucion::Sistema) {
        setHtml(narrarJacobi(resultado, it, cifras));
        return;
    }
    setHtml((descriptor.tipo == TipoResolucion::RaizPuntoInicial)
                ? narrarNewton(it, funcion, cifras)
                : narrarBiseccion(it, funcion, cifras));
}

void PanelProcedimiento::mostrarResumen(const Resultado& resultado,
                                        const std::string& funcionTexto, int cifras,
                                        const DescriptorMetodo& descriptor) {
    QString html;
    // Un sistema no tiene f(x) que enseñar: el rótulo lo pone el descriptor, no
    // una etiqueta que mentiría sobre lo que el usuario escribió.
    html += (descriptor.tipo == TipoResolucion::Sistema)
                ? QStringLiteral("<h3>Resultado — %1</h3>")
                      .arg(escaparHtml(QString::fromStdString(funcionTexto)))
                : QStringLiteral("<h3>Resultado — f(x) = %1</h3>")
                      .arg(escaparHtml(QString::fromStdString(funcionTexto)));
    html += QStringLiteral("<p><b>Método:</b> %1<br>"
                           "Iteraciones ejecutadas: <b>%2</b><br>%3</p>")
                .arg(escaparHtml(QString::fromStdString(descriptor.nombre)),
                     QString::number(resultado.iteracionesUsadas()),
                     narrarMotivo(resultado.motivo));

    if (!resultado.iteraciones.empty()) {
        const Iteracion& ultima = resultado.iteraciones.back();
        if (descriptor.tipo == TipoResolucion::Sistema) {
            // La solución es un vector: se escribe componente a componente, y no
            // como un único número que no existiría. Con los nombres del enunciado
            // (x, y, z) y no con los subíndices de la tabla, porque aquí se está
            // hablando de las ecuaciones.
            QStringList componentes;
            for (std::size_t i = 0; i < ultima.vectorX.size(); ++i) {
                componentes << nombreVariable(static_cast<int>(i)) +
                                   QStringLiteral(" = <b>%1</b>")
                                       .arg(aTexto(ultima.vectorX[i], cifras));
            }
            html += tr("<p>Vector solución (%1 ecuaciones):<br>%2</p>")
                        .arg(static_cast<int>(ultima.vectorX.size()))
                        .arg(componentes.join(QStringLiteral(" &nbsp;·&nbsp; ")));

            // ---- La fórmula de solución, que es lo que el enunciado pide ----
            //
            // x = Dx/D, y = Dy/D, z = Dz/D por la regla de Cramer. Se calcula por
            // determinantes, no con el propio Jacobi, y sirve justo para lo
            // contrario: para contrastar la aproximación con el valor verdadero y
            // ver si se acertó.
            if (resultado.formulaSolucion) {
                const FormulaSolucion& f = *resultado.formulaSolucion;
                QStringList cramer;
                cramer << QStringLiteral("<tr><td>D</td><td>= det(A)</td><td>= <b>%1</b></td></tr>")
                              .arg(aTexto(f.determinante, cifras));
                for (std::size_t i = 0; i < f.determinantes.size() && i < f.valores.size(); ++i) {
                    cramer << QStringLiteral("<tr><td>D%1</td><td>= det(A con la columna %2 "
                                             "sustituida por b)</td><td>= <b>%3</b></td></tr>")
                                  .arg(nombreVariable(static_cast<int>(i)),
                                       nombreVariable(static_cast<int>(i)),
                                       aTexto(f.determinantes[i], cifras));
                }
                html += tr("<p><b>Fórmula de solución (regla de Cramer)</b><br>"
                           "Cada variable se obtiene dividiendo su determinante por el "
                           "determinante de la matriz:</p><table cellspacing=\"4\">%1</table>")
                            .arg(cramer.join(QString()));

                QStringList valores;
                QStringList diferencias;
                for (std::size_t i = 0; i < f.valores.size(); ++i) {
                    valores << nombreVariable(static_cast<int>(i)) +
                                   QStringLiteral(" = D%1/D = <b>%2</b>")
                                       .arg(nombreVariable(static_cast<int>(i)),
                                            aTexto(f.valores[i], cifras));
                    if (i < ultima.vectorX.size()) {
                        diferencias << QStringLiteral("%1: %2")
                                           .arg(nombreVariable(static_cast<int>(i)),
                                                aTexto(std::fabs(ultima.vectorX[i] - f.valores[i]),
                                                       cifras));
                    }
                }
                html += QStringLiteral("<p><b>Solución exacta:</b> %1</p>")
                            .arg(valores.join(QStringLiteral(" &nbsp;·&nbsp; ")));
                if (!diferencias.isEmpty()) {
                    html += tr("<p>Diferencia con la aproximación de Jacobi "
                               "(|aproximado &minus; exacto|):<br>%1</p>")
                                .arg(diferencias.join(QStringLiteral(" &nbsp;·&nbsp; ")));
                }
            } else {
                html += tr("<p><i>No hay fórmula de solución:</i> det(A) = 0, de modo que el "
                           "sistema no tiene solución única y no se puede escribir "
                           "x = Dx/D. El método puede seguir convergiendo —a una de las "
                           "infinitas soluciones compatibles— pero no hay un valor único "
                           "con el que comparar.</i></p>");
            }

            if (resultado.reordenamiento.aplicado) {
                // El resumen nombra QUÉ ecuaciones se movieron, no solo que se
                // movieron: es el paso 1 del procedimiento, y el usuario tiene que
                // poder leerlo sin abrir la iteración.
                QString lista;
                const auto& intercambios = resultado.reordenamiento.intercambios;
                for (std::size_t i = 0; i < intercambios.size(); ++i) {
                    if (i > 0) {
                        lista += tr(" y ");
                    }
                    lista += tr("las ecuaciones %1 y %2")
                                 .arg(static_cast<int>(intercambios[i].first) + 1)
                                 .arg(static_cast<int>(intercambios[i].second) + 1);
                }
                html += tr("<p><b>Paso 1 · convergencia.</b> Se intercambiaron %1 —cada "
                           "una con su término independiente— para que la matriz fuera "
                           "estrictamente diagonalmente dominante. Mover las ecuaciones no "
                           "cambia la solución: solo el orden en que se escriben.</p>")
                            .arg(lista);
            } else if (!resultado.dominanteEstricto) {
                html += tr("<p><font color='#a05000'><b>Paso 1 · convergencia.</b> El "
                           "sistema no es diagonalmente dominante y no hay forma de "
                           "ordenarlo para que lo sea. Jacobi puede converger igualmente, "
                           "pero no hay garantía.</font></p>");
            }
            html += tr("<p>%1</p>")
                        .arg(resultado.dominanteEstricto
                                 ? tr("La convergencia estaba garantizada: la matriz es "
                                      "estrictamente diagonalmente dominante.")
                                 : tr("El sistema no es diagonalmente dominante, y aun así "
                                      "las iteraciones convergieron (o no llegaron a la "
                                      "tolerancia)."));
            // El residuo dice si la solución sirve de algo: sin él, un vector que
            // "ha convergido" no demuestra que satisfaga el sistema.
            if (resultado.matriz.size() == resultado.terminos.size() &&
                !resultado.terminos.empty()) {
                double maximo = 0.0;
                for (std::size_t i = 0; i < resultado.matriz.size(); ++i) {
                    double acumulado = resultado.terminos[i];
                    for (std::size_t j = 0; j < resultado.matriz[i].size(); ++j) {
                        if (j < ultima.vectorX.size()) {
                            acumulado -= resultado.matriz[i][j] * ultima.vectorX[j];
                        }
                    }
                    maximo = std::max(maximo, std::fabs(acumulado));
                }
                html += tr("<p>Residuo máximo ‖b &minus; A·x‖<sub>∞</sub> = %1</p>")
                            .arg(aTexto(maximo, cifras));
            }
            html += tr("<p>Error medido con %1, con p = %2.</p>")
                        .arg(etiquetaNorma(resultado.normaP))
                        .arg(resultado.normaP);
        } else if (descriptor.tipo == TipoResolucion::RaizPuntoInicial) {
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