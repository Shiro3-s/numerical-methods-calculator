// Funcion.cpp
// -----------------------------------------------------------------------------
// Analizador descendente recursivo para expresiones matemáticas f(x).
// Gramática:
//   expresion  := termino  (('+' | '-') termino)*
//   termino    := unario   (('*' | '/' | <implicito>) unario)*
//   unario     := '-' unario | potencia
//   potencia   := primario ('^' unario)?            (asociativa a la derecha)
//   primario   := numero | identificador | '(' expresion ')'
// Multiplicación implícita: cuando al final de un término el siguiente símbolo
// inicia un factor (dígito, '(', identificador o π) se asume un producto.
// -----------------------------------------------------------------------------
#include "Funcion.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace biseccion {

double evaluar(const Nodo& nodo, double x) {
    return std::visit(
        [x](const auto& n) -> double {
            using T = std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<T, NodoNumero>) {
                return n.valor;
            } else if constexpr (std::is_same_v<T, NodoVariable>) {
                return x;
            } else if constexpr (std::is_same_v<T, NodoBinario>) {
                const double izquierda = evaluar(*n.izquierdo, x);
                const double derecha = evaluar(*n.derecho, x);
                switch (n.op) {
                    case NodoBinario::Op::Suma: return izquierda + derecha;
                    case NodoBinario::Op::Resta: return izquierda - derecha;
                    case NodoBinario::Op::Multiplicacion: return izquierda * derecha;
                    case NodoBinario::Op::Division: return izquierda / derecha;
                    case NodoBinario::Op::Potencia: return std::pow(izquierda, derecha);
                }
                return 0.0;
            } else if constexpr (std::is_same_v<T, NodoUnario>) {
                return -evaluar(*n.hijo, x);
            } else {
                const double argumento = evaluar(*n.argumento, x);
                switch (n.funcion) {
                    case TipoFuncion::Sin: return std::sin(argumento);
                    case TipoFuncion::Cos: return std::cos(argumento);
                    case TipoFuncion::Tan: return std::tan(argumento);
                    case TipoFuncion::Asin: return std::asin(argumento);
                    case TipoFuncion::Acos: return std::acos(argumento);
                    case TipoFuncion::Atan: return std::atan(argumento);
                    case TipoFuncion::Sinh: return std::sinh(argumento);
                    case TipoFuncion::Cosh: return std::cosh(argumento);
                    case TipoFuncion::Tanh: return std::tanh(argumento);
                    case TipoFuncion::Ln: return std::log(argumento);
                    case TipoFuncion::Log10: return std::log10(argumento);
                    case TipoFuncion::Log2: return std::log2(argumento);
                    case TipoFuncion::Exp: return std::exp(argumento);
                    case TipoFuncion::Sqrt: return std::sqrt(argumento);
                    case TipoFuncion::Cbrt: return std::cbrt(argumento);
                    case TipoFuncion::Abs: return std::fabs(argumento);
                    case TipoFuncion::Floor: return std::floor(argumento);
                    case TipoFuncion::Ceil: return std::ceil(argumento);
                    case TipoFuncion::Signo:
                        return (argumento > 0.0) ? 1.0 : ((argumento < 0.0) ? -1.0 : 0.0);
                }
                return 0.0;
            }
        },
        nodo.datos);
}

namespace {

// ¿Es un símbolo simple (constante o función sin '(') que puede preceder a una
// 'x' pegada, p. ej. "ex", "pix", "sinx"?
[[nodiscard]] bool esSimboloConocido(const std::string& nombre) {
    static constexpr std::array<std::string_view, 26> simbolos = {
        "e",      "pi",     "\xCF\x80", "tau",    "\xCF\x84",
        "sin",    "cos",    "tan",      "ln",     "log",
        "exp",    "sqrt",   "cbrt",     "abs",    "floor",
        "ceil",   "sign",   "sgn",      "asin",   "acos",
        "atan",   "sinh",   "cosh",     "tanh",   "log2",
        "log10",
    };
    return std::ranges::find(simbolos, nombre) != simbolos.end();
}

// Analizador de un solo paso; consume los caracteres de 'expresion'.
class Analizador {
public:
    explicit Analizador(const std::string& expresion) : expresion_(expresion) {}

    [[nodiscard]] Nodo analizar() { return *parsearExpresion(); }

