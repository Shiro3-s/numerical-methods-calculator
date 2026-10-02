// Resultado.cpp
// -----------------------------------------------------------------------------
// Implementación de los accesores comunes del modelo de datos.
// -----------------------------------------------------------------------------
#include "Resultado.hpp"

#include <limits>

namespace biseccion {

std::optional<double> valorCampo(const Iteracion& iteracion, CampoIteracion campo,
                                TipoResolucion tipo) {
    // k y e_a existen en todos los métodos.
    if (campo == CampoIteracion::K) {
        return static_cast<double>(iteracion.k);
    }
    if (campo == CampoIteracion::Ea) {
        return iteracion.eaPorcentaje;
    }

    // El resto se reparte según el tipo: los campos del otro método no existen
    // aquí, y se devuelven vacíos en vez de como ceros sin significado.
    const bool intervalo = (tipo == TipoResolucion::RaizIntervalo);
    switch (campo) {
        case CampoIteracion::A: return intervalo ? std::optional(iteracion.a) : std::nullopt;
        case CampoIteracion::B: return intervalo ? std::optional(iteracion.b) : std::nullopt;
        case CampoIteracion::M: return intervalo ? std::optional(iteracion.m) : std::nullopt;
        case CampoIteracion::Fa: return intervalo ? std::optional(iteracion.fa) : std::nullopt;
        case CampoIteracion::Fb: return intervalo ? std::optional(iteracion.fb) : std::nullopt;
        case CampoIteracion::Fm: return intervalo ? std::optional(iteracion.fm) : std::nullopt;
        case CampoIteracion::X: return intervalo ? std::nullopt : iteracion.x;
        case CampoIteracion::Fx: return intervalo ? std::nullopt : iteracion.fx;
        case CampoIteracion::Fdx: return intervalo ? std::nullopt : iteracion.fdx;
        case CampoIteracion::Paso: return intervalo ? std::nullopt : iteracion.paso;
        case CampoIteracion::K:
        case CampoIteracion::Ea:
            return std::nullopt;  // ya atendidos arriba; nunca se llega aquí
    }
    return std::nullopt;
}

double Resultado::raiz() const {
    if (iteraciones.empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const Iteracion& ultima = iteraciones.back();
    // Un método de punto inicial deja x; uno de intervalo, el punto medio. Si
    // el último paso quedó registrado pero sin x, se conserva el punto medio:
    // es lo que el propio método assure como último punto válido.
    return ultima.x.value_or(ultima.m);
}

int Resultado::iteracionesUsadas() const {
    return static_cast<int>(iteraciones.size());
}

}  // namespace biseccion