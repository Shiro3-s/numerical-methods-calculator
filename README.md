# Métodos Numéricos — Aplicación Interactiva (C++ / Qt 6)

Aplicación gráfica de escritorio para **métodos numéricos** de la carrera de
Ingeniería, con entrada de funciones, tabla de iteraciones, gráfico interactivo
(inspirado en GeoGebra) y vista procedimental paso a paso. La presentación de
decimales se limita **dinámicamente** según las cifras significativas calculadas.

> **Estado actual: dos métodos completos.** La aplicación cubre el **método de
> bisección** (con la solución matemática de los cuatro ejercicios aplicados del
> enunciado) y el **método de Newton-Raphson** (con `f'(x)` escrita por el
> usuario, tangente en el gráfico y guardas contra derivada nula o divergencia).
> El núcleo vive en `src/core/`, no depende de Qt y devuelve resultados con
> errores tipados, de modo que cada método se añade como un módulo nuevo sin
> tocar la interfaz. Ver
> [§ 7 · Hoja de ruta](#7-hoja-de-ruta-ampliacion-a-otros-metodos).

---

## 1. Arquitectura de la Aplicación

```
Calculadora_metodos_numericos/
├── CMakeLists.txt                        # build con Qt6 + QCustomPlot + tests
├── third_party/qcustomplot/              # QCustomPlot 2.1.1 (GPL-3.0, ver GPL.txt)
├── tests/
│   ├── parser_test.cpp                   # sintaxis y evaluación del parser
│   ├── biseccion_test.cpp                # raíces, iteraciones y cifras E1–E4
│   ├── metodo_test.cpp                   # catálogo, descriptores y contrato
│   ├── newton_test.cpp                   # Newton-Raphson: raíces y guardas
│   └── gui_test.cpp                      # GUI offscreen (descriptor, bloqueo)
└── src/
    ├── main.cpp                          # QApplication + modos de consola
    ├── core/                             # lógica pura (sin dependencia de Qt)
    │   ├── Resultado.hpp/.cpp            # modelo de datos común a todo método
    │   ├── Metodo.hpp/.cpp               # contrato + catálogo de métodos
    │   ├── Biseccion.hpp/.cpp            # bisección (implementa MetodoNumerico)
    │   ├── NewtonRaphson.hpp/.cpp        # Newton-Raphson (idem)
    │   ├── CifrasSignificativas.hpp/.cpp # E_s = 0.5·10^(2−n) % → decimales
    │   └── Funcion.hpp/.cpp              # parser de expresiones → std::function
    ├── ejercicios/
    │   └── Ejercicios.hpp/.cpp           # datos de validación E1–E4 (consola y tests)
    └── gui/
        ├── VentanaPrincipal.hpp/.cpp     # integración de los 4 paneles + hilo
        ├── PanelEntrada.hpp/.cpp         # Panel 1 · selector de método + campos
        ├── ModeloIteraciones.hpp/.cpp    # modelo de tabla dirigido por descriptor
        ├── GraficoBiseccion.hpp/.cpp     # Panel 2 · gráfico interactivo
        ├── TablaIteraciones.hpp/.cpp     # Panel 3 · tabla clickeable
        └── PanelProcedimiento.hpp/.cpp   # Panel 4 · detalle paso a paso
```

**Separación de capas.** El núcleo (`core/`) es 100 % C++ estándar y se puede
probar en consola (`--verificar` / `--tablas`); la capa de presentación
(`gui/`) consume los mismos tipos (`Iteracion`, `Resultado`) sin duplicar lógica.

**La GUI está dirigida por el descriptor del método.** `DescriptorMetodo` declara el
tipo de resolución, las columnas de la tabla, la etiqueta de la expresión auxiliar,
el nombre del punto que se aproxima (`m` o `x`), si el método admite estimación
absoluta de iteraciones, si debe mostrar gráfico y si debe dibujar la recta
asociada a la iteración. `PanelEntrada` hace visibles unos campos u otros según
ese descriptor, `ModeloIteraciones` construye sus encabezados a partir de las
columnas declaradas, `GraficoBiseccion` decide qué resaltar y `PanelProcedimiento`
narra los pasos del método. Consecuencia práctica: **añadir un método nuevo no
requiere tocar la tabla, el gráfico ni el panel de entrada**. Solo hay que
implementar `MetodoNumerico` (dos métodos) y registrarlo en `catalogoMetodos()`.
Ver [§ 7 · Hoja de ruta](#7-hoja-de-ruta-ampliacion-a-otros-metodos).

`./build/Biseccion --metodos` imprime el catálogo completo (tipo de entrada,
columnas, gráfica y trazado asociado de cada método): es la forma rápida de
revisar si el descriptor de un método nuevo dice lo que debería.

### Flujo de datos

1. El usuario elige un **método** (no un ejercicio), escribe `f(x)` — y `f'(x)` si
   el método lo pide — más los datos que ese método requiere: `a, b` para bisección,
   `x₀` para Newton-Raphson.
2. `parsearFuncion` valida cada expresión (errores tipados en español, con posición).
3. `crearDesdeEntrada` construye el método ya configurado y `resolver` ejecuta el
   algoritmo en un `std::jthread` (cancelable con `stop_token`), devolviendo
   `std::expected<Resultado, ErrorMetodo>`.
4. El resultado se distribuye a la tabla (columnas del descriptor), al gráfico y al resumen.
5. Al seleccionar una fila, el gráfico resalta lo que corresponde a ese método: el
   intervalo `[a, b]` con sus puntos `a, b, m` en bisección, o el punto `x_k` y su
   **recta tangente** en Newton-Raphson. El panel derecho muestra la sustitución
   aritmética de esa iteración.
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
./build/Biseccion --metodos           # describe el catálogo de métodos (descriptor de cada uno)
./build/Biseccion --verificar         # verificación matemática de bisección (E1–E4)
./build/Biseccion --tablas            # tablas completas de iteraciones
./build/Biseccion --comparar          # bisección frente a Newton-Raphson en las mismas raíces
ctest --test-dir build --output-on-failure   # 5 pruebas: parser, bisección, contrato, Newton, GUI
```

Opciones de configuración:

```bash
cmake -S . -B build-debug -DENABLE_SANITIZERS=ON    # ASan + UBSan
cmake -S . -B build-tsan  -DENABLE_TSAN=ON          # ThreadSanitizer
cmake -S . -B build -DSTRICT_WARNINGS=OFF           # relaja -Werror
cmake -S . -B build -DBUILD_GUI_TESTS=OFF           # omite la prueba de interfaz (sin Qt Widgets)
```

`ENABLE_TSAN` y `ENABLE_SANITIZERS` son mutuamente excluyentes: ASan y TSan no
conviven en el mismo binario. Bajo TSan la prueba `gui_tests` queda
deshabilitada a propósito (ver abajo); los otros cuatro sí se validan.

Prueba automatizada de la interfaz (recorre bisección y Newton-Raphson y guarda
una captura de cada uno en `/tmp/opencode/`):

```bash
QT_QPA_PLATFORM=offscreen BISECCION_AUTOTEST=1 ./build/Biseccion
```

### Endurecimiento aplicado (skill modern-cpp)

- Estándar **C++23** (`CMAKE_CXX_STANDARD 23`).
- `-Wall -Wextra -Wpedantic -Werror` (advertencias = errores).
- `-D_FORTIFY_SOURCE=3`, `-fstack-protector-strong`, `-fstack-clash-protection`,
  `-ftrivial-auto-var-init=zero`, `-fPIC`.
- `-fPIE` + `-pie`: ejecutable reubicable, para que el ASLR pueda protegerlo
  (`file build/Biseccion` debe decir `pie executable`).
- `-Wl,-z,relro,-z,now`.
- `-DENABLE_SANITIZERS=ON` activa ASan + UBSan en depuración.
- `-DENABLE_TSAN=ON` activa ThreadSanitizer. La resolución corre en un
  `std::jthread` mientras la interfaz sigue viva, así que la skill pide TSan
  para código concurrente. Ver la nota sobre Qt más abajo.
- La biblioteca heredada QCustomPlot se compila con advertencias silenciadas
  (`-w`) para no contaminar los diagnósticos del código propio.

#### Por qué TSan no cubre la prueba de GUI

Qt no está anotado para TSan. Bajo TSan, `gui_tests` produce ~28 informes, y
**ninguno** es de este código: todos están dentro de `libQt6Core`/`libQt6Gui`
(`QThreadPoolPrivate::enqueueTask`, `QWaitCondition`, `QThread::start`),
alcanzados desde `QCustomPlot::replot()`, que decodifica fuera de línea en un
hilo del pool de Qt. El paso de resultado del `std::jthread` a la interfaz con
`QMetaObject::invokeMethod(..., Qt::QueuedConnection)` no produjo ni un informe:
el event loop de Qt aporta el orden happens-before que TSan necesita, así que el
hilo de cálculo y el de la GUI sí están correctamente sincronizados.

Tampoco conviene silenciar el ruido con un fichero de supresiones: la mayoría de
los símbolos de Qt llegan al informe sin nombre, y TSan casa un patrón de
supresión con **cualquier** frame del stack. Como Qt despacha todo por su event
loop, una regla `race:libQt6Core.so` no silencia solo a Qt: silencia también las
carreras de este proyecto. Se comprobó con un global escrito a la vez por el hilo
de cálculo y por el de la GUI: con la regla por biblioteca los dos informes
desaparecían. Por eso `cmake/tsan.supp` no existe y la prueba se deshabilita bajo
TSan en vez de tapar los Findings. Si algún día hay que cubrir la GUI con TSan,
habrá que hacerlo con Qt compilado con `-fsanitize=thread` o sobre los símbolos
del proyecto, no sobre los de Qt.

---

## 3. Implementación Lógica: los algoritmos y las cifras significativas (C++)

### 3.0 El contrato común (`Metodo.hpp` / `Resultado.hpp`)

Antes de los algoritmos hay dos piezas que hacen que «añadir un método» sea
adicionar un archivo y registrar una línea:

| Pieza | Contenido |
|---|---|
| `Resultado` | `Iteracion` (datos crudos de una vuelta), `MotivoParada`, `ErrorMetodo`, `CampoIteracion`, `ColumnaMetodo` |
| `MetodoNumerico` | interfaz con `descriptor()` y `resolver(Entrada, stop_token)` → `std::expected<Resultado, ErrorMetodo>` |
| `DescriptorMetodo` | lo que la GUI necesita para configurarse sola: tipo de entrada, columnas, etiqueta de la expresión auxiliar, si hay gráfico, si hay gráfica asociada |
| `Entrada` | datos del formulario ya traducidos: `expresion`, `expresionAuxiliar`, `a`, `b`, `x₀`, `n`, y los campos del sistema (reservados) |

`TipoResolucion` (`RaizIntervalo`, `RaizPuntoInicial`, `Sistema`) vive en
`Resultado.hpp` porque las iteraciones también lo necesitan: `valorCampo` lo usa
para devolver **vacío** —no un cero sin significado— cuando se le pide un campo
que el método no usa (`f(a)` en Newton-Raphson, `x` en bisección).

### 3.1 Bisección: los 5 pasos obligatorios

`Biseccion::resolver` implementa literalmente la estructura exigida:

| Paso | Definición | Implementación |
|---|---|---|
| 1 | Verificación inicial: si `f(a)·f(b) < 0` existe raíz en `[a, b]` | si `fa*fb >= 0` → `std::unexpected(ErrorMetodo::SinCambioDeSigno)` |
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

El mismo parser analiza `f'(x)`: es el usuario quien escribe la derivada (como
pide el enunciado), y la aplicación la valida con el mismo rigor que `f(x)`,
con el mismo tipo de errores y posiciones.

### 3.5 Newton-Raphson

`NewtonRaphson::resolver` aplica `x_{k+1} = x_k − f(x_k)/f'(x_k)` y guarda, en
cada iteración, el **paso** `−f(x_k)/f'(x_k)` para poder mostrarlo y comprobar
que la raíz está al otro lado del eje.

Criterio de parada: **`e_a < E_s`**, con `e_a = |(x_k − x_{k−1})/x_k|·100` y
`E_s = 0.5·10^(2−n) %`. Igual que en bisección, `e_a` es `std::optional` y **no
existe en la iteración 1**. No se usa `|f(x)| < ε` como criterio adicional: con
`n` cifras significativas ya se sabe cuándo parar, y mezclar dos criterios hace
las tablas menos predecibles.

Dos salvaguardas de seguridad que el enunciado no pide pero que un método iterativo
debe tener:

| Situación | `MotivoParada` | Por qué importa |
|---|---|---|
| `f'(x_k) = 0` | `DerivadaNula` | el paso sería infinito; el método no puede seguir (típico en `x³−3x` con `x₀ = 1`) |
| `x` o `f(x)` deja de ser finito, o `|x|` se dispara | `Divergente` | la iterada se ha ido al infinito; el enunciado no la cubre, pero una app debe decirlo y parar |

Ningún motivo de parada es un *error*: la ejecución ocurrió y su historia es
válida, así que la tabla y el gráfico se rellenan igual y el panel procedimental
explica en español por qué se terminó (`PanelProcedimiento::narrarMotivo`).

Como el enunciado no impone una estimación previa de iteraciones (el número
depende de la curvatura), `DescriptorMetodo::admiteEstimacionAbsoluta` es
`false` para Newton-Raphson y el panel de entrada no enseña el pre-cálculo de
§3.2 en ese método.

### 3.6 Comparación de los dos métodos (`--comparar`)

Sobre las mismas cuatro raíces de aplicación, con `n = 6` cifras:

| `f(x)` | bisección | Newton-Raphson |
|---|---|---|
| `x - cos(x)` | 22 | 5 |
| `ln(x) - x + 2` | 20 | 5 |
| `exp(x) - 5x` | 23 | 5 |
| `x*sin(x) - 1` | 22 | 4 |

Es el argumento estándar a favor de Newton-Raphson (convergencia cuadrática) y
en contra de su fragilidad (necesita `f'` y un buen `x₀`): por eso el método
pide la derivada al usuario y comprueba que `f'(x_k) ≠ 0`.

> **Intervalo al teclear a mano.** El usuario escribe `f(x)` y los extremos
> `[a, b]` a mano, así que nada impide que el intervalo no encaje con la función.
> El paso 1 lo cubre siempre: si `f(a)·f(b) ≥ 0` se informa **«Sin cambio de
> signo»** mostrando `f(a)` y `f(b)`, que es exactamente lo que hay que mirar
> para saber cuál de los tres números escritos está mal. (Antes, cuando los
> ejercicios eran presets, además se avisaba de que `f(x)` se había editado a
> mano; sin preset con el que comparar, ese aviso no tenía sentido y se
> eliminó.)

---

## 4. Implementación de Interfaz Gráfica (C++)

La ventana principal organiza los **cuatro paneles** con `QSplitter`:

```
┌───────────────────┬──────────────────────────────────────────────┐
│ 1 · ENTRADA       │  2 · GRÁFICO INTERACTIVO  (QCustomPlot)      │
│   Método: ▾       │      f(x) + lo que	resalte el método        │
│   f(x), [f'(x)],  ├──────────────────────────────────────────────┤
│   a,b  o  x₀, n   │  3 · TABLA DE ITERACIONES (QTableView)        │
│   Resolver/Cancel │   columnas según el método (ver abajo)       │
├───────────────────┤  (clic en fila ⇄ clic en marcador del gráfico)│
│ 4 · PROCEDIMIENTO │                                              │
└───────────────────┴──────────────────────────────────────────────┘
```

El combo superior selecciona el **método**, no el ejercicio. Al cambiarlo, el panel
reconfigura sus propios campos según el descriptor, sin tocar el código:

| Método | Campos que muestra | Columnas de la tabla | Gráfico |
|---|---|---|---|
| Bisección | `f(x)`, `a`, `b`, `n` | `k │ a │ b │ m │ f(m) │ e_a (%)` | sí · intervalo `[a, b]` |
| Newton-Raphson | `f(x)`, `f'(x)`, `x₀`, `n` | `k │ x │ f(x) │ f'(x) │ paso │ e_a (%)` | sí · tangente en `x_k` |
| *Jacobi (previsto)* | `n × n`, matriz aumentada | `k │ x₁…xₙ │ r₁…rₙ` | **no** (oculto) |

Cuando el descriptor declara `muestraGrafico == false`, el gráfico se oculta
(`limpiar()` hace `setVisible(false)`) y la tabla queda como único panel derecho:
es el camino que usará un sistema de ecuaciones, donde no hay una curva que
dibujar. Un panel vacío con dos ejes ocupa media ventana y no enseña nada.

### Qué resalta el gráfico (y por qué no es «el gráfico de bisección»)

`GraficoBiseccion` no sabe qué método se está ejecutando: recibe el
`DescriptorMetodo` y decide. La clave es que **el intervalo y la tangente son la
misma idea en distinto método**:

| Método de intervalo | Método de punto inicial |
|---|---|
| zona sombreada `[a, b]` y guías verticales en `a` y `b` | — |
| tracer y etiqueta en `a`, en `b` y en `m` | tracer y etiqueta solo en `x` |
| — | **recta tangente** `y − f(x_k) = f'(x_k)·(x − x_k)` |

Que `a` y `b` valgan `0` en una iteración de Newton-Raphson es indistinguible de
un cero legítimo, así que el gráfico pregunta al descriptor (`usaIntervalo`) en
lugar de mirar el valor; si no lo hiciera, registraría dos marcadores falsos en
`(0, 0)` que el usuario podría pulsar.

### Interactividad (enlace Tabla ⇄ Gráfico ⇄ Procedimiento)

- **Fila seleccionada en la tabla** → `TablaIteraciones::selectionChanged`
  emite `iteracionSeleccionada(k)`; `VentanaPrincipal` actualiza el gráfico
  (`GraficoBiseccion::resaltarIteracion`) y el panel procedimental
  (`PanelProcedimiento::mostrarIteracion`).
- **Clic en el gráfico** → busca el marcador más cercano (13 px) y emite
  `marcadorClickeado(k)`, que selecciona la fila equivalente en la tabla.
- **Gráfico**: sombreado translúcido del intervalo actual, guías verticales en
  `a` y `b`, tracers sobre la curva para `a`, `b` y el punto aproximado
  (`m` o `x`), etiquetas; soporta arrastre y zoom (los marcadores se re-posicionan
  al cambiar el rango).
- **Vista procedimental**: sustitución aritmética con los valores reales
  formateados a las `n` cifras significativas. Los pasos narrativos son **del
  método**: bisección muestra sus cinco pasos (`f(a)·f(b)`, `m = (a+b)/2`, criterio
  de signo, `e_a`, presentación) y Newton-Raphson los suyos (`xₖ`, `f'(xₖ)`, el
  paso `xₖ − f/f'`, `e_a`). El descriptor decide cuál se narra.

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

> **Dónde están ahora los ejercicios.** El enunciado pide resolver los cuatro
> ejercicios de aplicación dentro de la aplicación. Como el selector pasó a
> elegir **método** y no ejercicio (requisito de la ampliación), los cuatro casos
> ya **no son presets del desplegable**: viven en `src/ejercicios/Ejercicios.cpp`
> como *fixture* de datos, que alimenta tres cosas:
>
> 1. La **verificación de consola** (`--verificar`, `--tablas`, `--comparar`), que
>    reproduce exactamente las tablas que se pegan en la memoria.
> 2. Las **pruebas automáticas** (`tests/biseccion_test.cpp`, `tests/newton_test.cpp`).
> 3. La **lectura manual**: en la GUI se teclean en 20 segundos
>    (`x - cos(x)`, `0`, `1`, `n = 6`, *Resolver*). Los valores de abajo son los
>    que salen.
>
> Cada ejercicio lleva ahora también su derivada exacta, de modo que las mismas
> cuatro raíces validan bisección **y** Newton-Raphson.

Resumen de raíces obtenidas (la tabla completa se genera con
`./build/Biseccion --tablas`):

| Ejercicio | f(x) | f'(x) | [a, b] | Raíz (double) | f(raíz) |
|---|---|---|---|---|---|
| 5.1 · Equilibrio | x − cos(x) | 1 + sen(x) | [0, 1] | **0.739085133215159** | 2.5·10⁻¹⁵ |
| 5.2 · Tráfico de red | ln(x) − x + 2 | 1/x − 1 | [3, 4] | **3.146193220620589** | 4.4·10⁻¹⁵ |
| 5.3 · IoT energético | eˣ − 5x | eˣ − 5 | [0, 1] | **0.259171101819073** | 1.1·10⁻¹⁵ |
| 5.4 · Resonancia | x·sen(x) − 1 | sen(x) + x·cos(x) | [0, 2] | **1.114157140871928** | 3.0·10⁻¹⁵ |

Iteraciones necesarias según el criterio (absoluto vs. relativo `e_a < E_s`):

| n cifras | E1 abs/rel | E2 abs/rel | E3 abs/rel | E4 abs/rel |
|---|---|---|---|---|
| 4 | 15 / 15 | 15 / 13 | 15 / 17 | 16 / 16 |
| 5 | 18 / 19 | 18 / 16 | 18 / 20 | 19 / 19 |
| 6 | 21 / 22 | 21 / 20 | 21 / 23 | 22 / 22 |

Las mismas cuatro raíces con Newton-Raphson necesitan **5, 5, 5 y 4**
iteraciones (`x₀` = punto medio del intervalo, `n = 6`): ver §3.6 y
`./build/Biseccion --comparar`.

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
- ✅ **Ampliación a métodos adicionales**: el selector elige método (bisección,
  Newton-Raphson), cada uno con sus datos de entrada, sus columnas y su
  narración procedimental, sin código específico en la GUI (§1, §7).
- ✅ Prueba automatizada de la interfaz bajo `QT_QPA_PLATFORM=offscreen`
  (`tests/gui_test.cpp`, `ctest`): columnas correctas por método, gráfico
  visible/oculto según el descriptor y formulario reutilizable tras un error.

## 7. Hoja de ruta: ampliación a otros métodos

La aplicación es un **módulo** de una calculadora de métodos numéricos. Al ser
la GUI enteramente dirigida por el descriptor del método (§1), la tabla, el
gráfico y el panel procedimental se reutilizan tal cual: lo único que cambia es
el núcleo y su descriptor.

| Estado | Método | Notas de diseño |
|---|---|---|
| ✅ **Implementado** | Bisección | 5 pasos, `E_s = 0.5·10^(2−n) %`, cancelable con `stop_token` |
| ✅ **Implementado** | Newton-Raphson | Un punto inicial + `f'(x)` escrita por el usuario; paradas `DerivadaNula` y `Divergente`; el gráfico traza la tangente |
| 📌 Siguiente | **Jacobi** (sistemas) | `TipoResolucion::Sistema`: dimension + matriz aumentada, `‖Δx‖_∞ < E_s`, aviso de diagonal dominante, **sin gráfico** |
| 📌 Siguiente | Falsa posición (regla falsa) | Reutiliza `core/`; converge más rápido que la bisección |
| 📌 Siguiente | Secante | Dos puntos de arranque; evita la derivada |
| 📌 Siguiente | Punto fijo (`x = g(x)`) | Criterio de convergencia `|g(x) − x| < E_s` |
| 💡 Planeado | Raíces múltiples (deflación) | Reutiliza la bisección sobre cada factor |
| 💡 Planeado | Integración numérica | Trapecio, Simpson y Simpson compuesto |
| 💡 Planeado | Interpolación | Lagrange, diferencias divididas y Newton |
| 💡 Planeado | Ecuaciones diferenciales | Euler y Runge-Kutta 4 (el resultado es una tabla de pares `(x, y)`) |

### Cómo se incorpora un método nuevo

Cuatro pasos. **Ninguno toca `ModeloIteraciones`, `TablaIteraciones` ni
`PanelEntrada`**:

1. **Núcleo** — `src/core/<Metodo>.hpp/.cpp`, sin Qt, con la misma forma que
   `Biseccion::resolver`: `descriptor()` + `resolver(Entrada, stop_token)` →
   `std::expected<Resultado, ErrorMetodo>`. Si necesita campos nuevos, se añaden
   a `Iteracion` como `std::optional` (como se hizo con `x`, `fx`, `fdx`, `paso`)
   para no romper los métodos existentes, y se declara en `CampoIteracion` +
   `valorCampo` con su `TipoResolucion` correspondiente.
2. **Descriptor** — en `Metodo.cpp::catalogoMetodos()` se añade una línea con el
   método ya configurado. Ahí se declara el tipo de entrada, las columnas
   (`ColumnaMetodo`), la etiqueta de la expresión auxiliar y si hay gráfico y
   trazado asociado. `./build/Biseccion --metodos` lo imprime para revisarlo.
3. **Pruebas** — un `tests/<metodo>_test.cpp` registrado con
   `registrar_prueba(...)`, sin Qt. Si el método tieneGUI propia (campos
   especiales, como la matriz de Jacobi), se amplía `tests/gui_test.cpp`.
4. **Narrativa y documentación** — un caso más en
   `PanelProcedimiento::mostrarResumen` (descrito por `descriptor.tipo`) y una
   sección en este README.

> **Jacobi, el siguiente paso.** Es el que más pone a prueba el diseño, porque
> es el primero con `TipoResolucion::Sistema`: la iteración es un **vector**, la
> entrada necesita una matriz aumentada (dimension + `QTableWidget`), el criterio
> de parada pasa a ser `‖Δx‖_∞ < E_s` y el descriptor declarará
> `muestraGrafico = false`, que es exactamente el camino que ya hace la GUI
> cuando no hay nada que dibujar.

> Los métodos iterativos de una incógnita (bisección, Newton, secante, punto
> fijo) comparten panel de entrada, tabla y gráfico sin cambios. Los que no lo
> son (integración, interpolación, sistemas) necesitarán su propia tabla —el
> descriptor ya permite declarar columnas nuevas— pero reutilizarán el formato
> de cifras significativas y la lógica de errores.

## 8. Licencias y atribuciones

- Código de la aplicación: proyecto académico original (ISUM, 8.º semestre).
- **QCustomPlot 2.1.1** © Emanuel Eichhammer, distribuido bajo **GPL-3.0**
  (archivo `third_party/qcustomplot/GPL.txt`). No se ha modificado.