    [[nodiscard]] std::size_t posicion() const { return pos_; }

    [[nodiscard]] bool agotado() {
        ignorarEspacios();
        return pos_ >= expresion_.size();
    }

    [[nodiscard]] std::expected<Nodo, std::string> parsearExpresion() {
        auto izquierdo = parsearTermino();
        if (!izquierdo) {
            return izquierdo;
        }
        for (;;) {
            ignorarEspacios();
            const char c = actual();
            if (c != '+' && c != '-') {
                break;
            }
            ++pos_;
            auto derecho = parsearTermino();
            if (!derecho) {
                return derecho;
            }
            NodoBinario binario;
            binario.op = (c == '+') ? NodoBinario::Op::Suma : NodoBinario::Op::Resta;
            binario.izquierdo = std::make_unique<Nodo>(std::move(*izquierdo));
            binario.derecho = std::make_unique<Nodo>(std::move(*derecho));
            izquierdo = Nodo{std::move(binario)};
        }
        return izquierdo;
    }

    [[nodiscard]] std::expected<Nodo, std::string> parsearTermino() {
        auto izquierdo = parsearUnario();
        if (!izquierdo) {
            return izquierdo;
        }
        for (;;) {
            ignorarEspacios();
            const char c = actual();
            if (c == '*' || c == '/') {
                ++pos_;  // operador explícito
            } else if (empiezaFactor()) {
                // Multiplicación implícita: "5x", "2sin(x)", "(x-1)(x+2)"…
            } else {
                break;
            }
            auto derecho = parsearUnario();
            if (!derecho) {
                return derecho;
            }
            NodoBinario binario;
            binario.op = (c == '/') ? NodoBinario::Op::Division
                                    : NodoBinario::Op::Multiplicacion;
            binario.izquierdo = std::make_unique<Nodo>(std::move(*izquierdo));
            binario.derecho = std::make_unique<Nodo>(std::move(*derecho));
            izquierdo = Nodo{std::move(binario)};
        }
        return izquierdo;
    }

    [[nodiscard]] std::expected<Nodo, std::string> parsearUnario() {
        ignorarEspacios();
        if (actual() == '-') {
            ++pos_;
            auto hijo = parsearUnario();
            if (!hijo) {
                return hijo;
            }
            NodoUnario unario;
            unario.op = NodoUnario::Op::Negacion;
            unario.hijo = std::make_unique<Nodo>(std::move(*hijo));
            return Nodo{std::move(unario)};
        }
        return parsearPotencia();
    }

    [[nodiscard]] std::expected<Nodo, std::string> parsearPotencia() {
        auto base = parsearPrimario();
        if (!base) {
            return base;
        }
        ignorarEspacios();
        // Potencia: se admite tanto «^» como «**» (estilo Python). La comprobación
        // de «**» usa mirilla de dos caracteres para no confundirlo con un único
        // «*» de la multiplicación explícita (p. ej. "2x*3y" no dispara potencia).
        const bool dobleAsterisco = actual() == '*' &&
                                    pos_ + 1 < expresion_.size() &&
                                    expresion_[pos_ + 1] == '*';
        if (dobleAsterisco || actual() == '^') {
            pos_ += dobleAsterisco ? 2 : 1;
            auto exponente = parsearUnario();  // asociativa a la derecha
            if (!exponente) {
                return exponente;
            }
            NodoBinario binario;
            binario.op = NodoBinario::Op::Potencia;
            binario.izquierdo = std::make_unique<Nodo>(std::move(*base));
            binario.derecho = std::make_unique<Nodo>(std::move(*exponente));
            return Nodo{std::move(binario)};
        }
        return base;
    }

    [[nodiscard]] std::expected<Nodo, std::string> parsearPrimario() {
        ignorarEspacios();
        const char c = actual();
        if (agotado()) {
            return std::unexpected(
                std::format("la expresión termina de forma inesperada (posición {})", pos_));
        }
        if (c == '(') {
            ++pos_;
            auto interior = parsearExpresion();
            if (!interior) {
                return interior;
            }
            ignorarEspacios();
            if (actual() != ')') {
                return std::unexpected(
                    std::format("falta el paréntesis de cierre ')' (posición {})", pos_));
            }
            ++pos_;
            return interior;
        }
        if (std::isdigit(static_cast<unsigned char>(c))) {
            return parsearNumero();
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' ||
            static_cast<unsigned char>(c) == 0xCF /* primer byte de π/τ (UTF-8) */) {
            return parsearIdentificador();
        }
        return std::unexpected(
            std::format("símbolo inesperado '{}' (posición {})", c, pos_));
    }

private:
    [[nodiscard]] char actual() const {
        return pos_ < expresion_.size() ? expresion_[pos_] : '\0';
    }

