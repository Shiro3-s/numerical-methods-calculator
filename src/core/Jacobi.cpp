// Jacobi.cpp
// -----------------------------------------------------------------------------
// Implementación del método de Jacobi para sistemas lineales.
// -----------------------------------------------------------------------------
#include "Jacobi.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace biseccion {

namespace {

// Una diagonal "nula" en la práctica lo es por debajo de este valor: |a_ii| = 1e-18
// no es un divisor útil y daría iteraciones explosivas en vez de un diagnóstico.
constexpr double kEpsilonDiagonal = 1e-18;
// Por encima de este valor las componentes se consideran disparadas.
constexpr double kCotaDivergente = 1e12;
// Dos ecuaciones es el sistema más pequeño con sentido; uno solo es un caso
// degenerado de bisección con disfraz de sistema.
constexpr int kDimensionMinima = 2;
constexpr int kDimensionMaxima = 32;
// Por debajo de este determinante se considera que A es singular y la fórmula de
// Cramer no existe. Es relativo a la escala de A, no un valor fijo: un sistema de
// coeficientes en torno a 1e8 tiene determinantes "grandes" que en valor absoluto
// no significan nada.
constexpr double kCotaDeterminante = 1e-12;
// A partir de esta dimensión el determinante se calcula por eliminación en vez de
// por desarrollo de cofactores: 12! términos dejan de ser un momento y empiezan
// a desbordar (y a perder precisión por el número de sumandos).
constexpr std::size_t kDimensionMaximaCofactores = 8;

// Norma infinita: el máximo de los valores absolutos. Se usa solo para el
// residuo —«¿son cero TODOS los componentes?»— y para el diagnóstico de
// divergencia. El error que ve el usuario lo mide la norma de orden p.
[[nodiscard]] double normaInfinita(const std::vector<double>& v) {
    double maximo = 0.0;
    for (const double componente : v) {
        maximo = std::max(maximo, std::fabs(componente));
    }
    return maximo;
}

// Residuo b − A·x. Todas sus componentes a cero significan que el vector
// satisface el sistema exactamente en aritmética de punto flotante.
[[nodiscard]] std::vector<double> residuo(const std::vector<std::vector<double>>& matriz,
                                         const std::vector<double>& terminos,
                                         const std::vector<double>& x) {
    const std::size_t n = matriz.size();
    std::vector<double> r(n, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        double acumulado = terminos[i];
        for (std::size_t j = 0; j < n; ++j) {
            acumulado -= matriz[i][j] * x[j];
        }
        r[i] = acumulado;
    }
    return r;
}

// |a_ii| frente a Σ_{j≠i} |a_ij| en una fila: el número que decide si esa
// ecuación cumple la condición de convergencia.
[[nodiscard]] DominanciaFila medirFila(const std::vector<std::vector<double>>& matriz,
                                       std::size_t i) {
    DominanciaFila fila;
    fila.indice = i;
    fila.diagonal = std::fabs(matriz[i][i]);
    for (std::size_t j = 0; j < matriz.size(); ++j) {
        if (j != i) {
            fila.sumaResto += std::fabs(matriz[i][j]);
        }
    }
    return fila;
}

[[nodiscard]] std::size_t contarDominantes(const std::vector<std::vector<double>>& matriz) {
    std::size_t cuenta = 0;
    for (std::size_t i = 0; i < matriz.size(); ++i) {
        if (medirFila(matriz, i).dominante()) {
            ++cuenta;
        }
    }
    return cuenta;
}

// det(A) por desarrollo a lo largo de la primera fila. Es el mismo desarrollo
// que se hace a mano en el enunciado, así que con las matrices enteras del
// ejercicio sale el determinante exacto (129, no 128.99999999999997) y la
// fórmula x = Dx/D se puede escribir con números.redondos.
[[nodiscard]] double determinantePorCofactores(const std::vector<std::vector<double>>& matriz) {
    const std::size_t n = matriz.size();
    if (n == 1) {
        return matriz[0][0];
    }
    if (n == 2) {
        return matriz[0][0] * matriz[1][1] - matriz[0][1] * matriz[1][0];
    }
    double total = 0.0;
    for (std::size_t columna = 0; columna < n; ++columna) {
        // La menor que quita la fila 0 y la columna j.
        std::vector<std::vector<double>> menor(n - 1, std::vector<double>(n - 1));
        for (std::size_t i = 1; i < n; ++i) {
            std::size_t destino = 0;
            for (std::size_t j = 0; j < n; ++j) {
                if (j != columna) {
                    menor[i - 1][destino] = matriz[i][j];
                    ++destino;
                }
            }
        }
        const double signo = (columna % 2 == 0) ? 1.0 : -1.0;
        total += signo * matriz[0][columna] * determinantePorCofactores(menor);
    }
    return total;
}

// det(A) por eliminación gaussiana con pivoteo parcial. Mathemáticamente el
// mismo número, pero en O(n³) y sin el factorial del desarrollo, así que es lo
// único utilizable en sistemas grandes. Solo entra a partir de
// `kDimensionMaximaCofactores`, donde el desarrollo ya no es practical.
[[nodiscard]] double determinantePorEliminacion(std::vector<std::vector<double>> matriz) {
    const std::size_t n = matriz.size();
    double determinante = 1.0;
    for (std::size_t columna = 0; columna < n; ++columna) {
        std::size_t pivote = columna;
        for (std::size_t i = columna + 1; i < n; ++i) {
            if (std::fabs(matriz[i][columna]) > std::fabs(matriz[pivote][columna])) {
                pivote = i;
            }
        }
        if (std::fabs(matriz[pivote][columna]) < kEpsilonDiagonal) {
            return 0.0;
        }
        if (pivote != columna) {
            std::swap(matriz[pivote], matriz[columna]);
            determinante = -determinante;
        }
        determinante *= matriz[columna][columna];
        for (std::size_t i = columna + 1; i < n; ++i) {
            const double factor = matriz[i][columna] / matriz[columna][columna];
            if (factor == 0.0) {
                continue;
            }
            for (std::size_t j = columna; j < n; ++j) {
                matriz[i][j] -= factor * matriz[columna][j];
            }
        }
    }
    return determinante;
}

[[nodiscard]] double determinante(const std::vector<std::vector<double>>& matriz) {
    if (matriz.size() <= kDimensionMaximaCofactores) {
        return determinantePorCofactores(matriz);
    }
    return determinantePorEliminacion(matriz);
}

}  // namespace

