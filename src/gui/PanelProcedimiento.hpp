// PanelProcedimiento.hpp
// -----------------------------------------------------------------------------
// Panel 4 · Vista procedimental: muestra la sustitución aritmética paso a paso
// (pasos 1 a 4) de la iteración seleccionada y el criterio de cifras (paso 5).
// -----------------------------------------------------------------------------
#pragma once

#include <QTextBrowser>

#include <string>

#include "core/Biseccion.hpp"

namespace biseccion {

class PanelProcedimiento : public QTextBrowser {
    Q_OBJECT

public:
    explicit PanelProcedimiento(QWidget* padre = nullptr);

    // Detalle aritmético de una iteración concreta.
    void mostrarIteracion(const Iteracion& iteracion, const std::string& funcionTexto, int cifras);

    // Resumen final tras completar la resolución.
    void mostrarResumen(const Resultado& resultado, const std::string& funcionTexto, int cifras);

    // Mensaje informativo (o de error).
    void mostrarInformacion(const QString& html);

    // Escapa '&', '<' y '>' para interpolar texto de usuario dentro del HTML
    // que muestra este panel. Compartido: el mensaje de error del parser también
    // cita caracteres tecleados por el usuario.
    [[nodiscard]] static QString escaparHtml(QString texto);

private:
    static QString aTexto(double valor, int cifras);
};

}  // namespace biseccion