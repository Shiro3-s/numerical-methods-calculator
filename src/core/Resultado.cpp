// Resultado.cpp
// -----------------------------------------------------------------------------
// Implementación de los accesores comunes del modelo de datos.
// -----------------------------------------------------------------------------
#include "Resultado.hpp"

#include <cstddef>
#include <limits>
#include <string>
#include <vector>

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

    // El resto se reparte según el tipo: los campos de los otros métodos no
    // existen aquí, y se devuelven vacíos en vez de como ceros sin significado.
    // Sin este filtro, la tabla de un método de sistema mostraría ceros falsos en
    // las columnas de intervalo, y la de Newton en las del sistema.
    const bool intervalo = (tipo == TipoResolucion::RaizIntervalo);
    const bool punto = (tipo == TipoResolucion::RaizPuntoInicial);
    const bool sistema = (tipo == TipoResolucion::Sistema);
    switch (campo) {
        case CampoIteracion::A: return intervalo ? std::optional(iteracion.a) : std::nullopt;
        case CampoIteracion::B: return intervalo ? std::optional(iteracion.b) : std::nullopt;
        case CampoIteracion::M: return intervalo ? std::optional(iteracion.m) : std::nullopt;
        case CampoIteracion::Fa: return intervalo ? std::optional(iteracion.fa) : std::nullopt;
        case CampoIteracion::Fb: return intervalo ? std::optional(iteracion.fb) : std::nullopt;
        case CampoIteracion::Fm: return intervalo ? std::optional(iteracion.fm) : std::nullopt;
        case CampoIteracion::X: return punto ? iteracion.x : std::nullopt;
        case CampoIteracion::Fx: return punto ? iteracion.fx : std::nullopt;
        case CampoIteracion::Fdx: return punto ? iteracion.fdx : std::nullopt;
        case CampoIteracion::Paso: return punto ? iteracion.paso : std::nullopt;
        case CampoIteracion::Norma: return sistema ? iteracion.norma : std::nullopt;
        // VectorX no se resuelve aquí: es una columna de familia que la tabla
        // expande en varias, y cada subcolumna pide su componente con
        // `valorComponente`.
        case CampoIteracion::VectorX: return std::nullopt;
        case CampoIteracion::K:
        case CampoIteracion::Ea:
            return std::nullopt;  // ya atendidos arriba; nunca se llega aquí
    }
    return std::nullopt;
}

std::optional<double> valorComponente(const Iteracion& iteracion, std::size_t indice) {
    if (indice < iteracion.vectorX.size()) {
        return iteracion.vectorX[indice];
    }
    // Un vector más corto que la columna esperada: vacío, no cero. Un 0.0 aquí
    // parecería un valor calculado.
    return std::nullopt;
}

std::string etiquetaVariable(std::size_t indice) {
    // Unicode tiene un subíndice por dígito, pero ninguno para el 0 suelto ni
    // para los números de dos cifras. A partir de x₁₀ se cae a paréntesis en vez
    // de inventar un glifo que no se renderiza.
    static constexpr const char* const kSubindices[] = {
        "\u2081", "\u2082", "\u2083", "\u2084", "\u2085",
        "\u2086", "\u2087", "\u2088", "\u2089",
    };
    constexpr std::size_t kConSubindice = sizeof(kSubindices) / sizeof(kSubindices[0]);
    const std::size_t numero = indice + 1;
    if (numero <= kConSubindice) {
        return std::string("x") + kSubindices[numero - 1];
    }
    return "x(" + std::to_string(numero) + ")";
}

std::string nombreVariable(std::size_t indice) {
    // x, y, z, w… como las nombra el enunciado. Se deja fuera la «p» (que es el
    // orden de la norma del error) y la «e» (que es el símbolo del error), para
    // que un sistema largo no se lea con letras que el ejercicio ya usa para
    // otra cosa.
    static constexpr const char* const kNombres[] = {
        "x", "y", "z", "w", "v", "u", "t", "s",
        "r", "q", "o", "n", "m", "l", "k", "j",
    };
    constexpr std::size_t kConNombre = sizeof(kNombres) / sizeof(kNombres[0]);
    if (indice < kConNombre) {
        return kNombres[indice];
    }
    // A partir de aquí se vuelve al subíndice, que al menos no se confunde con
    // ningún símbolo del enunciado.
    return etiquetaVariable(indice);
}

double Resultado::raiz() const {
    // La solución de un sistema es un vector: no hay un escalar que devolver, y
    // devolver el m = 0.0 que el sistema deja en su último punto presentaría un
    // cero como si fuera la respuesta.
    if (tipo == TipoResolucion::Sistema) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (iteraciones.empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const Iteracion& ultima = iteraciones.back();
    // Un método de punto inicial deja x; uno de intervalo, el punto medio. Si
    // el último paso quedó registrado pero sin x, se conserva el punto medio:
    // es lo que el propio método asegura como último punto válido.
    return ultima.x.value_or(ultima.m);
}

int Resultado::iteracionesUsadas() const {
    return static_cast<int>(iteraciones.size());
}

const std::vector<double>& Resultado::solucion() const {
    static const std::vector<double> vacio;
    if (iteraciones.empty()) {
        return vacio;
    }
    return iteraciones.back().vectorX;
}

int Resultado::dimension() const {
    return static_cast<int>(solucion().size());
}

}  // namespace biseccion