DescriptorMetodo Jacobi::descriptor() const {
    DescriptorMetodo d;
    d.clave = "jacobi";
    d.nombre = "Jacobi";
    d.descripcion =
        "Método de Jacobi para sistemas lineales: todas las variables se actualizan "
        "a la vez usando solo la iterada anterior";
    d.tipo = TipoResolucion::Sistema;
    // No hay f(x) que escribir: el sistema es una matriz, no una expresión.
    d.requiereExpresion = false;
    d.requiereExpresionAuxiliar = false;
    d.etiquetaRaiz = "x";
    // Un sistema no tiene una curva que dibujar: el gráfico se oculta entero.
    d.muestraGrafico = false;
    d.muestraTrazado = false;
    // La estimación "k ≈ iteraciones para n cifras" es propia de la bisección;
    // aquí el criterio es la norma del error relativo.
    d.admiteEstimacionAbsoluta = false;
    // La columna VectorX se declara una sola vez; la tabla la expande en
    // x₁…x_n según la dimensión del sistema que se resolvió. El título de la
    // columna del error lleva el subíndice p entre paréntesis porque aquí la
    // norma tiene orden variable: poner «‖Δx‖∞» fijo mentiría en cuanto el
    // usuario cambiara el campo de la norma.
    d.columnas = {
        { "k", CampoIteracion::K },
        { "x", CampoIteracion::VectorX },
        { "‖Δx‖p", CampoIteracion::Norma },
        { "e_a (%)", CampoIteracion::Ea },
    };
    return d;
}

