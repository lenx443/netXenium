# Conceptos Fundamentales

## Registro

Un **registro** es una referencia a un objeto residente en memoria, diferenciándose de las variables tradicionales al representar directamente las propiedades de una instancia.  
Cada registro pertenece a un contexto específico de acceso y alcance, dividiéndose en tres categorías principales:

### Registros Globales
Corresponden a valores accesibles únicamente desde un contexto global.  
Cada módulo define su propio conjunto de registros globales, los cuales actúan como propiedades del módulo cuando este es importado.

### Registros Estáticos
Asociados a una implementación concreta, los registros estáticos solo pueden ser accedidos o modificados desde la propia implementación o desde las instancias derivadas de ella.

### Registros de Instancia
Son aquellos que pertenecen a una instancia individual.  
Solo pueden ser accedidos y modificados desde la propia instancia, y actúan como propiedades de esta cuando son públicos.

---

## Meta-registro

Un **meta-registro** es un tipo avanzado de registro con la capacidad de **alterar su propio comportamiento** en tiempo de ejecución.  
Este tipo de registro puede definir dinámicamente la lógica que determina cómo será accedido, modificado o evaluado, permitiendo asociar código ejecutable a sus operaciones de lectura y escritura.

