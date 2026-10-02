// Metodo.cpp
// -----------------------------------------------------------------------------
// Catálogo de métodos numéricos: el registro que alimenta el desplegable de la
// GUI. Mantener el orden estable facilita el arranque y las pruebas.
// -----------------------------------------------------------------------------
#include "Metodo.hpp"

#include <memory>
#include <string>
#include <vector>

#include "Biseccion.hpp"
#include "Funcion.hpp"
#include "NewtonRaphson.hpp"

namespace biseccion {

std::vector<std::unique_ptr<MetodoNumerico>> catalogoMetodos() {
    // El catálogo entrega instancias cuyo propósito aquí es obtener el
    // DESCRIPTOR (rótulo, tipo, columnas, si muestra gráfico). Resolver requiere
    // las expresiones ya analizadas, así que la GUI construye su propia instancia
    // configurada a partir del descriptor que este catálogo describe.
    //
    // Por eso el constructor recibe evaluadores vacíos: no se invocan al pedir
    // el descriptor. Para resolver, usar `crearDesdeEntrada`.
    std::vector<std::unique_ptr<MetodoNumerico>> metodos;
    metodos.push_back(std::make_unique<Biseccion>(std::function<double(double)>{}));
    metodos.push_back(std::make_unique<NewtonRaphson>(std::function<double(double)>{},
                                                      std::function<double(double)>{}));
    return metodos;
}

std::unique_ptr<MetodoNumerico> crearDesdeEntrada(const Entrada& entrada,
                                                   const DescriptorMetodo& descriptor) {
    // Analiza las expresiones una sola vez y devuelve el método ya configurado,
    // o ExpresionInvalida si f(x) (o la expresión auxiliar) no se puede analizar.
    const std::string& clave = descriptor.clave;

    if (clave == "biseccion") {
        auto f = parsearFuncion(entrada.expresion);
        if (!f) {
            return nullptr;
        }
        return std::make_unique<Biseccion>(std::move(f->f));
    }

    if (clave == "newton_raphson") {
        auto f = parsearFuncion(entrada.expresion);
        if (!f) {
            return nullptr;
        }
        if (entrada.expresionAuxiliar.empty()) {
            return nullptr;
        }
        auto df = parsearFuncion(entrada.expresionAuxiliar);
        if (!df) {
            return nullptr;
        }
        return std::make_unique<NewtonRaphson>(std::move(f->f), std::move(df->f));
    }

    // Método desconocido: no se puede construir.
    return nullptr;
}

const MetodoNumerico* buscarMetodo(const std::string& clave) {
    const auto metodos = catalogoMetodos();
    for (const auto& m : metodos) {
        if (m->descriptor().clave == clave) {
            return m.get();
        }
    }
    return nullptr;
}

}  // namespace biseccion