std::expected<void, ErrorMetodo>
Jacobi::validar(const std::vector<std::vector<double>>& matriz,
                const std::vector<double>& terminos) {
    const std::size_t n = matriz.size();
    if (n < static_cast<std::size_t>(kDimensionMinima)) {
        return std::unexpected(ErrorMetodo::DimensionInvalida);
    }
    if (n > static_cast<std::size_t>(kDimensionMaxima)) {
        return std::unexpected(ErrorMetodo::DimensionInvalida);
    }
    if (terminos.size() != n) {
        return std::unexpected(ErrorMetodo::DimensionInvalida);
    }
    for (const auto& fila : matriz) {
        if (fila.size() != n) {
            return std::unexpected(ErrorMetodo::DimensionInvalida);
        }
        for (const double valor : fila) {
            if (!std::isfinite(valor)) {
                return std::unexpected(ErrorMetodo::DimensionInvalida);
            }
        }
    }
    for (const double valor : terminos) {
        if (!std::isfinite(valor)) {
            return std::unexpected(ErrorMetodo::DimensionInvalida);
        }
    }
    return {};
}

bool Jacobi::esDiagonalmenteDominante(const std::vector<std::vector<double>>& matriz) {
    const std::size_t n = matriz.size();
    if (n < 2) {
        return false;
    }
    for (std::size_t i = 0; i < n; ++i) {
        if (matriz[i].size() != n) {
            return false;
        }
        if (!medirFila(matriz, i).dominante()) {
            return false;
        }
    }
    return true;
}

std::vector<DominanciaFila> Jacobi::dominanciaPorFila(
    const std::vector<std::vector<double>>& matriz) {
    std::vector<DominanciaFila> filas;
    filas.reserve(matriz.size());
    for (std::size_t i = 0; i < matriz.size(); ++i) {
        filas.push_back(medirFila(matriz, i));
    }
    return filas;
}

Reordenamiento Jacobi::reordenarParaDominancia(std::vector<std::vector<double>>& matriz,
                                               std::vector<double>& terminos) {
    Reordenamiento reordenamiento;
    const std::size_t n = matriz.size();
    if (n < 2) {
        return reordenamiento;
    }
    const std::size_t objetivo = n;

    // Cada pasada prueba todos los pares y aplica el que más filas dominantes
    // deja. Solo se aceptan intercambios que MEJOREN la cuenta, así que cada
    // pasada aplicada la incrementa al menos en uno y nunca pasa de n: n es una
    // cota suficiente y ajustada, no un tope arbitrario. Terminar sin haber
    // llegado al objetivo es un resultado legítimo —hay sistemas sin ningún orden
    // dominante— y eso no invalida la matriz, solo avisa de que la convergencia no
    // está garantizada.
    //
    // El coste importa: el panel de entrada llama a esta función en cada tecla
    // tecleada para anticipar el paso 1, así que hacer n² pasadas multiplicaría
    // ese trabajo por n sin cambiar nunca el resultado.
    for (std::size_t pasada = 0; pasada < n; ++pasada) {
        const std::size_t actuales = contarDominantes(matriz);
        if (actuales == objetivo) {
            break;
        }

        std::size_t mejorCuenta = actuales;
        std::size_t mejorOrigen = 0;
        std::size_t mejorDestino = 0;
        bool encontrado = false;

        for (std::size_t origen = 0; origen < n; ++origen) {
            for (std::size_t destino = 0; destino < n; ++destino) {
                if (origen == destino) {
                    continue;
                }
                std::swap(matriz[origen], matriz[destino]);
                std::swap(terminos[origen], terminos[destino]);
                const std::size_t cuenta = contarDominantes(matriz);
                std::swap(matriz[origen], matriz[destino]);
                std::swap(terminos[origen], terminos[destino]);
                if (cuenta > mejorCuenta) {
                    mejorCuenta = cuenta;
                    mejorOrigen = origen;
                    mejorDestino = destino;
                    encontrado = true;
                }
            }
        }

        if (!encontrado) {
            break;
        }

        // Ahora sí, el intercambio definitivo. La iterada inicial NO se toca: el
        // lugar i sigue despejando la variable i tras el intercambio (su diagonal
        // es el coeficiente de esa variable), así que x⁰ está indexado por
        // variable y no hay nada que reordenar aquí. Ver el comentario del
        // encabezado.
        std::swap(matriz[mejorOrigen], matriz[mejorDestino]);
        std::swap(terminos[mejorOrigen], terminos[mejorDestino]);
        reordenamiento.intercambios.emplace_back(mejorOrigen, mejorDestino);
    }

    // `aplicado` responde a «¿hubo que mover ecuaciones?», no a «¿se consiguió la
    // dominancia?». Lo segundo se lee en `Resultado::dominanteEstricto`, que se
    // calcula sobre la matriz final. Confundir ambos dos casos muy distintos:
    // un sistema que no se pudo ordenar tiene `aplicado` true e
    // `dominanteEstricto` false, y esa es justo la combinación que hay que
    // poder avisar.
    reordenamiento.aplicado = !reordenamiento.intercambios.empty();
    return reordenamiento;
}

