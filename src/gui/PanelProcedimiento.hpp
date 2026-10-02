// PanelProcedimiento.hpp
// -----------------------------------------------------------------------------
// Panel 4 · Vista procedimental: muestra la sustitución aritmética paso a paso
// (pasos 1 a 4) de la iteración seleccionada y el criterio de cifras (paso 5).
// -----------------------------------------------------------------------------
#pragma once

#include <QTextBrowser>

#include <string>

#include "core/Metodo.hpp"
#include "core/Resultado.hpp"

namespace biseccion {

class PanelProcedimiento : public QTextBrowser {
    Q_OBJECT

public:
    explicit PanelProcedimiento(QWidget* padre = nullptr);

    // Detalle aritmético de una iteración concreta. Los pasos narrativos
    // dependen del método: los 5 pasos obligatorios son de bisección, no de la app.
    void mostrarIteracion(const Iteracion& iteracion, const std::string& funcionTexto,
                          int cifras, const DescriptorMetodo& descriptor);

    // Resumen final tras completar la resolución.
    void mostrarResumen(const Resultado& resultado, const std::string& funcionTexto,
                        int cifras, const DescriptorMetodo& descriptor);

    // Mensaje informativo (o de error).
    void mostrarInformacion(const QString& html);

    // Escapa '&', '<' y '>' para interpolar texto de usuario dentro del HTML
    // que muestra este panel. Compartido: el mensaje de error del parser también
    // cita caracteres tecleados por el usuario.
    [[nodiscard]] static QString escaparHtml(QString texto);

private:
    static QString aTexto(double valor, int cifras);
    [[nodiscard]] QString narrarBiseccion(const Iteracion&, const QString&, int) const;
    [[nodiscard]] QString narrarNewton(const Iteracion&, const QString&, int) const;
    [[nodiscard]] static QString narrarMotivo(MotivoParada motivo);
};

}  // namespace biseccion