    void ignorarEspacios() {
        while (pos_ < expresion_.size() &&
               std::isspace(static_cast<unsigned char>(expresion_[pos_]))) {
            ++pos_;
        }
    }

    // ¿El siguiente símbolo inicia un factor (para la multiplicación implícita)?
    [[nodiscard]] bool empiezaFactor() const {
        if (pos_ >= expresion_.size()) {
            return false;
        }
        const unsigned char b = static_cast<unsigned char>(expresion_[pos_]);
        if (std::isdigit(b) || b == '(') {
            return true;
        }
        if (std::isalpha(b) || expresion_[pos_] == '_') {
            return true;
        }
        return b == 0xCF;  // π o τ en UTF-8
    }

    [[nodiscard]] std::expected<Nodo, std::string> parsearNumero() {
        const std::size_t inicio = pos_;
        while (std::isdigit(static_cast<unsigned char>(actual()))) {
            ++pos_;
        }
        if (actual() == '.') {
            ++pos_;
            while (std::isdigit(static_cast<unsigned char>(actual()))) {
                ++pos_;
            }
        }
        // Exponente científico opcional (p. ej. 1e-3). Con mirilla: solo se
        // considera parte del número si hay al menos un dígito tras el signo;
        // por eso "2ex" se lee como 2·e·x y no como un número inválido.
        if (actual() == 'e' || actual() == 'E') {
            std::size_t mirilla = pos_ + 1;
            if (mirilla < expresion_.size() &&
                (expresion_[mirilla] == '+' || expresion_[mirilla] == '-')) {
                ++mirilla;
            }
            if (mirilla < expresion_.size() &&
                std::isdigit(static_cast<unsigned char>(expresion_[mirilla]))) {
                pos_ = mirilla;
                while (pos_ < expresion_.size() &&
                       std::isdigit(static_cast<unsigned char>(expresion_[pos_]))) {
                    ++pos_;
                }
            }
        }
        const std::string token = expresion_.substr(inicio, pos_ - inicio);
        char* final = nullptr;
        const double valor = std::strtod(token.c_str(), &final);
        if (final == token.c_str() || *final != '\0') {
            return std::unexpected(
                std::format("número no válido '{}' (posición {})", token, inicio));
        }
        NodoNumero numero;
        numero.valor = valor;
        return Nodo{numero};
    }