double Jacobi::norma(std::vector<double> valores, int p) {
    if (valores.empty()) {
        return 0.0;
    }
    const int orden = std::max(1, p);
    double suma = 0.0;
    for (const double componente : valores) {
        // std::pow con exponente entero exacto y base no negativa: para p alto,
        // multiplicar p veces acumula menos error de redondeo que una potencia
        // fraccionaria, que es lo que hace la raíz final.
        const double absoluto = std::fabs(componente);
        double potencia = 1.0;
        for (int i = 0; i < orden; ++i) {
            potencia *= absoluto;
        }
        suma += potencia;
    }
    return std::pow(suma, 1.0 / static_cast<double>(orden));
}

std::optional<FormulaSolucion> Jacobi::formulaCramer(
    const std::vector<std::vector<double>>& matriz,
    const std::vector<double>& terminos) {
    if (const auto forma = validar(matriz, terminos); !forma) {
        return std::nullopt;
    }
    const std::size_t n = matriz.size();

    const double det = determinante(matriz);
    if (!std::isfinite(det) || std::fabs(det) < kCotaDeterminante) {
        return std::nullopt;
    }

    FormulaSolucion formula;
    formula.determinante = det;
    formula.determinantes.reserve(n);
    formula.valores.reserve(n);

    for (std::size_t i = 0; i < n; ++i) {
        // Dx, Dy… se obtienen cambiando en A la columna i por el vector b. Es la
        // misma matriz con una columna distinta, así que el determinante no es
        // arbitrario: es el mismo tipo de número que D.
        std::vector<std::vector<double>> modificada = matriz;
        for (std::size_t fila = 0; fila < n; ++fila) {
            modificada[fila][i] = terminos[fila];
        }
        const double detParcial = determinante(modificada);
        formula.determinantes.push_back(detParcial);
        formula.valores.push_back(detParcial / det);
    }

    // Una solución con componentes no finitos viene de una división que se
    // desbordó; en ese caso vale más no presentar nada que presentar un nan.
    for (const double valor : formula.valores) {
        if (!std::isfinite(valor)) {
            return std::nullopt;
        }
    }
    return formula;
}

