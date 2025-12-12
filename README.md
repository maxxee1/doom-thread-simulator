## Compilación

Desde la raíz del proyecto, ejecutar:

```bash
make clean && make
```

Esto compila todos los archivos del directorio `src/` y genera el ejecutable:

```
./doom_sim
```

---

## Ejecución

Ejecutar el programa indicando el archivo de configuración:

```bash
./doom_sim config_multihero.txt
```

También se pueden probar otros escenarios incluidos:

```bash
./doom_sim config_fin_meta.txt
./doom_sim config_heroe_muere_multi.txt
./doom_sim config_stress.txt
```

---

## Formato del archivo de configuración

Cada archivo define el tamaño del mapa, los héroes y monstruos con sus atributos.

Estructura general:

```
GRID_SIZE <ancho> <alto>

HERO_COUNT <N>

HERO_i_HP <vida>
HERO_i_ATTACK_DAMAGE <daño>
HERO_i_ATTACK_RANGE <rango>
HERO_i_START <x> <y>
HERO_i_PATH (<x1>,<y1>) (<x2>,<y2>) ...

MONSTER_COUNT <M>

MONSTER_i_HP <vida>
MONSTER_i_ATTACK_DAMAGE <daño>
MONSTER_i_VISION_RANGE <visión>
MONSTER_i_ATTACK_RANGE <rango>
MONSTER_i_COORDS <x> <y>
```

Ejemplo mínimo:

```
GRID_SIZE 10 5
HERO_COUNT 1
HERO_1_HP 100
HERO_1_ATTACK_DAMAGE 10
HERO_1_ATTACK_RANGE 1
HERO_1_START 1 1
HERO_1_PATH (2,1) (3,1) (4,1)
MONSTER_COUNT 1
MONSTER_1_HP 30
MONSTER_1_ATTACK_DAMAGE 5
MONSTER_1_VISION_RANGE 3
MONSTER_1_ATTACK_RANGE 1
MONSTER_1_COORDS 8 4
```

---

## Condiciones de término

La simulación finaliza automáticamente cuando se cumple una de las siguientes condiciones:

- Todos los héroes completan su camino → “Motivo: todos los héroes completaron su camino.”
- Todos los héroes mueren → “Motivo: todos los héroes han muerto.”

La muerte de los monstruos no termina la simulación; los héroes continúan hasta su meta.

---

## Archivos incluidos

| Archivo | Descripción |
|----------|-------------|
| `src/main.c` | Inicialización, creación de hilos y control principal. |
| `src/sim.c` | Lógica del juego (movimiento, ataques y sincronización). |
| `src/parser.c` | Lectura del archivo de configuración. |
| `Makefile` | Compilación automatizada. |
| `config_multihero.txt` | Escenario base (2 héroes, 3 monstruos). |
| `config_fin_meta.txt` | Escenario de llegada a la meta. |
| `config_heroe_muere_multi.txt` | Escenario donde todos los héroes mueren. |
| `config_stress.txt` | Prueba de carga con múltiples entidades. |

---

## Ejecución rápida (opcional)

Puedes agregar al `Makefile` la siguiente regla:

```makefile
run:
	./doom_sim config_stress.txt
```

Y ejecutarla con:

```bash
make run
```

---

## Requisitos

- Linux o WSL con soporte pthread  
- GCC versión 9 o superior, o Clang  
- `make` instalado (`sudo apt install build-essential`)  
