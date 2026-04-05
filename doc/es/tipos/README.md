# Tipos básicos

## Descripción general

netXenium cuenta con los siguientes tipos básicos para representar valores y estructuras de datos en tiempo de ejecución.

## Tabla de tipos

| Tipo      | Descripción                              | Ejemplo                                      |
|-----------|------------------------------------------|----------------------------------------------|
| `number`  | Número decimal                           | `123`                                        |
| `string`  | Cadena de texto ASCII                    | `"Hola Mundo"`                               |
| `boolean` | Valor booleano                           | `true`, `false`                              |
| `tuple`   | Colección de datos inmutable             | `1, 2, 3`                                    |
| `vector`  | Colección de datos mutable               | `? 1, 2, 3`                                  |
| `map`     | Colección de pares clave-valor mutable   | `{key -> "value", [1] -> "number"}`          |
| `nil`     | Valor vacío o ausencia de valor          | `??`                                         |

## Detalle por tipo

### `number`
Representa un número decimal. Se utiliza para operaciones aritméticas y comparaciones numéricas.

```netXenium
local x = 42
local pi = 3.14159
```

### `string`
Representa una cadena de texto en formato ASCII.

```netXenium
local saludo = "Hola Mundo"
```

### `boolean`
Representa un valor lógico de verdad, utilizado en expresiones condicionales y de control de flujo.

```netXenium
local activo = true
local vacio = false
```

### `tuple`
Colección ordenada de datos de tamaño fijo e inmutable. Una vez creado, sus elementos no pueden modificarse.

```netXenium
local coordenada = 10, 20, 30
```

### `vector`
Colección ordenada de datos mutable. Sus elementos pueden agregarse, modificarse o eliminarse en tiempo de ejecución.

```netXenium
local lista = ? 1, 2, 3
```

### `map`
Colección de pares clave-valor mutable. Permite asociar cualquier valor como clave con cualquier valor como dato.

```netXenium
local config = {key -> "value", [1] -> "number"}
```

### `nil`
Representa la ausencia de valor. Se utiliza para indicar que una variable no tiene un dato asignado.

```netXenium
local sinValor = ??
```