std::expected<Resultado, ErrorMetodo>
Jacobi::resolver(const Entrada& entrada, std::stop_token detener) const {
    // Sistema por defecto cuando no se escribió matriz: el 3×3 del enunciado,
    // que además viene en el ORDEN en que está escrito allí a propósito, con solo
    // la fila 2 dominante, para que el paso 1 del procedimiento tenga algo que
    // reordenar y se vea que funciona.
    static const std::vector<std::vector<double>> kMatrizPorDefecto = {
        { 1.0, 1.0, 4.0 },
        { -2.0, 4.0, 1.0 },
        { 6.0, 3.0, -2.0 },
    };
    static const std::vector<double> kTerminosPorDefecto = { 15.0, 9.0, 6.0 };

    const std::vector<std::vector<double>>& matriz =
        entrada.matriz.empty() ? kMatrizPorDefecto : entrada.matriz;
    const std::vector<double>& terminos =
        entrada.terminos.empty() ? kTerminosPorDefecto : entrada.terminos;

    // Si la entrada declara una dimensión que no cuadra con la matriz, es un
    // estado incoherente de la GUI: mejor rechazarlo que resolver de más o de
    // menos filas.
    if (entrada.dimension > 0 &&
        static_cast<std::size_t>(entrada.dimension) != matriz.size()) {
        return std::unexpected(ErrorMetodo::DimensionInvalida);
    }

    const int cifras = std::clamp(entrada.cifras, 1, 12);
    const double es = 0.5 * std::pow(10.0, 2.0 - cifras);
    // Holgado a propósito: con la dominante estricta la convergencia es
    // geométrica, pero sin ella puede ser lenta. 50 por ecuación deja margen de
    // sobra sin convertir el tope en un lazo infinito en la práctica.
    const int maxIter = std::max(100, static_cast<int>(matriz.size()) * 50);

    return resolver(matriz, terminos, entrada.vectorInicial, es, maxIter, std::move(detener),
                    entrada.normaP);
}

