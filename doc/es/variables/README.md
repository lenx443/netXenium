# Variables

## Concepto de variable
Una variable es un identificador simbólico que mantiene una asociación dinámica con un valor en tiempo de ejecución, dentro de un entorno de ejecución determinado.

## Características
- La asociación entre el identificador y el valor se crea, modifica y resuelve en tiempo de ejecución.
- La variable no representa una ubicación de memoria fija, sino una referencia lógica gestionada por el intérprete.
- Su existencia y visibilidad están definidas por el scope activo del entorno de ejecución.
- El valor asociado puede cambiar durante la ejecución, incluso en tipo y representación.

## Declaración de variables

### `local` — Alcance léxico
```netXenium
local foo = 1
```

La palabra clave `local` declara una variable con alcance léxico limitado al scope actual, creando un binding explícito entre un identificador y un valor dentro del entorno de ejecución activo.

### `var` — Contexto de ejecución actual
```netXenium
var foo = 3
```

La palabra clave `var` declara una variable en el contexto de ejecución actual, creando un binding dinámico entre un identificador y un valor dentro del entorno activo, conforme a las reglas de resolución de nombres del lenguaje.

### `global` — Contexto global del módulo
```netXenium
global foo = 13
```

La palabra clave `global` declara una variable en el contexto de ejecución global, creando un binding dinámico entre un identificador y un valor dentro del entorno global del módulo, conforme a las reglas de resolución de nombres del lenguaje.

## Resumen de palabras clave

| Palabra clave | Alcance              | Tipo de binding |
|---------------|----------------------|-----------------|
| `local`       | Scope actual         | Léxico          |
| `var`         | Contexto actual      | Dinámico        |
| `global`      | Módulo completo      | Dinámico        |