    [[nodiscard]] std::expected<Nodo, std::string> parsearIdentificador() {
        const std::size_t inicio = pos_;
        // Consume letras, '_', π y τ.
        while (pos_ < expresion_.size()) {
            const unsigned char b = static_cast<unsigned char>(expresion_[pos_]);
            if (std::isalpha(b) || expresion_[pos_] == '_') {
                ++pos_;
                continue;
            }
            // π = 0xCF 0x80 y τ = 0xCF 0x84 en UTF-8.
            if (b == 0xCF && pos_ + 1 < expresion_.size()) {
                const unsigned char b2 = static_cast<unsigned char>(expresion_[pos_ + 1]);
                if (b2 == 0x80 || b2 == 0x84) {
                    pos_ += 2;
                    continue;
                }
            }
            break;
        }
        // Sufijo numérico: la base va dentro del nombre (log2, log10, …).
        if (expresion_.compare(inicio, pos_ - inicio, "log") == 0) {
            while (pos_ < expresion_.size() &&
                   std::isdigit(static_cast<unsigned char>(expresion_[pos_]))) {
                ++pos_;
            }
        }
        std::string token = expresion_.substr(inicio, pos_ - inicio);

        // Símbolo pegado a una 'x' final: "ex" es e·x y "2ex" es 2·e·x.
        // Se devuelve la 'x' al flujo para que la multiplicación implícita
        // la convierta en un producto ("sinx" pedirá '(x)' explícitos).
        if (token.size() > 1 && token.back() == 'x' &&
            esSimboloConocido(token.substr(0, token.size() - 1))) {
            --pos_;
            token.pop_back();
        }

        if (token == "x") {
            return Nodo{NodoVariable{}};
        }
        if (token == "pi" || token == "\xCF\x80") {
            return Nodo{NodoNumero{PI}};
        }
        if (token == "e") {
            return Nodo{NodoNumero{EULER}};
        }
        if (token == "tau" || token == "\xCF\x84") {
            return Nodo{NodoNumero{TAU}};
        }

        TipoFuncion funcion;
        if (token == "sin") {
            funcion = TipoFuncion::Sin;
        } else if (token == "cos") {
            funcion = TipoFuncion::Cos;
        } else if (token == "tan") {
            funcion = TipoFuncion::Tan;
        } else if (token == "asin") {
            funcion = TipoFuncion::Asin;
        } else if (token == "acos") {
            funcion = TipoFuncion::Acos;
        } else if (token == "atan") {
            funcion = TipoFuncion::Atan;
        } else if (token == "sinh") {
            funcion = TipoFuncion::Sinh;
        } else if (token == "cosh") {
            funcion = TipoFuncion::Cosh;
        } else if (token == "tanh") {
            funcion = TipoFuncion::Tanh;
        } else if (token == "ln") {
            funcion = TipoFuncion::Ln;
        } else if (token == "log" || token == "log10") {
            funcion = TipoFuncion::Log10;
        } else if (token == "log2") {
            funcion = TipoFuncion::Log2;
        } else if (token == "exp") {
            funcion = TipoFuncion::Exp;
        } else if (token == "sqrt") {
            funcion = TipoFuncion::Sqrt;
        } else if (token == "cbrt") {
            funcion = TipoFuncion::Cbrt;
        } else if (token == "abs") {
            funcion = TipoFuncion::Abs;
        } else if (token == "floor") {
            funcion = TipoFuncion::Floor;
        } else if (token == "ceil") {
            funcion = TipoFuncion::Ceil;
        } else if (token == "sign" || token == "sgn") {
            funcion = TipoFuncion::Signo;
        } else {
            return std::unexpected(
                std::format("función o símbolo desconocido '{}' (posición {})", token, inicio));
        }

        // Las funciones requieren argumento entre paréntesis.
        ignorarEspacios();
        if (actual() != '(') {
            return std::unexpected(std::format(
                "la función '{}' requiere un paréntesis '(' (posición {})", token, pos_));
        }
        ++pos_;
        auto argumento = parsearExpresion();
        if (!argumento) {
            return argumento;
        }
        ignorarEspacios();
        if (actual() != ')') {
            return std::unexpected(std::format(
                "falta el paréntesis de cierre ')' de '{}' (posición {})", token, pos_));
        }
        ++pos_;

        NodoAplicada aplicada;
        aplicada.funcion = funcion;
        aplicada.argumento = std::make_unique<Nodo>(std::move(*argumento));
        return Nodo{std::move(aplicada)};
    }

    const std::string& expresion_;
    std::size_t pos_ = 0;
};

}  // namespace

std::expected<Funcion, ErrorParser> parsearFuncion(const std::string& expresion) {
    Analizador analizador(expresion);

    auto nodo = analizador.parsearExpresion();
    if (!nodo) {
        return std::unexpected(ErrorParser{std::move(nodo.error()), analizador.posicion()});
    }
    if (!analizador.agotado()) {
        return std::unexpected(ErrorParser{
            std::format("caracteres inesperados al final de la expresión (posición {})",
                        analizador.posicion()),
            analizador.posicion()});
    }

    Funcion funcion;
    // Texto canónico sin espacios redundantes.
    std::string texto;
    texto.reserve(expresion.size());
    bool espacioPrevio = false;
    for (const char c : expresion) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            espacioPrevio = !texto.empty();
        } else {
            if (espacioPrevio) {
                texto.push_back(' ');
                espacioPrevio = false;
            }
            texto.push_back(c);
        }
    }
    funcion.texto = std::move(texto);

    auto puntero = std::make_shared<Nodo>(std::move(*nodo));
    funcion.f = [puntero](double x) {
        return evaluar(*puntero, x);
    };
    return funcion;
}

}  // namespace biseccion