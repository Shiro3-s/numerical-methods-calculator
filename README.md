# Métodos Numéricos — Aplicación Interactiva (C++ / Qt 6)

Aplicación gráfica de escritorio para **métodos numéricos** de la carrera de
Ingeniería, con entrada de funciones, tabla de iteraciones, gráfico interactivo
(inspirado en GeoGebra) y vista procedimental paso a paso. La presentación de
decimales se limita **dinámicamente** según las cifras significativas calculadas.

> **Estado actual: método 1 de N.** Hoy la aplicación cubre por completo el
> **método de bisección**, incluida la solución matemática de los cuatro
> ejercicios aplicados del enunciado. El diseño ya está preparado para alojar
> **más métodos numéricos** (ver [§ 7 · Hoja de ruta](#7-hoja-de-ruta-ampliacion-a-otros-metodos)):
> el núcleo vive en `src/core/`, no depende de Qt y devuelve resultados con
> errores tipados, de modo que cada método se añade como un módulo nuevo sin
> tocar la interfaz.

---

## 1. Arquitectura de la Aplicación

```
biseccion/
├── CMakeLists.txt                        # build con Qt6 + QCustomPlot + tests
├── third_party/qcustomplot/              # QCustomPlot 2.1.1 (GPL-3.0, ver GPL.txt)
├── tests/                                # pruebas automáticas (CTest, sin Qt)
│   ├── parser_test.cpp                   # sintaxis y evaluación del parser
│   └── biseccion_test.cpp                # raíces, iteraciones y cifras E1–E4
└── src/
    ├── main.cpp                          # QApplication + modos de consola
    ├── core/                             # lógica pura (sin dependencia de Qt)
    │   ├── Biseccion.hpp/.cpp            # algoritmo de los 5 pasos
    │   ├── CifrasSignificativas.hpp/.cpp # E_s = 0.5·10^(2−n) % → decimales
    │   └── Funcion.hpp/.cpp              # parser de expresiones → std::function
    ├── ejercicios/
    │   └── Ejercicios.hpp/.cpp           # presets E1–E4
    └── gui/
        ├── VentanaPrincipal.hpp/.cpp     # integración de los 4 paneles + hilo
        ├── PanelEntrada.hpp/.cpp         # Panel 1 · entrada y presets
        ├── GraficoBiseccion.hpp/.cpp     # Panel 2 · gráfico interactivo
        ├── TablaIteraciones.hpp/.cpp     # Panel 3 · tabla clickeable
        └── PanelProcedimiento.hpp/.cpp   # Panel 4 · detalle paso a paso
```

**Separación de capas.** El núcleo (`core/`) es 100 % C++ estándar y se puede
probar en consola (`--verificar` / `--tablas`); la capa de presentación
(`gui/`) consume los mismos tipos (`Iteracion`, `Resultado`) sin duplicar lógica.

**Preparado para más métodos.** `core/` no depende de Qt y cada algoritmo expone
una clase propia (`Biseccion`) con la misma firma: `resolver(...) → std::expected<Resultado, Error>`
y `std::stop_token` para poder cancelar. La GUI (tabla, gráfico, panel
procedimental) trabaja contra esos tipos genéricos, de modo que sumar un método
nuevo consiste en añadir su módulo en `core/`, su diálogo de entrada y conectarlo
al `ModeloIteraciones`. Ver [§ 7 · Hoja de ruta](#7-hoja-de-ruta-ampliacion-a-otros-metodos).

### Flujo de datos

1. El usuario escribe `f(x)`, extremos `a, b` y cifras significativas `n`
   (o elige un preset E1–E4).
2. `parsearFuncion` valida la expresión (errores tipados en español).
3. `Biseccion::resolver` ejecuta los 5 pasos en un `std::jthread` (cancelable
   con `stop_token`) y devuelve `std::expected<Resultado, ErrorBiseccion>`.
4. El resultado se distribuye a la tabla (modelo), al gráfico y al resumen.
5. Al seleccionar una fila de la tabla se **resalta el intervalo** `[a, b]` y
   los puntos `a, b, m` en el gráfico, y el panel derecho muestra la
   sustitución aritmética de esa iteración (pasos 1 a 4).
6. Hacer clic sobre un marcador del gráfico selecciona la fila equivalente
   (enlace Tabla ⇄ Gráfico, concepto GeoGebra).

---

## 2. Dependencias y Configuración (CMake / Bibliotecas)

### Requisitos

| Dependencia | Versión usada | Función |
|---|---|---|
| Qt 6 (Widgets + PrintSupport) | 6.11.2 | GUI y eventos |
| QCustomPlot | 2.1.1 (vendoreado) | trazado interactivo |
| CMake | ≥ 3.21 (usado 4.4) | construcción |
| Compilador C++ | GCC 16 (requiere C++23) | código |

QCustomPlot no está empaquetado en la mayoría de distros; por eso se **vendeora**
en `third_party/qcustomplot/` (archivos `qcustomplot.h`, `qcustomplot.cpp`
descargados de `https://www.qcustomplot.com/release/2.1.1/QCustomPlot-source.tar.gz`,
licencia **GPL-3.0**, incluida en `GPL.txt`).

### Compilación y ejecución

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/Biseccion                     # interfaz gráfica
./build/Biseccion --verificar         # verificación matemática (E1–E4)
./build/Biseccion --tablas            # tablas completas de iteraciones
ctest --test-dir build --output-on-failure   # pruebas automáticas del parser y núcleo
```

Opciones de configuración:

```bash
cmake -S . -B build-debug -DENABLE_SANITIZERS=ON    # ASan + UBSan
cmake -S . -B build -DSTRICT_WARNINGS=OFF           # relaja -Werror
```

Prueba automatizada de la interfaz (resuelve E1 y cierra):

```bash
QT_QPA_PLATFORM=offscreen BISECCION_AUTOTEST=1 ./build/Biseccion
```

### Endurecimiento aplicado (skill modern-cpp)

- Estándar **C++23** (`CMAKE_CXX_STANDARD 23`).
- `-Wall -Wextra -Wpedantic -Werror` (advertencias = errores).
- `-D_FORTIFY_SOURCE=3`, `-fstack-protector-strong`, `-fstack-clash-protection`,
  `-fPIC`, `-Wl,-z,relro,-z,now`.
- `-DENABLE_SANITIZERS=ON` activa ASan + UBSan en depuración.
- La biblioteca heredada QCustomPlot se compila con advertencias silenciadas
  (`-w`) para no contaminar los diagnósticos del código propio.

---

## 3. Implementación Lógica: Algoritmo de Bisección y Cifras Significativas (C++)

### 3.1 Los 5 pasos obligatorios

`Biseccion::resolver` implementa literalmente la estructura exigida:

| Paso | Definición | Implementación |
|---|---|---|
| 1 | Verificación inicial: si `f(a)·f(b) < 0` existe raíz en `[a, b]` | si `fa*fb >= 0` → `std::unexpected(ErrorBiseccion::SinCambioDeSigno)` |
| 2 | Punto medio `m = (a + b) / 2` | cálculo directo |
| 3 | Signo de `f(a)·f(m)` | un solo producto `fa*fm`; ramas `< 0`, `> 0`, `== 0` (raíz exacta) sin redundancias |
| 4 | Parada `e_a = \|(m_actual − m_anterior)/m_actual\|·100`, parar si `e_a < E_s` | `e_a` es `std::optional<double>`: **no existe en la iteración 1** |
| 5 | Cifras significativas y presentación | módulo `CifrasSignificativas` |

```cpp
// Fragmento esencial de Biseccion.cpp (pasos 2–4)
const double m = (a + b) / 2.0;
const double fm = f_(m);

if (k > 1) {                                  // paso 4
    const double ea = std::fabs((m - mAnterior) / m) * 100.0;
    iteracion.eaPorcentaje = ea;
    if (ea < toleranciaEsPorcentaje) resultado.motivo = MotivoParada::ToleranciaAlcanzada;
}
const double producto = fa * fm;              // paso 3 (una sola evaluación)
if (producto == 0.0) /* raíz exacta en m */;
if (producto < 0.0) b = m;                    // mitad inferior
else                { a = m; fa = fm; }       // mitad superior
```

### 3.2 Pre-cálculo de iteraciones (dos criterios)

- **Criterio absoluto** (garantizar *n* cifras exactas):
  `k ≥ ceil(log₂( (b−a) / (0.5·10⁻ⁿ) ))`
  → `Biseccion::iteracionesParaCifras(a, b, n)`.
- **Criterio relativo** (error relativo porcentual bajo el umbral):
  `e_a < E_s` con `E_s = 0.5·10^(2−n) %`.
  → verificado por simulación en `--verificar`.

Ambos criterios se muestran en el panel de entrada (estimación) y en la
verificación de consola.

### 3.3 Cifras significativas → decimales dinámicos

De la fórmula del paso 5 se despeja `n = 2 − log₁₀(E_s / 0.5)`. Para presentar
cada valor con `n` cifras significativas sin basura decimal:

```
decimales = n − 1 − floor(log₁₀ |valor|)   (recortado a ≥ 0; el 0 usa n − 1)
```

El cálculo interno **siempre** usa `double`; el recorte es exclusivo de la capa
de presentación (tabla, gráfico, procedimiento).

### 3.4 Parser de expresiones

`parsearFuncion` implementa un analizador descendente recursivo
(`expresión → término → unario → potencia → primario`) con la gramática:

```
expresion := termino  (('+' | '-') termino)*
termino   := unario   (('*' | '/' | <implícito>) unario)*
unario    := '-' unario | potencia
potencia  := primario ('^' unario)?        (asociativa a la derecha)
primario  := numero | identificador | '(' expresion ')'
```

Soporta **multiplicación implícita** (se asume `*` cuando un término termina y
el siguiente símbolo inicia un factor): `5x`, `2sin(x)`, `x(x-1)`,
`(x-1)(x+2)`, `2π`. La potencia `^` tiene mayor precedencia que el producto
implícito (`3x^2 = 3·(x²)`), y `^` es asociativa a la derecha (`x^2^3 = x^(2³)`).

Funciones disponibles: `sin cos tan asin acos atan sinh cosh tanh ln log log10
log2 exp sqrt cbrt abs floor ceil sign/sgn`. Detalles:

- `log` y `log10` son **base 10**; `ln` es natural.
- Constantes: `pi`/`π`, `e`, `tau`/`τ`.
- Números con notación científica (`1e-3`), pero con **mirilla**: si no hay
  dígitos tras la `e`/`E`, no es exponente — `2e3 = 2000` mientras que
  `2ex = 2·e·x` (constante `e` por variable `x`).
- Acepta espacios donde sea razonable (`x sin(x)`, `pi / 2`).

El AST usa `std::variant` y devuelve `std::expected<Funcion, ErrorParser>` con
mensajes de error en español y posición («función o símbolo desconocido 'foo'»,
«falta el paréntesis de cierre ')'»…). Las pruebas de esta capa están en
`tests/parser_test.cpp` (~40 casos) y se ejecutan con `ctest`.

> **Intervalo al digitar a mano.** Los presets E1–E4 fijan `[a, b]` según el
> enunciado, pero al **editar** `f(x)` manualmente el intervalo no se recalcula
> solo. Si el usuario escribe una función distinta, el panel de entrada muestra
> el aviso «⚠ f(x) editada a mano: revise que el intervalo [a, b] siga
> conteniendo la raíz»; el intervalo editable siempre se valida en el paso 1
> (si `f(a)·f(b) ≥ 0` se informa «SinCambioDeSigno»).

---

## 4. Implementación de Interfaz Gráfica (C++)

La ventana principal organiza los **cuatro paneles** con `QSplitter`:

```
┌───────────────────┬──────────────────────────────────────────────┐
│ 1 · ENTRADA       │  2 · GRÁFICO INTERACTIVO  (QCustomPlot)      │
│   f(x), a, b, n   │      f(x), intervalo [a,b], puntos a·b·m     │
│   presets E1–E4   ├──────────────────────────────────────────────┤
│   Resolver/Cancel │  3 · TABLA DE ITERACIONES (QTableView)        │
├───────────────────┤        k | a | b | m | f(m) | e_a (%)         │
│ 4 · PROCEDIMIENTO │  (clic en fila ⇄ clic en marcador del gráfico)│
└───────────────────┴──────────────────────────────────────────────┘
```

### Interactividad (enlace Tabla ⇄ Gráfico ⇄ Procedimiento)

- **Fila seleccionada en la tabla** → `TablaIteraciones::selectionChanged`
  emite `iteracionSeleccionada(k)`; `VentanaPrincipal` actualiza el gráfico
  (`GraficoBiseccion::resaltarIteracion`) y el panel procedimental
  (`PanelProcedimiento::mostrarIteracion`).
- **Clic en el gráfico** → busca el marcador más cercano (13 px) y emite
  `marcadorClickeado(k)`, que selecciona la fila equivalente en la tabla.
- **Gráfico**: sombreado translúcido del intervalo actual, guías verticales en
  `a` y `b`, tracers sobre la curva para `a`, `b`, `m` y etiquetas; soporta
  arrastre y zoom (los marcadores se re-posicionan al cambiar el rango).
- **Vista procedimental**: sustitución aritmética de los pasos 1 a 4
  (`f(a)·f(b)`, `m = (a+b)/2`, criterio de signo, `e_a`) con los valores reales
  formateados a las `n` cifras significativas.

### Concurrencia

La resolución corre en un `std::jthread`; el `std::stop_token` del sistema
permite **Cancelar** en cualquier iteración. El resultado vuelve al hilo de la
interfaz mediante `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`, con
un `QPointer` de guardia contra destrucción.

### Formato dinámico en la UI

Cada celda de la tabla, etiqueta del gráfico y dato del procedimiento se
formatea con `formatearParaCifras(valor, n)`. El tooltip de cada celda guarda la
precisión completa del `double` para inspección, sin contaminar la vista.

---

## 5. Solución Matemática de los Ejercicios

Resumen de raíces obtenidas (la tabla completa se muestra dentro de la
aplicación y se genera con `./build/Biseccion --tablas`):

| Ejercicio | f(x) | [a, b] | Raíz m (double) | f(m) |
|---|---|---|---|---|
| 5.1 · Equilibrio | x − cos(x) | [0, 1] | **0.739085133215159** | 2.5·10⁻¹⁵ |
| 5.2 · Tráfico de red | ln(x) − x + 2 | [3, 4] | **3.146193220620589** | 4.4·10⁻¹⁵ |
| 5.3 · IoT energético | eˣ − 5x | [0, 1] | **0.259171101819073** | 1.1·10⁻¹⁵ |
| 5.4 · Resonancia | x·sen(x) − 1 | [0, 2] | **1.114157140871928** | 3.0·10⁻¹⁵ |

Iteraciones necesarias según el criterio (absoluto vs. relativo `e_a < E_s`):

| n cifras | E1 abs/rel | E2 abs/rel | E3 abs/rel | E4 abs/rel |
|---|---|---|---|---|
| 4 | 15 / 15 | 15 / 13 | 15 / 17 | 16 / 16 |
| 5 | 18 / 19 | 18 / 16 | 18 / 20 | 19 / 19 |
| 6 | 21 / 22 | 21 / 20 | 21 / 23 | 22 / 22 |

> **Observación didáctica.** El criterio relativo exige **más** iteraciones que
> el absoluto cuando la raíz es menor que 1 (E1 y E3) porque el factor
> `|m_actual|` del denominador reduce el peso del error, y **menos** cuando la
> raíz supera 1 (E2). Por eso ambos criterios coexisten: el absoluto garantiza
> cifras correctas y el relativo controla el error porcentual estimado.

### 5.1 Ejercicio 1 (Punto de equilibrio)

`f(x) = x − cos(x)`, intervalo `[0, 1]`.

Verificación: `f(0) = −1 < 0`, `f(1) = 0.45970 > 0` → `f(0)·f(1) < 0`, existe
raíz. La raíz (el punto fijo de `cos`) es `m ≈ 0.739085133215159`.

Tabla de iteraciones (con `n = 6` cifras significativas):

| k  | a          | b          | m          | f(m)        | e_a (%)   |
|----|------------|------------|------------|-------------|-----------|
| 1  | 0.00000000 | 1.00000000 | 0.50000000 | −0.37758256 | —         |
| 2  | 0.50000000 | 1.00000000 | 0.75000000 | 0.01831113  | 33.333333 |
| 3  | 0.50000000 | 0.75000000 | 0.62500000 | −0.18596312 | 20.000000 |
| 4  | 0.62500000 | 0.75000000 | 0.68750000 | −0.08533495 | 9.090909  |
| 5  | 0.68750000 | 0.75000000 | 0.71875000 | −0.03387937 | 4.347826  |
| 6  | 0.71875000 | 0.75000000 | 0.73437500 | −0.00787473 | 2.127660  |
| 7  | 0.73437500 | 0.75000000 | 0.74218750 | 0.00519571  | 1.052632  |
| 8  | 0.73437500 | 0.74218750 | 0.73828125 | −0.00134515 | 0.529101  |
| 9  | 0.73828125 | 0.74218750 | 0.74023438 | 0.00192387  | 0.263852  |
| 10 | 0.73828125 | 0.74023438 | 0.73925781 | 0.00028901  | 0.132100  |
| 11 | 0.73828125 | 0.73925781 | 0.73876953 | −0.00052816 | 0.066094  |
| 12 | 0.73876953 | 0.73925781 | 0.73901367 | −0.00011960 | 0.033036  |
| 13 | 0.73901367 | 0.73925781 | 0.73913574 | 0.00008470  | 0.016515  |
| 14 | 0.73901367 | 0.73913574 | 0.73907471 | −0.00001745 | 0.008258  |
| 15 | 0.73907471 | 0.73913574 | 0.73910522 | 0.00003363  | 0.004129  |
| 16 | 0.73907471 | 0.73910522 | 0.73908997 | 0.00000809  | 0.002065  |
| 17 | 0.73907471 | 0.73908997 | 0.73908234 | −0.00000468 | 0.001032  |
| 18 | 0.73908234 | 0.73908997 | 0.73908615 | 0.00000170  | 0.000516  |
| 19 | 0.73908234 | 0.73908615 | 0.73908424 | −0.00000149 | 0.000258  |
| 20 | 0.73908424 | 0.73908615 | 0.73908520 | 0.00000011  | 0.000129  |
| 21 | 0.73908424 | 0.73908520 | 0.73908472 | −0.00000069 | 0.000065  |
| 22 | 0.73908472 | 0.73908520 | 0.73908496 | −0.00000029 | 0.000032  |

### 5.2 Ejercicio 2 (Tráfico de red)

`f(x) = ln(x) − x + 2`, intervalo `[3, 4]`.

Verificación: `f(3) = 0.09861 > 0`, `f(4) = −0.61371 < 0` → existe raíz.
Raíz: `m ≈ 3.146193220620589`.

Tabla de iteraciones (con `n = 6` cifras significativas):

| k  | a          | b          | m          | f(m)        | e_a (%)   |
|----|------------|------------|------------|-------------|-----------|
| 1  | 3.00000000 | 4.00000000 | 3.50000000 | −0.24723703 | —         |
| 2  | 3.00000000 | 3.50000000 | 3.25000000 | −0.07134500 | 7.692308  |
| 3  | 3.00000000 | 3.25000000 | 3.12500000 | 0.01443428  | 4.000000  |
| 4  | 3.12500000 | 3.25000000 | 3.18750000 | −0.02826309 | 1.960784  |
| 5  | 3.12500000 | 3.18750000 | 3.15625000 | −0.00686539 | 0.990099  |
| 6  | 3.12500000 | 3.15625000 | 3.14062500 | 0.00379682  | 0.497512  |
| 7  | 3.14062500 | 3.15625000 | 3.14843750 | −0.00153120 | 0.248139  |
| 8  | 3.14062500 | 3.14843750 | 3.14453125 | 0.00113358  | 0.124224  |
| 9  | 3.14453125 | 3.14843750 | 3.14648438 | −0.00019862 | 0.062073  |
| 10 | 3.14453125 | 3.14648438 | 3.14550781 | 0.00046753  | 0.031046  |
| 11 | 3.14550781 | 3.14648438 | 3.14599609 | 0.00013447  | 0.015521  |
| 12 | 3.14599609 | 3.14648438 | 3.14624023 | −0.00003207 | 0.007760  |
| 13 | 3.14599609 | 3.14624023 | 3.14611816 | 0.00005120  | 0.003880  |
| 14 | 3.14611816 | 3.14624023 | 3.14617920 | 0.00000956  | 0.001940  |
| 15 | 3.14617920 | 3.14624023 | 3.14620972 | −0.00001125 | 0.000970  |
| 16 | 3.14617920 | 3.14620972 | 3.14619446 | −0.00000084 | 0.000485  |
| 17 | 3.14617920 | 3.14619446 | 3.14618683 | 0.00000436  | 0.000242  |
| 18 | 3.14618683 | 3.14619446 | 3.14619064 | 0.00000176  | 0.000121  |
| 19 | 3.14619064 | 3.14619446 | 3.14619255 | 0.00000046  | 0.000061  |
| 20 | 3.14619255 | 3.14619446 | 3.14619350 | −0.00000019 | 0.000030  |

### 5.3 Ejercicio 3 (Dispositivo IoT)

`f(x) = eˣ − 5x`, intervalo `[0, 1]`.

Verificación: `f(0) = 1 > 0`, `f(1) = −2.28172 < 0` → existe raíz.
La solución de `eˣ = 5x` es la rama de la W de Lambert: `m ≈ 0.259171101819073`.

Tabla de iteraciones (con `n = 6` cifras significativas):

| k  | a          | b          | m          | f(m)        | e_a (%)   |
|----|------------|------------|------------|-------------|-----------|
| 1  | 0.00000000 | 1.00000000 | 0.50000000 | −0.85127873 | —         |
| 2  | 0.00000000 | 0.50000000 | 0.25000000 | 0.03402542  | 100.000000|
| 3  | 0.25000000 | 0.50000000 | 0.37500000 | −0.42000859 | 33.333333 |
| 4  | 0.25000000 | 0.37500000 | 0.31250000 | −0.19566206 | 20.000000 |
| 5  | 0.25000000 | 0.31250000 | 0.28125000 | −0.08146524 | 11.111111 |
| 6  | 0.25000000 | 0.28125000 | 0.26562500 | −0.02387913 | 5.882353  |
| 7  | 0.25000000 | 0.26562500 | 0.25781250 | 0.00503365  | 3.030303  |
| 8  | 0.25781250 | 0.26562500 | 0.26171875 | −0.00943265 | 1.492537  |
| 9  | 0.25781250 | 0.26171875 | 0.25976562 | −0.00220197 | 0.751880  |
| 10 | 0.25781250 | 0.25976562 | 0.25878906 | 0.00141522  | 0.377358  |
| 11 | 0.25878906 | 0.25976562 | 0.25927734 | −0.00039353 | 0.188324  |
| 12 | 0.25878906 | 0.25927734 | 0.25903320 | 0.00051081  | 0.094251  |
| 13 | 0.25903320 | 0.25927734 | 0.25915527 | 0.00005863  | 0.047103  |
| 14 | 0.25915527 | 0.25927734 | 0.25921631 | −0.00016745 | 0.023546  |
| 15 | 0.25915527 | 0.25921631 | 0.25918579 | −0.00005441 | 0.011774  |
| 16 | 0.25915527 | 0.25918579 | 0.25917053 | 0.00000211  | 0.005888  |
| 17 | 0.25917053 | 0.25918579 | 0.25917816 | −0.00002615 | 0.002944  |
| 18 | 0.25917053 | 0.25917816 | 0.25917435 | −0.00001202 | 0.001472  |
| 19 | 0.25917053 | 0.25917435 | 0.25917244 | −0.00000496 | 0.000736  |
| 20 | 0.25917053 | 0.25917244 | 0.25917149 | −0.00000142 | 0.000368  |
| 21 | 0.25917053 | 0.25917149 | 0.25917101 | 0.00000034  | 0.000184  |
| 22 | 0.25917101 | 0.25917149 | 0.25917125 | −0.00000054 | 0.000092  |
| 23 | 0.25917101 | 0.25917125 | 0.25917113 | −0.00000010 | 0.000046  |

### 5.4 Ejercicio 4 (Frecuencia de resonancia)

`f(x) = x·sen(x) − 1`, intervalo `[0, 2]`.

Verificación: `f(0) = −1 < 0`, `f(2) = 0.81859 > 0` → existe raíz.
La solución de `x·sen(x) = 1` es `m ≈ 1.114157140871928`.

Tabla de iteraciones (con `n = 6` cifras significativas):

| k  | a          | b          | m          | f(m)        | e_a (%)   |
|----|------------|------------|------------|-------------|-----------|
| 1  | 0.00000000 | 2.00000000 | 1.00000000 | −0.15852902 | —         |
| 2  | 1.00000000 | 2.00000000 | 1.50000000 | 0.49624248  | 33.333333 |
| 3  | 1.00000000 | 1.50000000 | 1.25000000 | 0.18623077  | 20.000000 |
| 4  | 1.00000000 | 1.25000000 | 1.12500000 | 0.01505104  | 11.111111 |
| 5  | 1.00000000 | 1.12500000 | 1.06250000 | −0.07182663 | 5.882353  |
| 6  | 1.06250000 | 1.12500000 | 1.09375000 | −0.02836172 | 2.857143  |
| 7  | 1.09375000 | 1.12500000 | 1.10937500 | −0.00664277 | 1.408451  |
| 8  | 1.10937500 | 1.12500000 | 1.11718750 | 0.00420803  | 0.699301  |
| 9  | 1.10937500 | 1.11718750 | 1.11328125 | −0.00121649 | 0.350877  |
| 10 | 1.11328125 | 1.11718750 | 1.11523438 | 0.00149600  | 0.175131  |
| 11 | 1.11328125 | 1.11523438 | 1.11425781 | 0.00013981  | 0.087642  |
| 12 | 1.11328125 | 1.11425781 | 1.11376953 | −0.00053832 | 0.043840  |
| 13 | 1.11376953 | 1.11425781 | 1.11401367 | −0.00019925 | 0.021915  |
| 14 | 1.11401367 | 1.11425781 | 1.11413574 | −0.00002972 | 0.010957  |
| 15 | 1.11413574 | 1.11425781 | 1.11419678 | 0.00005505  | 0.005478  |
| 16 | 1.11413574 | 1.11419678 | 1.11416626 | 0.00001266  | 0.002739  |
| 17 | 1.11413574 | 1.11416626 | 1.11415100 | −0.00000853 | 0.001370  |
| 18 | 1.11415100 | 1.11416626 | 1.11415863 | 0.00000207  | 0.000685  |
| 19 | 1.11415100 | 1.11415863 | 1.11415482 | −0.00000323 | 0.000342  |
| 20 | 1.11415482 | 1.11415863 | 1.11415672 | −0.00000058 | 0.000171  |
| 21 | 1.11415672 | 1.11415863 | 1.11415768 | 0.00000074  | 0.000086  |
| 22 | 1.11415672 | 1.11415768 | 1.11415720 | 0.00000008  | 0.000043  |

---

## 6. Verificación final (listado de comprobación del enunciado)

- ✅ Decimales en pantalla **limitados dinámicamente** por las cifras
  significativas (`E_s = 0.5·10^(2−n) %`); el `double` completo solo aparece en
  los tooltips de inspección.
- ✅ Condiciones `f(a)·f(m) < 0` y `f(a)·f(m) > 0` implementadas sin
  redundancias (un único producto `fa·fm` evaluado una vez).
- ✅ `e_a` **no se calcula** en la iteración 1: se representa con
  `std::optional` ausente (en la UI se muestra «—»).
- ✅ Verificación de raíces e iteraciones ejecutable: `./build/Biseccion --verificar`.
- ✅ Parser ampliado para funciones digitadas libremente: multiplicación
  implícita, 19 funciones, `log` base 10, constantes `π`/`e`/`τ` y notación
  científica con mirilla; cubierto por `tests/` y CTest (`ctest`).
- ✅ Código modular, comentado en español y con almacenamiento gestionado por
  RAII (sin `new`/`delete` manuales).
- ✅ Identificadores, comentarios y rótulos en español; palabras reservadas de
  C++ y biblioteca estándar en inglés.

## 7. Hoja de ruta: ampliación a otros métodos

La aplicación es el **primer módulo** de una calculadora de métodos numéricos.
La tabla, el gráfico y el panel procedimental son reutilizables: solo cambia el
núcleo. La tabla siguiente resume el estado de cada método.

| Estado | Método | Notas de diseño |
|---|---|---|
| ✅ **Implementado** | Bisección | 5 pasos, `E_s = 0.5·10^(2−n) %`, cancelable con `stop_token` |
| 📌 Siguiente | Falsa posición (regla falsa) | Reutiliza `core/`; converge más rápido que la bisección |
| 📌 Siguiente | Newton-Raphson | Requiere `f'(x)`; se puede derivar del AST del parser |
| 📌 Siguiente | Secante | Dos puntos de arranque; evita la derivada |
| 📌 Siguiente | Punto fijo (`x = g(x)`) | Criterio de convergencia `|g(x) − x| < E_s` |
| 💡 Planeado | Raíces múltiples (deflación) | Reutiliza la bisección sobre cada factor |
| 💡 Planeado | Integración numérica | Trapecio, Simpson y Simpson compuesto |
| 💡 Planeado | Interpolación | Lagrange, diferencias divididas y Newton |
| 💡 Planeado | Sistemas lineales | Gauss, Gauss-Jordan, LU y Jacobi |
| 💡 Planeado | Ecuaciones diferenciales | Euler y Runge-Kutta 4 |

### Cómo se incorpora un método nuevo

1. **Núcleo** — `src/core/<Metodo>.hpp/.cpp`, sin Qt, con la misma forma que
   `Biseccion::resolver`: `std::expected<Resultado, Error>` + `std::stop_token`.
   `Iteracion` crece con los campos que ese método necesite (derivada, error
   absoluto, segundo punto, …) sin romper los existentes.
2. **Pruebas** — un `tests/<metodo>_test.cpp` registrado en el `CMakeLists.txt`
   con `registrar_prueba(...)`; se compila sin Qt, igual que las actuales.
3. **Interfaz** — un selector de método en `PanelEntrada` y un panel
   procedimental que describa los pasos del algoritmo nuevo.
   `ModeloIteraciones` se reutiliza tal cual (muestra `k`, `a`, `b`, `m`,
   `f(m)`, `e_a`).
4. **Documentación** — sección de resultados en este README y fila en la tabla
   anterior.

> Los métodos iterativos (bisección, Newton, secante, punto fijo) comparten panel
> de entrada, tabla y gráfico sin cambios. Los que no lo son (integración,
> interpolación, sistemas lineales) necesitarán su propia tabla, pero podrán
> reutilizar el gráfico y el formateo de cifras significativas.

## 8. Licencias y atribuciones

- Código de la aplicación: proyecto académico original (ISUM, 8.º semestre).
- **QCustomPlot 2.1.1** © Emanuel Eichhammer, distribuido bajo **GPL-3.0**
  (archivo `third_party/qcustomplot/GPL.txt`). No se ha modificado.