#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

/**********************FUNSION INSAID PARA LEER COORDENADAS**********************/
static int leer_par(const char *s, Coordenada *salida) {
    int x, y;
    if (sscanf(s, " ( %d , %d ) ", &x, &y) == 2) {
        salida->x = x;
        salida->y = y;
        return 0;
    }
    return -1;
}

/**********************PARA DEJAR LAS LINEAS JOYA**********************/
static char* leer_linea(FILE *archivo) {
    char *linea = NULL;
    size_t capacidad = 0;
    ssize_t n = getline(&linea, &capacidad, archivo);

    if (n == -1) {
        free(linea);
        return NULL;
    }

    while (n > 0 && (linea[n-1] == '\n' || linea[n-1] == '\r')) linea[--n] = '\0';
    return linea;
}

/**********************FUNCION PARA LEER LAS CONFIGG**********************/
int parsear_config(const char *ruta, Mundo *mundo) {

    memset(mundo, 0, sizeof(*mundo));
    FILE *archivo = fopen(ruta, "r");
    if (!archivo) {
        perror("fopen");
        return -1;
    }

    char *linea;
    int monstruos_esperados = -1;
    int heroes_esperados = 1; // valor por defecto

    while ((linea = leer_linea(archivo))) {
        if (strncmp(linea, "HERO_COUNT", 10) == 0) {
            sscanf(linea, "HERO_COUNT %d", &heroes_esperados);
        }
        free(linea);
    }

    rewind(archivo);

    mundo->cantidad_heroes = heroes_esperados;
    mundo->heroes = calloc(mundo->cantidad_heroes, sizeof(Heroe));

    for (int i = 0; i < mundo->cantidad_heroes; i++) {
        mundo->heroes[i].vivo = true;
    }

    while ((linea = leer_linea(archivo))) {

        if (linea[0] == '\0' || linea[0] == '#') {
            free(linea);
            continue;
        }

        if (strncmp(linea, "GRID_SIZE", 9) == 0) {
            sscanf(linea, "GRID_SIZE %d %d", &mundo->grilla.ancho, &mundo->grilla.alto);
        }
        else if (strncmp(linea, "HERO_", 5) == 0) {
            int id = 1;
            sscanf(linea, "HERO_%d_", &id);
            int indice = id - 1;

            if (indice < 0 || indice >= mundo->cantidad_heroes) {
                free(linea);
                continue;
            }

            if (strstr(linea, "_HP ")) {
                sscanf(linea, "HERO_%*d_HP %d", &mundo->heroes[indice].hp);
            }
            else if (strstr(linea, "_ATTACK_DAMAGE ")) {
                sscanf(linea, "HERO_%*d_ATTACK_DAMAGE %d", &mundo->heroes[indice].danio_ataque);
            }
            else if (strstr(linea, "_ATTACK_RANGE ")) {
                sscanf(linea, "HERO_%*d_ATTACK_RANGE %d", &mundo->heroes[indice].rango_ataque);
            }
            else if (strstr(linea, "_START ")) {
                sscanf(linea, "HERO_%*d_START %d %d", &mundo->heroes[indice].pos.x, &mundo->heroes[indice].pos.y);
            }
            else if (strstr(linea, "_PATH")) {

                int cantidad = 0;

                for (char *p = linea; *p; ++p) {
                    if (*p == '(') {
                        cantidad++;
                    }
                }

                mundo->heroes[indice].ruta = calloc(cantidad, sizeof(Coordenada));
                mundo->heroes[indice].largo_ruta = cantidad;
                char *p = strchr(linea, '(');
                int c = 0;
                while (p && c < cantidad) {
                    Coordenada coord;
                    if (leer_par(p, &coord) == 0) {
                        mundo->heroes[indice].ruta[c++] = coord;
                    }

                    p = strchr(p + 1, '(');
                }
            }
        }
        else if (strncmp(linea, "HERO_HP", 7) == 0 && mundo->cantidad_heroes == 1) {
            sscanf(linea, "HERO_HP %d", &mundo->heroes[0].hp);
        }
        else if (strncmp(linea, "HERO_ATTACK_DAMAGE", 18) == 0 && mundo->cantidad_heroes == 1) {
            sscanf(linea, "HERO_ATTACK_DAMAGE %d", &mundo->heroes[0].danio_ataque);
        }
        else if (strncmp(linea, "HERO_ATTACK_RANGE", 17) == 0 && mundo->cantidad_heroes == 1) {
            sscanf(linea, "HERO_ATTACK_RANGE %d", &mundo->heroes[0].rango_ataque);
        }
        else if (strncmp(linea, "HERO_START", 10) == 0 && mundo->cantidad_heroes == 1) {
            sscanf(linea, "HERO_START %d %d", &mundo->heroes[0].pos.x, &mundo->heroes[0].pos.y);
        }
        else if (strncmp(linea, "HERO_PATH", 9) == 0 && mundo->cantidad_heroes == 1) {
            int cantidad = 0;

            for (char *p = linea; *p; p++) {
                if (*p == '(') {
                    cantidad++;
                }
            }

            mundo->heroes[0].ruta = calloc(cantidad, sizeof(Coordenada));
            mundo->heroes[0].largo_ruta = cantidad;
            char *p = strchr(linea, '(');
            int c = 0;

            while (p && c < cantidad) {
                Coordenada coord;
                if (leer_par(p, &coord) == 0) {
                    mundo->heroes[0].ruta[c++] = coord;
                }

                p = strchr(p + 1, '(');
            }
        }
        else if (strncmp(linea, "MONSTER_COUNT", 13) == 0) {
            sscanf(linea, "MONSTER_COUNT %d", &monstruos_esperados);
            mundo->cantidad_monstruos = monstruos_esperados;
            mundo->monstruos = calloc(mundo->cantidad_monstruos, sizeof(Monstruo));

            for (int i = 0; i < mundo->cantidad_monstruos; i++) {
                mundo->monstruos[i].vivo = true;
            }
        }
        else if (strncmp(linea, "MONSTER_", 8) == 0) {

            int id = -1;

            sscanf(linea, "MONSTER_%d_", &id);
            int indice = id - 1;

            if (indice < 0 || indice >= mundo->cantidad_monstruos) {
                free(linea);
                continue;
            }

            if (strstr(linea, "_HP ")) {
                sscanf(linea, "MONSTER_%*d_HP %d", &mundo->monstruos[indice].hp);
            }
            else if (strstr(linea, "_ATTACK_DAMAGE ")) { 
                sscanf(linea, "MONSTER_%*d_ATTACK_DAMAGE %d", &mundo->monstruos[indice].danio_ataque);
            }
            else if (strstr(linea, "_VISION_RANGE ")) {
                sscanf(linea, "MONSTER_%*d_VISION_RANGE %d", &mundo->monstruos[indice].rango_vision);
            }
            else if (strstr(linea, "_ATTACK_RANGE ")) {
                sscanf(linea, "MONSTER_%*d_ATTACK_RANGE %d", &mundo->monstruos[indice].rango_ataque);
            }
            else if (strstr(linea, "_COORDS ")) {
                sscanf(linea, "MONSTER_%*d_COORDS %d %d", &mundo->monstruos[indice].pos.x, &mundo->monstruos[indice].pos.y);
            }
        }

        free(linea);
    }
    
    fclose(archivo);
    return 0;
}