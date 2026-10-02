// Biseccion.cpp
// -----------------------------------------------------------------------------
// Implementación del algoritmo con la estructura obligatoria de los 5 pasos.
// -----------------------------------------------------------------------------
#include "Biseccion.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace biseccion {

namespace {

// Tope de seguridad para la estimación de iteraciones: evita que un intervalo
// patológico (o valores no finitos) derive un bucle interminable en la GUI.
constexpr int kMaxIteracionesEstimadas = 10'000;

// Compara signos SIN multiplicar. f(a)·f(b) puede desbordarse a ±inf o
// desvanecerse a 0 por subnormal, y en ambos casos daría una decisión falsa:
// un producto que cae a cero se confundiría con una raíz exacta.
[[nodiscard]] inline bool signosOpuestos(double u, double v) {
    return (u < 0.0) != (v < 0.0);
}

// Construye el resultado trivial para una raíz que cae exactamente en un
// extremo del intervalo: no hay nada que iterar.
[[nodiscard]] Resultado raizExactaEnExtremo(double a, double b, double raiz,
                                            double fa, double fb) {
    Resultado resultado;
    resultado.motivo = MotivoParada::RaizExacta;
    resultado.tipo = TipoResolucion::RaizIntervalo;
    Iteracion iteracion;
    iteracion.k = 1;
    iteracion.a = a;
    iteracion.b = b;
    iteracion.m = raiz;
    iteracion.x = raiz;
    iteracion.fa = fa;
    iteracion.fb = fb;
    iteracion.fm = 0.0;
    iteracion.fx = 0.0;
    resultado.iteraciones.push_back(iteracion);
    return resultado;
}

}  // namespace

Biseccion::Biseccion(std::function<double(double)> f) : f_(std::move(f)) {}

DescriptorMetodo Biseccion::descriptor() const {
    DescriptorMetodo d;
    d.clave = "biseccion";
    d.nombre = "Bisección";
    d.descripcion = "Método de bisección: intervalo [a, b] con f(a)·f(b) < 0";
    d.tipo = TipoResolucion::RaizIntervalo;
    d.etiquetaAuxiliar.clear();
    d.requiereExpresionAuxiliar = false;
    d.etiquetaRaiz = "m";
    d.muestraGrafico = true;
    d.muestraTrazado = false;
    d.admiteEstimacionAbsoluta = true;
    d.columnas = {
        { "k", CampoIteracion::K },
        { "a", CampoIteracion::A },
        { "b", CampoIteracion::B },
        { "m", CampoIteracion::M },
        { "f(m)", CampoIteracion::Fm },
        { "e_a (%)", CampoIteracion::Ea },
    };
    return d;
}

int Biseccion::iteracionesParaCifras(double a, double b, int n) {
    n = std::clamp(n, 1, 15);
    const double ancho = std::fabs(b - a);
    // Intervalo degenerado (a == b): log2(0) es -inf y convertirlo a int sería
    // comportamiento indefinido; además no hay reducción que aplicar.
    if (!(ancho > 0.0)) {
        return 0;
    }
    const double toleranciaAbsoluta = 0.5 * std::pow(10.0, -n);
    const double necesarias = std::log2(ancho / toleranciaAbsoluta);
    // El intervalo ya cumple la tolerancia (o el cociente no es representable).
    if (!(necesarias > 0.0)) {
        return 0;
    }
    return std::clamp(static_cast<int>(std::ceil(necesarias)), 0,
                      kMaxIteracionesEstimadas);
}

std::expected<Resultado, ErrorMetodo>
Biseccion::resolver(const Entrada& entrada, std::stop_token detener) const {
    const int n = std::clamp(entrada.cifras, 1, 12);
    const double es = 0.5 * std::pow(10.0, 2.0 - n);
    const int maxIter = std::max(300, iteracionesParaCifras(entrada.a, entrada.b, n) + 20);
    return resolver(entrada.a, entrada.b, es, maxIter, detener);
}

std::expected<Resultado, ErrorMetodo>
Biseccion::resolver(double a, double b, double toleranciaEsPorcentaje,
                    int maxIteraciones, std::stop_token detener) const {
    // ---------- Paso 1: verificación inicial -----------------------------
    // El cambio de signo se decide comparando signos, no multiplicando: el
    // producto puede desbordarse o desvanecerse a 0 y mentir.
    const double faInicial = f_(a);
    const double fbInicial = f_(b);

    if (faInicial == 0.0 || fbInicial == 0.0) {
        // Hay una raíz exacta en un extremo: el método no necesita iterar.
        return raizExactaEnExtremo(a, b, (faInicial == 0.0) ? a : b, faInicial,
                                   fbInicial);
    }
    if (!signosOpuestos(faInicial, fbInicial)) {
        // Sin cambio de signo, no se garantiza una raíz en [a, b].
        return std::unexpected(ErrorMetodo::SinCambioDeSigno);
    }

    Resultado resultado;
    resultado.tipo = TipoResolucion::RaizIntervalo;
    double fa = faInicial;
    double fb = fbInicial;
    double mAnterior = std::numeric_limits<double>::quiet_NaN();

    for (int k = 1; k <= maxIteraciones; ++k) {
        if (detener.stop_requested()) {
            resultado.motivo = MotivoParada::Interrumpido;
            break;
        }

        // ---------- Paso 2: punto medio --------------------------------
        const double m = (a + b) / 2.0;
        const double fm = f_(m);

        Iteracion iteracion;
        iteracion.k = k;
        iteracion.a = a;
        iteracion.b = b;
        iteracion.m = m;
        iteracion.x = m;  // coherente para el modelo genérico
        iteracion.fa = fa;
        iteracion.fb = fb;  // f(b) se arrastra desde la vuelta anterior
        iteracion.fm = fm;
        iteracion.fx = fm;

        // ---------- Paso 4: error relativo porcentual -------------------
        // e_a = |(m_actual − m_anterior) / m_actual| · 100
        // NO se calcula en la primera iteración (no hay m anterior).
        if (k > 1) {
            const double ea = std::fabs((m - mAnterior) / m) * 100.0;
            iteracion.eaPorcentaje = ea;
            if (ea < toleranciaEsPorcentaje) {
                resultado.motivo = MotivoParada::ToleranciaAlcanzada;
            }
        }
        resultado.iteraciones.push_back(iteracion);
        if (resultado.motivo == MotivoParada::ToleranciaAlcanzada) {
        break;
    }

    // ---------- Paso 3: signo de f(a)·f(m) --------------------------
    // La decisión usa f(m) == 0 y la comparación de signos; el producto
    // fa·fm solo se muestra en el panel procedimental.
    if (fm == 0.0) {
        // 3c: raíz exacta en m; el algoritmo termina.
        resultado.motivo = MotivoParada::RaizExacta;
        break;
    }
    if (signosOpuestos(fa, fm)) {
        // 3a: la raíz está en la mitad inferior → nuevo b = m.
        b = m;
        fb = fm;
    } else {
        // 3b: la raíz está en la mitad superior → nuevo a = m.
        a = m;
        fa = fm;
    }

    mAnterior = m;
    }

    return resultado;
}

const std::function<double(double)>& Biseccion::funcion() const {
    return f_;
}

}  // namespace biseccion