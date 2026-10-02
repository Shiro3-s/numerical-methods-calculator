# Métodos Numéricos — Aplicación Interactiva (C++ / Qt 6)

Calculadora de escritorio para **métodos numéricos**, con entrada de funciones,
tabla de iteraciones, gráfico interactivo y vista procedimental paso a paso. Los
decimales en pantalla se limitan **dinámicamente** según las cifras significativas
que se piden.

Tres métodos, seleccionables desde un desplegable:

| Método | Qué resuelve | Datos de entrada |
|---|---|---|
| **Bisección** | la raíz de `f(x) = 0` dentro de un intervalo | `f(x)`, `a`, `b`, `n` |
| **Newton-Raphson** | la raíz de `f(x) = 0` desde un punto | `f(x)`, `f'(x)`, `x₀`, `n` |
| **Jacobi** | un sistema lineal `A·x = b` de `n` ecuaciones | la matriz, `b`, `x⁰`, el orden `p` de la norma, `n` |

**Compilar y ejecutar en Linux** (todo el detalle, con Windows y macOS, en
[§ 1](#1-instalación-y-ejecución)):

```bash
# Arch Linux
sudo pacman -S --needed base-devel cmake qt6-base

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/Biseccion
```

> **Cómo está organizado el código.** El núcleo (`src/core/`) es C++ estándar
> puro, sin una sola dependencia de Qt, y cada método implementa un contrato de dos
> métodos (`descriptor()` y `resolver()`). La interfaz (`src/gui/`) está **dirigida
> por el descriptor**: el descriptor declara qué columnas tiene la tabla, qué
> campos se piden, si hay gráfico y si se dibuja la recta asociada, y la interfaz
> se reconfigura sola. Añadir un método es escribir su núcleo y registrarlo; no
> hay que tocar ni la tabla ni el gráfico ni el panel de entrada. Ver
> [§ 7 · Añadir un método nuevo](#7-añadir-un-método-nuevo).

---

## Índice

- [1 · Instalación y ejecución](#1-instalación-y-ejecución)
- [2 · Modos de consola](#2-modos-de-consola)
- [3 · Arquitectura](#3-arquitectura)
- [4 · Los algoritmos](#4-los-algoritmos)
- [5 · La interfaz](#5-la-interfaz)
- [6 · Ejercicios de referencia](#6-ejercicios-de-referencia)
- [7 · Añadir un método nuevo](#7-añadir-un-método-nuevo)
- [8 · Pruebas](#8-pruebas)
- [9 · Licencias](#9-licencias)

---

## 1. Instalación y ejecución

### Qué hace falta

| Dependencia | Versión | Para qué |
|---|---|---|
| CMake | ≥ 3.21 | construir el proyecto |
| Compilador con **C++23** | GCC 14+ · Clang 19+ · MSVC 19.36+ | el código (`std::expected`, `std::print`, `std::jthread`) |
| Qt 6 | ≥ 6.2 | Widgets, PrintSupport y el loop de eventos |
| QCustomPlot | 2.1.1 | el gráfico interactivo |

El mínimo del compilador lo fija `std::print`, que es lo más moderno que usa el
proyecto: llegó a libstdc++ en GCC 14 y a libc++ en LLVM 19. `std::expected` (GCC
13) y `std::jthread` (GCC 10) llegaron antes, así que el que manda es `<print>`.

**Verificado en esta máquina** con **GCC 16.2.1** y **Clang 22.1.8**, **CMake
4.4.3** y **Qt 6.11.2**. Con ambos compiladores pasan las 6 pruebas, los 6 modos de
consola y el autotest; Clang usa la libstdc++ de GCC, no libc++, así que libc++
sigue sin probarse.

QCustomPlot no viene empaquetado en casi ninguna distribución, así que va
**incluido en el repositorio** en `third_party/qcustomplot/` (`qcustomplot.h` y
`qcustomplot.cpp`, de `QCustomPlot-source.tar.gz` 2.1.1, licencia **GPL-3.0** en
`GPL.txt`). No hace falta descargarlo ni instalarlo.

### Instalar las dependencias

<details open>
<summary><b>Arch Linux</b></summary>

```bash
sudo pacman -S --needed base-devel cmake qt6-base
```

`base-devel` trae GCC y las cabeceras estándar. En Qt 6, el paquete `qt6-base`
incluye Widgets y PrintSupport; no hace falta instalar nada más.

Con Clang en vez de GCC, o con los dos a la vez:

```bash
sudo pacman -S --needed clang
```
</details>

<details>
<summary><b>Debian / Ubuntu</b></summary>

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev
```

Nota: en Ubuntu 22.04 y anteriores `qt6-base-dev` no existe (Qt 6 no está en los
repositorios); hace falta una distribución de 24.04 en adelante.
</details>

<details>
<summary><b>Fedora</b></summary>

```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel
```
</details>

<details>
<summary><b>macOS (Homebrew)</b> — documentado, sin verificar</summary>

```bash
xcode-select --install
brew install cmake qt
```

Qt va a `/opt/homebrew/opt/qt`, así que CMake lo encuentra solo si se le dice dónde
buscar:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
```
</details>

<details>
<summary><b>Windows</b> — documentado, sin verificar</summary>

Con MSVC, desde «Developer PowerShell for VS 2022» (o cualquier terminal con el
entorno de desarrollo abierto):

```powershell
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=cl -DCMAKE_BUILD_TYPE=Release
cmake --build build
.\build\Release\Biseccion.exe
```

Si CMake no encuentra Qt 6 —si se instaló desde `qt.io` y no quedó en el
`PATH`—, se le indica la ruta:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="C:/Qt/6.11.2/msvc2022_64"
```
</details>

> ### ⚠️ Qué está verificado y qué no
>
> | Sistema | Estado |
> |---|---|
> | **Arch Linux** | ✅ **verificado**: es la máquina de desarrollo. Compila limpio con GCC y con Clang; pasan las 6 pruebas de CTest, los 6 modos de consola y el autotest |
> | Debian / Ubuntu | ⚠️ **no verificado**. El nombre del paquete (`qt6-base-dev`) es el correcto para Qt 6, pero no se ha probado en ninguna de las dos |
> | Fedora | ⚠️ **no verificado**. Igual: `qt6-qtbase-devel` es el paquete correcto, sin ninguna prueba detrás |
> | macOS | ⚠️ **no verificado**. Necesita además `-DCMAKE_PREFIX_PATH`, porque Qt no se registra en el `PATH` |
> | Windows | ⚠️ **no verificado**. Ver la nota de abajo |
>
> Los tres sistemas sin verificar **funcionan en principio**: el `CMakeLists.txt`
> es un proyecto CMake estándar, sin scripts propios de shell ni rutas absoluta,
> y las instrucciones son las que corresponden a un proyecto de ese tipo. Lo que
> no hay es una ejecución real detrás, así que no deben darse por buenas sin
> probarlas. En Windows quedan dos puntos que solo una prueba puede resolver:
> el `QStandardPaths::TempLocation` del autotest, que devuelve la carpeta del
> usuario y no `/tmp`, y el orden de `cmake --build` con el generador Ninja (que
> no usa configuraciones, así que el binario queda en `build\` y no en
> `build\Release\`).

### Compilar

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

En Windows no hay `nproc`, y con el generador Ninja (el que se usa con MSVC) el
`CMAKE_BUILD_TYPE` no aplica —no hay configuraciones—, así que el binario queda
en `build\` y no en `build\Release\`:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=cl
cmake --build build
```

Con el generador por defecto de Visual Studio, en cambio, sí hacen falta las
configuraciones:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Para depurar:

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -j"$(nproc)"
```

Con Clang, que también está probado:

```bash
cmake -S . -B build-clang -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build-clang -j"$(nproc)"
ctest --test-dir build-clang
```

El directorio de compilación es libre: `build`, `build-clang`, `build-debug`,
`build-asan` conviven sin problema, porque CMake guarda su caché en el directorio
que se le da y no en el código fuente.

### Ejecutar

```bash
./build/Biseccion
```

En Windows, según el generador:

```powershell
.\build\Biseccion.exe            # con Ninja
.\build\Release\Biseccion.exe    # con el generador de Visual Studio
```

Si la ventana no aparece y el equipo es un servidor sin pantalla, se fuerza el
renderizador por software:

```bash
QT_QPA_PLATFORM=offscreen ./build/Biseccion --ayuda
```

### Opciones de configuración

| Opción | Por defecto | Qué hace |
|---|---|---|
| `ENABLE_SANITIZERS` | `OFF` | compila con ASan + UBSan |
| `ENABLE_TSAN` | `OFF` | compila con ThreadSanitizer |
| `STRICT_WARNINGS` | `ON` | `-Werror`; con `OFF` las advertencias no paran la compilación |
| `BUILD_GUI_TESTS` | `ON` | con `OFF` no compila `gui_tests` (la aplicación sigue necesitando Qt) |

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build-asan -j"$(nproc)"
ctest --test-dir build-asan --output-on-failure -E gui_tests
```

`ENABLE_TSAN` y `ENABLE_SANITIZERS` son excluyentes: ASan y TSan no conviven en el
mismo binario.

El proyecto se compila con `-Wall -Wextra -Wpedantic -Werror`, `-fPIE -pie`,
`-D_FORTIFY_SOURCE=3`, `-fstack-protector-strong`, `-fstack-clash-protection`,
`-ftrivial-auto-var-init=zero` y `-Wl,-z,relro,-z,now`. QCustomPlot se compila con
las advertencias silenciadas (`-w`) para que no contamine los diagnósticos del
código propio.

---

## 2. Modos de consola

Además de la ventana, el mismo binario tiene modos que imprimen la solución
matemática por terminal. Sirven para verificar los números sin abrir la interfaz y
son los que usan las pruebas.

```bash
./build/Biseccion --ayuda        # lista de modos (alias: --help, -h)
```

| Modo | Qué imprime |
|---|---|
| *(sin argumento)* | la interfaz gráfica |
| `--ayuda` | esta lista de modos |
| `--metodos` | el catálogo: para cada método, sus columnas, su tipo de entrada y si dibuja gráfico |
| `--verificar` | raíz, `f(raíz)` e iteraciones necesarias de los cuatro ejercicios, para `n = 4, 5, 6` |
| `--tablas` | las tablas completas de iteraciones de los cuatro ejercicios |
| `--comparar` | bisección frente a Newton-Raphson en las mismas raíces |
| `--sistema` | el sistema lineal del enunciado resuelto con Jacobi, paso a paso |

Ejemplos:

```bash
./build/Biseccion --verificar
./build/Biseccion --sistema
./build/Biseccion --metodos
```

Un argumento que no sea ninguno de estos **no abre la ventana**: responde con el
error y muestra la ayuda. Escribir `--vrf` por `--verificar` no puede dejar al
usuario con una ventana abierta que ignora lo que escribió.

---

## 3. Arquitectura

```
Calculadora_metodos_numericos/
├── CMakeLists.txt
├── third_party/qcustomplot/              # QCustomPlot 2.1.1 (GPL-3.0, ver GPL.txt)
├── src/
│   ├── main.cpp                          # punto de entrada: GUI o modos de consola
│   ├── core/                             # C++ estándar puro, sin Qt
│   │   ├── Metodo.hpp/.cpp               # contrato MetodoNumerico + catálogo
│   │   ├── Resultado.hpp/.cpp            # modelo de datos común a todo método
│   │   ├── Biseccion.hpp/.cpp            # bisección
│   │   ├── NewtonRaphson.hpp/.cpp        # Newton-Raphson
│   │   ├── Jacobi.hpp/.cpp               # Jacobi para sistemas lineales
│   │   ├── CifrasSignificativas.hpp/.cpp # E_s = 0.5·10^(2−n) % → decimales
│   │   └── Funcion.hpp/.cpp              # parser de expresiones → std::function
│   ├── ejercicios/
│   │   └── Ejercicios.hpp/.cpp           # los cuatro ejercicios como fixture
│   └── gui/
│       ├── VentanaPrincipal.hpp/.cpp     # los cuatro paneles + el hilo de cálculo
│       ├── PanelEntrada.hpp/.cpp         # selector de método + sus campos
│       ├── ModeloIteraciones.hpp/.cpp    # modelo de tabla dirigido por descriptor
│       ├── TablaIteraciones.hpp/.cpp     # QTableView con selección por fila
│       ├── GraficoBiseccion.hpp/.cpp     # QCustomPlot: curva, intervalo, tangente
│       └── PanelProcedimiento.hpp/.cpp   # la narración paso a paso
└── tests/
    ├── parser_test.cpp
    ├── biseccion_test.cpp
    ├── newton_test.cpp
    ├── jacobi_test.cpp
    ├── metodo_test.cpp
    └── gui_test.cpp
```

### El contrato

Todo método implementa dos funciones:

```cpp
class MetodoNumerico {
public:
    virtual DescriptorMetodo descriptor() const = 0;
    virtual std::expected<Resultado, ErrorMetodo> resolver(
        const Entrada&, std::stop_token = {}) const = 0;
};
```

`DescriptorMetodo` declara el tipo de entrada (intervalo, punto inicial o
sistema), las columnas de la tabla, la etiqueta de la expresión auxiliar, el
nombre del punto que se aproxima (`m` o `x`) y si hay gráfico y recta asociada.
`Resultado` lleva las iteraciones, el motivo de parada y —en el caso de un
sistema— la matriz con la que se resolvió.

**Los errores son valores, no excepciones.** `resolver` devuelve
`std::expected<Resultado, ErrorMetodo>`, y `ErrorMetodo` tiene tres casos
(`SinCambioDeSigno`, `ExpresionInvalida`, `DimensionInvalida`): son los que impiden
**siquiera empezar** a calcular. Un cálculo que sí arranca pero no puede continuar
—una diagonal nula, una divergencia, una cancelación— no es un error sino un
`Resultado` con su `MotivoParada` puesto, para que la interfaz pueda mostrar las
iteraciones que sí se hicieron y explicar en cuál se paró. Así, ningún fallo se
reporta con una excepción atravesando la interfaz, y ninguno se pierde.

### El descriptor dirige la interfaz

`PanelEntrada` hace visibles unos campos u otros según el descriptor,
`ModeloIteraciones` construye los encabezados a partir de las columnas declaradas,
`GraficoBiseccion` decide qué resaltar y `PanelProcedimiento` narra los pasos del
método. Un método nuevo no requiere tocar ninguno de esos cuatro ficheros.

Hay un caso que rompe la regla general y que el diseño contempla: **una columna
que se expande**. La columna `VectorX` de Jacobi se declara **una sola vez** en el
descriptor, pero un sistema tiene tantas componentes como ecuaciones, así que
`ModeloIteraciones` la expande en `x₁ │ x₂ │ … │ x_n` según la dimensión real del
vector resuelto. El descriptor declara la familia; el modelo la despliega.

`--metodos` imprime el catálogo completo, que es la forma rápida de comprobar que
el descriptor de un método dice lo que debería:

```bash
./build/Biseccion --metodos
```

### Concurrencia

La resolución corre en un `std::jthread`, así que la ventana sigue respondiendo
mientras se calcula y el botón **Cancelar** puede interrumpir el cálculo en
cualquier iteración (`std::stop_token`). El resultado vuelve al hilo de la interfaz
con `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` y un `QPointer` de
guardia contra destrucción.

---

## 4. Los algoritmos

### 4.1 Bisección

```
m = (a + b) / 2
f(a)·f(m) < 0  →  b ← m        (la raíz está en [a, m])
f(a)·f(m) > 0  →  a ← m        (la raíz está en [m, b])
```

Se evalúa **un único producto** `fa·fm` por iteración y se decide con su signo, en
lugar de recalcular `f(a)·f(m)` y `f(a)·f(m) < 0` por separado: la aritmética es
la misma pero el código dice una sola vez lo que quiere decir.

**El intervalo tiene que contener un cambio de signo.** Si el usuario escribe un
intervalo que no encaja con la función, el paso 1 lo dice mostrando `f(a)` y `f(b)`,
que es exactamente lo que hay que mirar para saber cuál de los tres números
escritos es el que está mal. La condición de diagonal dominante es suficiente pero
no necesaria en Jacobi y ahí el aviso **no bloquea**; en bisección, en cambio, sin
cambio de signo el intervalo es directamente inválido y no hay tabla que calcular.

### 4.2 Newton-Raphson

```
x_{k+1} = x_k − f(x_k) / f'(x_k)
```

Convergencia cuadrática a cambio de dos cosas: hay que escribir `f'(x)` y hay que
elegir `x₀` bien. El método comprueba `f'(x_k) ≠ 0` en cada iteración y para con
`DerivadaNula` si se anula.

### 4.3 Cifras significativas

El número de decimales que se **muestra** depende de cuántas cifras significativas
se piden, según `E_s = 0.5·10^(2−n) %`. Con `n = 6` se muestran 6 cifras
significativas, no 6 decimales: un `0.25` sale como `0.250000` y un `1.4` como
`1.40000`.

El `double` completo no se pierde: se guarda en el *tooltip* de cada celda para
inspección, sin contaminar la vista.

### 4.4 Parser de expresiones

`parsearFuncion` acepta las expresiones que el usuario teclea, con multiplicación
implícita (`2x`), notación científica (`1.5e-3`), las constantes `π`, `e` y `τ`, y
**18 funciones**:

| Familia | Funciones |
|---|---|
| trigonométricas | `sin`, `cos`, `tan` |
| trigonométricas inversas | `asin`, `acos`, `atan` |
| hiperbólicas | `sinh`, `cosh`, `tanh` |
| exponencial y logaritmo | `exp`, `ln`, `log` (base 10, también `log10`) |
| raíces | `sqrt`, `cbrt` |
| signo y redondeo | `abs`, `sign` (también `sgn`), `floor`, `ceil` |

No están las recíprocas `sec`, `csc` y `cot`: se escriben `1/cos(x)`, `1/sin(x)` y
`1/tan(x)`, que es más claro de leer y no añade tres símbolos al lenguaje.

Los errores se reportan con **posición**: `no se reconoce «sqr» en el carácter 5`,
que es lo que permite corregir en vez de adivinar.

### 4.5 Jacobi

`Jacobi::resolver` sigue los cuatro pasos del enunciado, en ese orden.

#### Paso 1 · Verificar la convergencia (diagonal dominante)

Comprueba fila a fila si `A` es estrictamente diagonalmente dominante
(`|a_ii| > Σ_{j≠i} |a_ij|`), y devuelve los números que lo justifican para que la
narración pueda escribir `|6| > 5` en lugar de un sí/no global.

Si alguna fila no lo cumple, `reordenarParaDominancia` intercambia ecuaciones hasta
que todas lo hagan. La estrategia es de **mejor mejora**: entre todos los
intercambios posibles elige el que más filas dominantes deja, y para cuando ninguno
mejora el resultado. Una pasada útil aporta al menos una fila dominante nueva, así
que nunca hacen falta más de `n` pasadas. Terminar sin haberlo conseguido es un
resultado legítimo —puede que no exista ningún orden dominante— y en ese caso se
**informa y se sigue** con el que haya.

Dos detalles que son fáciles de invertir, y que `tests/jacobi_test.cpp` fija:

- **El término independiente viaja con su ecuación.** Mover solo la fila de
  coeficientes separaría la igualdad de su lado derecho y se resolvería otra cosa.
- **La iterada inicial NO se reordena.** La fila en la posición `i` siempre despeja
  la variable `i`, porque su diagonal es el coeficiente `a_ii` de esa misma variable.
  Intercambiar las ecuaciones 1 y 3 cambia *qué ecuación* ocupa cada sitio, pero el
  lugar `i` sigue resolviendo `x_i`. Como `x⁰` va indexado por variable, una
  iterada inicial `(0, 0, 1)` sigue siendo `(0, 0, 1)` después de reordenar: si
  también se moviera, se leería como «la primera ecuación empieza en 1», que no es
  lo que escribió el usuario.

El reordenamiento también **repara diagonales nulas**: `[[0,1],[1,1]]` no se puede
resolver tal como está escrito, pero intercambiando las ecuaciones la diagonal deja
de anularse y el sistema resuelve.

`Resultado` guarda las dos versiones del sistema (antes y después de reordenar) y
la lista de intercambios, para que el resumen pueda enseñar el antes y el después.
El panel de entrada además **anticipa** el paso 1 antes de resolver, diciendo qué
filas se van a intercambiar.

#### Paso 2 · Despejar una variable de cada ecuación

```
x_i^(k+1) = ( b_i − Σ_{j≠i} a_ij · x_j^(k) ) / a_ii
```

Que ninguna componente use el valor recién calculado por su vecina es exactamente
lo que distingue a Jacobi de Gauss-Seidel, y por eso dos iteraciones nunca se pisan.

#### Paso 3 · La tabla empieza en k = 0

La fila `k` registra el vector `x^(k)` y el error con el que se llegó a él. La
tabla **empieza en `k = 0` con la iterada inicial `x⁰`** que escribió el usuario, y
el primer error es el de `k = 1`. La fila `k = 0` no tiene error: no existe `x^(−1)`
con la que compararla.

#### Paso 4 · Los tres errores

| Error | Definición | Dónde vive |
|---|---|---|
| Absoluto | `\|x_i^(k) − x_i^(k−1)\|` | `Iteracion::errorAbsoluto`, uno por componente |
| Relativo | `\|x_i^(k) − x_i^(k−1)\| / \|x_i^(k)\|` | `Iteracion::errorRelativo`, uno por componente |
| P norma de orden p | `‖Δx‖_p = (Σ \|Δxᵢ\|^p)^(1/p)` | `Iteracion::norma` |

El criterio de parada es el error relativo porcentual sobre la norma del orden `p`:

```
e_a = ‖Δx‖_p / ‖x^(k)‖_p · 100      mientras e_a > E_s
```

**El orden `p` es un campo visible del panel de entrada**, con 3 por defecto, y al
lado la fórmula escrita con el `p` ya sustituido. No es un número repartido por el
código: el descriptor declara el título de la columna en genérico (`‖Δx‖p`) porque
no lo conoce, y la tabla lo reescribe con el valor que se usó. Con `p = 1` se
obtiene la suma de los valores absolutos y con `p = 2` el módulo euclídeo; `p ≥ 3`
reparte más el peso entre las componentes grandes y baja el error. El campo
acepta de 1 a 12, y el núcleo nunca recibe menos de 1: por debajo de 1 la expresión
`(Σ |Δxᵢ|^p)^(1/p)` deja de ser una norma.

#### La solución y su fórmula

`Jacobi::formulaCramer` aplica la regla de Cramer: `x = Dx/D`, `y = Dy/D`,
`z = Dz/D`. Los determinantes se calculan por **desarrollo de cofactores** hasta
`n = 8` —el mismo desarrollo que se hace a mano, así que con las matrices enteras
del ejercicio sale el determinante exacto (`129`, no `128.99999999999997`)— y por
eliminación gaussiana por encima, donde el factorial del desarrollo ya no es
usable.

Si `det(A)` es cero, la fórmula no se presenta porque no hay solución única. **Eso
no bloquea a Jacobi**, que puede converger en un sistema compatible singular; solo
deja de presentarse la referencia contra la que comparar.

La solución es un **vector**, y se lee con `Resultado::solucion()`
(`Resultado::raiz()` devuelve `NaN` para un sistema: devolver el `m = 0.0` que un
sistema deja en su último punto presentaría un cero como si fuera la respuesta). La
narración muestra además el residuo `‖b − A·x‖∞` junto a la solución: sin él, un
vector que «ha convergido» no demuestra que satisfaga el sistema.

#### `x`, `y`, `z` y no `a₁`, `a₂`, `a₃`

Hay dos formas de nombrar una variable, y cada una va en un sitio distinto:

| Forma | Se usa en | Por qué |
|---|---|---|
| `x, y, z…` (`Resultado::nombreVariable`) | cabeceras de la rejilla del sistema, ecuaciones de la narración, vector solución | las columnas **son** los coeficientes de cada variable; llamarlas `a₁, a₂, a₃` escondía justo lo que el ejercicio pide ver |
| `x₁, x₂…` (`Resultado::etiquetaVariable`) | subcolumnas de la tabla de iteraciones | con cuatro ecuaciones habría dos «x»; el subíndice distingue componentes de un vector |

La cabecera lateral de la rejilla repite el nombre de la variable que despeja cada
fila. Es lo que hace legible la última columna: un `x⁰ = (0, 0, 1)` solo significa
«x en 0, y en 0, z en 1» si cada fila dice a qué variable pertenece.

#### El sistema por defecto

Para 3 ecuaciones, la rejilla se precarga con el **sistema del enunciado**, escrito
en el orden en que aparece allí —con solo la fila 2 dominante, para que el paso 1
tenga algo que reordenar y se vea que funciona— y con `x⁰ = (0, 0, 1)`. Para
cualquier otra dimensión se pone uno estrictamente dominante (todos los
coeficientes valen `1` salvo la diagonal, que vale `n+1`; así cada fila cumple
`n+1 > n−1`), que resuelve en pocas iteraciones.

#### Cuándo para

| Situación | `MotivoParada` |
|---|---|
| `e_a < E_s` | `ToleranciaAlcanzada` |
| `a_ii = 0` y ningún intercambio lo arregla | `DiagonalNula` |
| `b − A·x` nulo en todas sus componentes | `SolucionExacta` |
| una componente deja de ser finita o se dispara | `Divergente` |
| se pulsa *Cancelar* | `Interrumpido` |
| se alcanza el límite de iteraciones sin cumplir el criterio | `MaxIteraciones` |

---

## 5. La interfaz

```
┌───────────────────┬──────────────────────────────────────────────┐
│ 1 · ENTRADA       │  2 · GRÁFICO INTERACTIVO  (QCustomPlot)      │
│   Método: ▾       │      f(x) + lo que resalte el método         │
│   f(x), [f'(x)],  ├──────────────────────────────────────────────┤
│   a,b  o  x₀, n   │  3 · TABLA DE ITERACIONES (QTableView)        │
│   Resolver/Cancel │   columnas según el método                    │
├───────────────────┤  (clic en fila ⇄ clic en marcador del gráfico)│
│ 4 · PROCEDIMIENTO │                                              │
└───────────────────┴──────────────────────────────────────────────┘
```

El desplegable superior selecciona el **método**, no el ejercicio. Al cambiarlo, el
panel reconfigura sus propios campos:

| Método | Campos que muestra | Columnas de la tabla | Gráfico |
|---|---|---|---|
| Bisección | `f(x)`, `a`, `b`, `n` | `k │ a │ b │ m │ f(m) │ e_a (%)` | sí · intervalo `[a, b]` |
| Newton-Raphson | `f(x)`, `f'(x)`, `x₀`, `n` | `k │ x │ f(x) │ f'(x) │ paso │ e_a (%)` | sí · tangente en `x_k` |
| Jacobi | dimensión `n`, rejilla `x │ y │ … │ b │ x⁰`, orden `p`, `n` cifras | `k │ x₁ │ x₂ │ … │ x_n │ ‖Δx‖ₚ │ e_a (%)` | **no** (oculto) |

Para Jacobi la fila de `f(x)` se oculta, y con ella las de `f'(x)`, `a`, `b` y `x₀`.
El editor del sistema va en su propia caja (`A·x = b`) para que se muestre y se
oculte **entera**: un sistema es una matriz, no una expresión, y no encaja en las
filas de un formulario construido alrededor de `f(x)`.

Cuando el descriptor declara que no hay gráfico, el gráfico se oculta y la tabla
queda como único panel derecho. Es lo que hace un sistema: donde no hay curva que
dibujar, un panel vacío con dos ejes ocupa media ventana y no enseña nada.

### Qué resalta el gráfico

`GraficoBiseccion` no sabe qué método se está ejecutando: recibe el descriptor y
decide. La clave es que **el intervalo y la tangente son la misma idea en distinto
método**:

| Método de intervalo | Método de punto inicial |
|---|---|
| zona sombreada `[a, b]` y guías verticales en `a` y `b` | — |
| tracer y etiqueta en `a`, en `b` y en `m` | tracer y etiqueta solo en `x` |
| — | **recta tangente** `y − f(x_k) = f'(x_k)·(x − x_k)` |

Que `a` y `b` valgan `0` en una iteración de Newton-Raphson es indistinguible de un
cero legítimo, así que el gráfico pregunta al descriptor en lugar de mirar el
valor; si no, registraría dos marcadores falsos en `(0, 0)`.

### Interactividad

- **Fila seleccionada en la tabla** → `TablaIteraciones::selectionChanged` emite
  `iteracionSeleccionada(k)`, y el gráfico y el panel procedimental se actualizan.
- **Clic en el gráfico** → busca el marcador más cercano (13 px) y selecciona la
  fila equivalente en la tabla. Enlace Tabla ⇄ Gráfico ⇄ Procedimiento.

### La narración paso a paso

El panel de procedimiento enseña la sustitución aritmética con los valores reales,
formateados a las `n` cifras significativas. Los pasos que narra son **del método**:
bisección muestra sus cinco pasos, Newton-Raphson los suyos, y Jacobi los **cuatro
pasos del enunciado** (§4.5). El descriptor decide cuál se narra.

`mostrarIteracion` recibe el `Resultado` entero y no solo la `Iteracion`: la
narrativa de un sistema necesita `A` y `b` para escribir el desarrollo, y con solo
la fila solo podría repetir el resultado.

### Por qué el editor de sistema necesita código explícito de layout

Dos hechos de Qt que se combinan y producen un fallo invisible al leer el código:

1. Un `QTableWidget` es un **área de scroll**, así que su `minimumSizeHint` es
   diminuto (solo la cabecera). Un `QLayout` que ande justo de espacio se lo queda
   entero y lo deja en una franja de unos pocos píxeles: las celdas existen, son
   editables y tienen el texto correcto, pero no hay superficie con la que hacer
   clic. Por eso la rejilla lleva un `minimumHeight` explícito. Ese mínimo se limita
   a **8 filas**: con 12 ecuaciones la rejilla entera mediría ~386 px y el
   `minimumHeight` se propagaría al `minimumSizeHint` del panel y de la ventana,
   que podría no caber en una pantalla normal. A partir de 8 filas usa scroll.
2. Un `QSplitter` **reparte los tamaños una sola vez**, al construirse. Como el
   panel de entrada se crea con el primer método del catálogo (bisección, sin
   rejilla), el reparto inicial no contemplaba la altura que hace falta cuando
   aparece el editor de sistema. `PanelEntrada` emite por eso
   `altoRequeridoCambiado()` y `VentanaPrincipal` vuelve a repartir en
   `ajustarAltoPanelEntrada()`.

El síntoma era «puedo elegir el método y el número de ecuaciones, pero no las
ecuaciones»: el selector de dimensión funcionaba y la rejilla no. La prueba
`tests/gui_test.cpp` lo cubre comprobando la altura de la rejilla y **tecleando un
sistema 2×2 completo** para comprobar que lo tecleado llega al resultado.

---

## 6. Ejercicios de referencia

Los cuatro ejercicios del enunciado son todos de una incógnita, así que valen para
bisección y para Newton-Raphson. Viven en `src/ejercicios/Ejercicios.cpp` como
*fixture* de datos que alimenta la verificación de consola y las pruebas
automáticas.

**Cómo se teclea cada uno en la interfaz.** Se elige el método en el desplegable, se
escriben los campos y se pulsa *Resolver*. No hay un desplegable de ejercicios: el
selector elige método.

### Bisección — necesita un intervalo con cambio de signo

| Ejercicio | Campo `f(x)` | `a` | `b` | `n` |
|---|---|---|---|---|
| 1 · Control de algoritmo (equilibrio) | `x - cos(x)` | 0 | 1 | 6 |
| 2 · Tráfico de red (saturación) | `ln(x) - x + 2` | 3 | 4 | 6 |
| 3 · Dispositivo IoT (cruce energético) | `exp(x) - 5*x` | 0 | 1 | 6 |
| 4 · Procesamiento de señales (resonancia) | `x*sin(x) - 1` | 0 | 2 | 6 |

El intervalo es lo único que hay que acertar: si `f(a)` y `f(b)` tienen el mismo
signo, bisección no puede converger y la aplicación lo dice.

### Newton-Raphson — necesita `f'(x)` y un buen `x₀`

| Ejercicio | Campo `f(x)` | Campo `f'(x)` | `x₀` | `n` |
|---|---|---|---|---|
| 1 · Equilibrio | `x - cos(x)` | `1 + sin(x)` | 0.5 | 6 |
| 2 · Tráfico de red | `ln(x) - x + 2` | `1/x - 1` | 3.5 | 6 |
| 3 · Dispositivo IoT | `exp(x) - 5*x` | `exp(x) - 5` | 0.5 | 6 |
| 4 · Resonancia | `x*sin(x) - 1` | `sin(x) + x*cos(x)` | 1 | 6 |

`x₀` es el punto medio del intervalo en los cuatro casos, que es lo que usa
`--comparar`. Newton es mucho más rápido (5, 5, 5 y 4 iteraciones frente a 22, 20,
23 y 22 de bisección) pero necesita las dos cosas que bisección no pide.

### Jacobi — el sistema del enunciado

Los cuatro ejercicios anteriores son de una incógnita, así que Jacobi necesita un
sistema propio. El que viene precargado para 3 ecuaciones es el del enunciado,
escrito en el orden en que aparece allí (con solo la fila 2 dominante, para que el
paso 1 tenga algo que reordenar):

```
   x +  y + 4z = 15
 −2x + 4y +  z =  9
  6x + 3y − 2z =  6
```

con `x⁰ = (0, 0, 1)` y `p = 3`. La solución exacta es `(1, 2, 3)`, por la regla de
Cramer con `D = 129`, `Dx = 129`, `Dy = 258`, `Dz = 387`. Los pasos 1 a 4, el
intercambio de las filas 1 y 3 y la tabla completa se imprimen con:

```bash
./build/Biseccion --sistema
```

### Verificación numérica

Los valores de referencia que fija `tests/` se calcularon **por fuera** (Python,
doble precisión, sin compartir código con la implementación) para que las pruebas
comparen dos caminos distintos y no uno consigo mismo.

```bash
./build/Biseccion --verificar    # raíces, f(raíz) e iteraciones por criterio
./build/Biseccion --comparar     # bisección frente a Newton en las mismas raíces
./build/Biseccion --tablas       # las tablas completas de iteraciones
```

`--tablas` imprime las cuatro tablas enteras para `n = 4, 5, 6`; no están en este
README porque se regeneran con un comando y pueden desincronizarse si se copian a
mano.

> **Observación didáctica.** El criterio relativo (`e_a < E_s`) exige **más**
> iteraciones que el absoluto cuando la raíz es menor que 1 (los ejercicios 1 y 3),
> porque el factor `|m|` del denominador reduce el peso del error, y **menos**
> cuando la raíz supera 1 (el ejercicio 2). Por eso ambos criterios coexisten: el
> absoluto garantiza las cifras y el relativo controla el error porcentual.

---

## 7. Añadir un método nuevo

La aplicación es un módulo de una calculadora de métodos numéricos. Como la
interfaz está dirigida por el descriptor (§3), la tabla, el gráfico y el panel
procedimental se reutilizan tal cual: **lo único que cambia es el núcleo y su
descriptor**.

1. **`src/core/<Metodo>.{hpp,cpp}`** — heredar de `MetodoNumerico` e implementar
   `descriptor()` y `resolver()`. `resolver` devuelve
   `std::expected<Resultado, ErrorMetodo>` y recibe un `std::stop_token` para poder
   cancelarse.
2. **Registrarlo** en `catalogoMetodos()` (`src/core/Metodo.cpp`).
3. **`src/main.cpp`** — añadir su modo de consola si hace falta, y una entrada en
   `ayudaEnConsola()`.
4. **`tests/<metodo>_test.cpp`** — registrado con `registrar_prueba(...)`, sin Qt.
   Si el método necesita campos propios en la interfaz (como la matriz de Jacobi),
   se amplía `tests/gui_test.cpp`.
5. **`src/gui/PanelProcedimiento.cpp`** — un caso más en la narración, y una sección
   en este README.

Piezas que ya están puestas y que un método nuevo puede reutilizar:

| Pieza | Para qué |
|---|---|
| `DescriptorMetodo::requiereExpresion` | `false` para métodos cuya entrada no es `f(x)`: la interfaz se salta el parseo |
| `DescriptorMetodo::requiereExpresionAuxiliar` | `true` para los que piden `f'(x)` en un campo aparte |
| `CampoIteracion::VectorX` | una columna *de familia*: el modelo la expande en `x₁ │ x₂ │ …` según la dimensión real |
| `Entrada::normaP` | un parámetro del método que llega hasta la etiqueta de una columna |
| `Jacobi::dominanciaPorFila` + `Reordenamiento` + `matrizOriginal` | narrar una comprobación y **su** corrección: guardar el «antes» y el «después» |
| `FormulaSolucion` | una segunda referencia, calculada de forma independiente, para contrastar |
| `MotivoParada` frente a `ErrorMetodo` | distinguir «no se pudo **empezar**» de «se paró en la iteración 12»: el primero es un error, el segundo un resultado que se puede enseñar |

| Estado | Método | Notas |
|---|---|---|
| ✅ Implementado | Bisección | 5 pasos, `E_s = 0.5·10^(2−n) %`, cancelable |
| ✅ Implementado | Newton-Raphson | `f'(x)` escrita por el usuario; tangente en el gráfico; paradas `DerivadaNula` y `Divergente` |
| ✅ Implementado | Jacobi (sistemas) | reordenamiento de filas hasta la diagonal dominante (informa, no bloquea), `p` configurable, fórmula de Cramer, sin gráfico |
| 📌 Siguiente | Gauss-Seidel (sistemas) | reutiliza la rejilla; converge en menos iteraciones, pero pierde la simetría: usa `xᵢ⁽ᵏ⁺¹⁾` en la fila `i` |
| 📌 Siguiente | Falsa posición | reutiliza `core/`; converge más rápido que la bisección |
| 📌 Siguiente | Secante | dos puntos de arranque; evita la derivada |
| 💡 Planeado | Punto fijo (`x = g(x)`) | criterio de convergencia `\|g(x) − x\| < E_s` |
| 💡 Planeado | Raíces múltiples (deflación) | reutiliza la bisección sobre cada factor |
| 💡 Planeado | Integración numérica | trapecio, Simpson y Simpson compuesto |
| 💡 Planeado | Interpolación | Lagrange, diferencias divididas y Newton |

---

## 8. Pruebas

Seis binarios de prueba registrados con CTest:

```bash
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

| Prueba | Qué cubre |
|---|---|
| `parser_tests` | sintaxis y evaluación del parser, incluidos los errores con posición |
| `biseccion_tests` | raíces, iteraciones y cifras E1–E4 |
| `metodo_tests` | catálogo, descriptores y el contrato común |
| `newton_tests` | raíces y guardas (`DerivadaNula`, divergencia) |
| `jacobi_tests` | convergencia, dominancia, reordenamiento, fórmula de solución y forma de la entrada |
| `gui_tests` | la interfaz bajo `QT_QPA_PLATFORM=offscreen`: columnas por método, gráfico visible u oculto, y que lo tecleado llega al resultado |

Las seis pruebas tardan **menos de un segundo** en total. Para el trabajo
diario, `-E gui_tests` va aún más al grano:

```bash
ctest --test-dir build -E gui_tests     # solo el núcleo
ctest --test-dir build -R gui_tests     # solo la interfaz
```

Los cinco binarios del núcleo **no enlazan Qt**: se compilan con `AUTOMOC OFF` y
sin ninguna biblioteca de Qt (`ldd build/jacobi_tests` no muestra ni una), porque
`src/core/` es C++ estándar puro. Solo `gui_tests` —y la aplicación— dependen de
él. `BUILD_GUI_TESTS=OFF` evita compilar `gui_tests`, pero **no** quita Qt del
proyecto: la aplicación sigue necesitando Widgets y PrintSupport.

`gui_tests` recorre los trece pasos de la interfaz uno tras otro, y cada paso
**espera a que termine el cálculo anterior** —el botón *Resolver* se rehabilita
cuando el resultado vuelve— en vez de esperar un número fijo de milisegundos. Con
plazos fijos la prueba era intermitente: si una resolución tardaba más de lo
previsto, el paso siguiente comprobaba una tabla que aún no estaba rellena. Por
eso baja de 12 s a 0,6 s y ya no depende de la velocidad de la máquina.

### Autotest con capturas

```bash
QT_QPA_PLATFORM=offscreen BISECCION_AUTOTEST=1 ./build/Biseccion
```

Recorre los tres métodos sin interacción y guarda una captura de cada uno, e
imprime el directorio por el que las dejó (el directorio temporal del sistema,
`$TMPDIR/metodos_numericos` o `C:/Users/<usuario>/AppData/Local/Temp/metodos_numericos`
en Windows). Es la forma rápida de ver la ventana sin tenerla delante.

---

## 9. Licencias

- **Código de la aplicación**: proyecto académico original (ISUM, 8.º semestre).
- **QCustomPlot 2.1.1** © Emanuel Eichhammer, distribuido bajo **GPL-3.0**
  (`third_party/qcustomplot/GPL.txt`). No se ha modificado.