std::expected<Resultado, ErrorMetodo>
Jacobi::resolver(const std::vector<std::vector<double>>& matriz,
                 const std::vector<double>& terminos,
                 const std::vector<double>& iteradaInicial,
                 double toleranciaEsPorcentaje,
                 int maxIteraciones,
                 std::stop_token detener,
                 int normaP) const {
    if (const auto forma = validar(matriz, terminos); !forma) {
        return std::unexpected(forma.error());
    }

    const std::size_t n = matriz.size();

    Resultado resultado;
    resultado.tipo = TipoResolucion::Sistema;
    resultado.normaP = std::max(1, normaP);

    // ---- Paso 1 del procedimiento: verificación de la convergencia ----
    //
    // Se guarda el sistema tal como llegó antes de tocarlo. Sin esta copia, la
    // narración describiría el sistema ya reordenado y el usuario no vería qué
    // ecuaciones hubo que mover —que es justo lo que el paso 1 pide enseñar—.
    resultado.matrizOriginal = matriz;
    resultado.terminosOriginal = terminos;
    resultado.dominanciaOriginal = dominanciaPorFila(matriz);

    // La iterada inicial se normaliza a n componentes: si el usuario no escribió
    // ninguna, o escribió otra de otro tamaño, se parte del vector nulo, que es
    // la elección neutra y en un sistema dominante lleva a la solución igual que
    // cualquier otra.
    std::vector<double> inicial(n, 0.0);
    if (iteradaInicial.size() == n) {
        inicial = iteradaInicial;
    }

    // `matriz` y `terminos` son referencias a datos del llamante: el sistema se
    // reordena sobre copias. `inicial` ya es una copia propia.
    std::vector<std::vector<double>> sistema = matriz;
    std::vector<double> lados = terminos;

    resultado.reordenamiento = reordenarParaDominancia(sistema, lados);

    // A partir de aquí, `sistema`/`lados`/`inicial` son los que se iteran.
    resultado.matriz = sistema;
    resultado.terminos = lados;
    resultado.dominanteEstricto = esDiagonalmenteDominante(sistema);
    resultado.dominanciaFinal = dominanciaPorFila(sistema);

    // Ya en el orden en el que se va a iterar, la iterada inicial es `x`.
    std::vector<double> x = inicial;

    // ---- Fórmula de solución: la referencia contra la que se mide ----
    // Se calcula sobre el sistema ya ordenado, que es el que se resuelve. Mover
    // filas cambia el signo del determinante pero no los valores de Dx/D, así que
    // la solución es la misma y no hay que distinguir entre las dos matrices.
    resultado.formulaSolucion = formulaCramer(sistema, lados);

    std::vector<double> xAnterior;

    // La tabla empieza en k = 0 con la iterada inicial, como en el enunciado.
    // `k <= maxIteraciones` da maxIteraciones pasos y una fila más.
    for (int k = 0; k <= maxIteraciones; ++k) {
        if (detener.stop_requested()) {
            resultado.motivo = MotivoParada::Interrumpido;
            break;
        }

        Iteracion iteracion;
        iteracion.k = k;
        iteracion.vectorX = x;

        // El error mide cuánto cambió el vector AL LLEGAR a él. En k = 0 no
        // existe x^(−1) con el que comparar, así que la iterada inicial se
        // registra sin error: es la fila que el enunciado imprime sin columna de
        // error al principio de la tabla.
        if (k > 0) {
            std::vector<double> diferencia(n, 0.0);
            for (std::size_t i = 0; i < n; ++i) {
                diferencia[i] = x[i] - xAnterior[i];
            }
            iteracion.norma = norma(diferencia, resultado.normaP);

            // Los otros dos errores del enunciado, uno por componente. El relativo
            // necesita el valor absoluto del denominador distinto de cero, así que
            // se deja vacío ahí en vez de guardar un cero que parece una
            // aproximación perfecta.
            iteracion.errorAbsoluto.resize(n);
            iteracion.errorRelativo.resize(n);
            for (std::size_t i = 0; i < n; ++i) {
                iteracion.errorAbsoluto[i] = std::fabs(diferencia[i]);
                const double absolutoX = std::fabs(x[i]);
                if (absolutoX > 0.0) {
                    iteracion.errorRelativo[i] = iteracion.errorAbsoluto[i] / absolutoX * 100.0;
                }
            }

            const double normaX = norma(x, resultado.normaP);
            if (normaX > 0.0) {
                const double ea = (*iteracion.norma / normaX) * 100.0;
                iteracion.eaPorcentaje = ea;
                if (ea < toleranciaEsPorcentaje) {
                    resultado.motivo = MotivoParada::ToleranciaAlcanzada;
                }
            }
        }

        resultado.iteraciones.push_back(iteracion);

        if (resultado.motivo == MotivoParada::ToleranciaAlcanzada) {
            break;
        }

        // Solución exacta: el residuo se anula componente a componente.
        if (normaInfinita(residuo(sistema, lados, x)) == 0.0) {
            resultado.motivo = MotivoParada::SolucionExacta;
            break;
        }

        // Paso de Jacobi. Antes de dividir, se comprueba que TODAS las diagonales
        // sirvan: si una sola se anula, el método no puede avanzar y avisar en la
        // fila en la que ocurre es más útil que dejar un NaN en la tabla.
        std::vector<double> xSiguiente(n, 0.0);
        bool diagonalUtilizable = true;
        for (std::size_t i = 0; i < n; ++i) {
            const double diagonal = sistema[i][i];
            if (!std::isfinite(diagonal) || std::fabs(diagonal) < kEpsilonDiagonal) {
                diagonalUtilizable = false;
                break;
            }
        }
        if (!diagonalUtilizable) {
            resultado.motivo = MotivoParada::DiagonalNula;
            break;
        }

        for (std::size_t i = 0; i < n; ++i) {
            // Todo el sumatorio usa x (la iterada ANTERIOR): ninguna componente
            // lee el valor que otra acaba de calcular. Ahí está la diferencia con
            // Gauss-Seidel. Y es el despeje del paso 2 del procedimiento, escrito
            // con el signo que resulta de pasar los términos al otro lado.
            double suma = lados[i];
            for (std::size_t j = 0; j < n; ++j) {
                if (j != i) {
                    suma -= sistema[i][j] * x[j];
                }
            }
            xSiguiente[i] = suma / sistema[i][i];
        }

        bool estable = true;
        for (const double componente : xSiguiente) {
            if (!std::isfinite(componente) || std::fabs(componente) > kCotaDivergente) {
                estable = false;
                break;
            }
        }
        if (!estable) {
            resultado.motivo = MotivoParada::Divergente;
            break;
        }

        xAnterior = x;
        x = std::move(xSiguiente);
    }

    return resultado;
}

}  // namespace biseccion