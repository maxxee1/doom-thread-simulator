#include "sim.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


typedef struct {
    Mundo *mundo;
    int indice;
} Argumentos;

static void evaluar_termino(Mundo *mundo) {
    int heroes_vivos = 0; 
    int rutas_completadas = 0; 
    
    for (int i = 0; i < mundo->cantidad_heroes; i++) {
        heroes_vivos += mundo->heroes[i].vivo ? 1 : 0;
    }
    for (int i = 0; i < mundo->cantidad_heroes; i++) {
        rutas_completadas += (mundo->heroes[i].indice_ruta >= mundo->heroes[i].largo_ruta);
    }

    if (heroes_vivos == 0) {
        g_finalizado = 1;
        g_motivo_final = FIN_HEROES_MUERTOS;
    } else if (rutas_completadas == mundo->cantidad_heroes) {
        g_finalizado = 1;
        g_motivo_final = FIN_HEROES_COMPLETARON;
    }
}

/**********************HILO DEL HEROE**********************/
static void *hilo_heroe(void *p) {
    Argumentos *args = (Argumentos*)p;

    while (1) {
        barrera_inicio();

        pthread_mutex_lock(&g_bloqueo);
        paso_heroe(args->mundo, args->indice);
        pthread_mutex_unlock(&g_bloqueo);

        barrera_fin();

        pthread_mutex_lock(&g_bloqueo);
        if (g_finalizado) {
            pthread_mutex_unlock(&g_bloqueo);
            break;
        }
        pthread_mutex_unlock(&g_bloqueo);
    }
    return NULL;
}

/**********************HILO DEL MONSTRUO**********************/
static void *hilo_monstruo(void *p) {
    Argumentos *args = (Argumentos*)p;
    while (1) {
        barrera_inicio();

        pthread_mutex_lock(&g_bloqueo);
        paso_monstruo(args->mundo, args->indice);
        pthread_mutex_unlock(&g_bloqueo);

        barrera_fin();

        pthread_mutex_lock(&g_bloqueo);
        if (g_finalizado) {
            pthread_mutex_unlock(&g_bloqueo);
            break;
        }
        pthread_mutex_unlock(&g_bloqueo);
    }
    return NULL;
}

/**********************HILO DEL MONITOR**********************/
static void *hilo_monitor(void *p) {
    Mundo *mundo = (Mundo*)p;
    long ultimo_tick = -1;

    while (1) {
        pthread_mutex_lock(&g_bloqueo);

        while (ultimo_tick == g_tick && !g_finalizado) {
            pthread_cond_wait(&g_tick_cv, &g_bloqueo);
        }

        long t = g_tick;
        ultimo_tick = t;

        if (t % 10 == 0) {
            printf("[tick %ld]\n", t);

            for (int h = 0; h < mundo->cantidad_heroes; h++) {
                Heroe *H = &mundo->heroes[h];
                printf("  Héroe%d: pos=(%d,%d) vida=%d vivo=%d ruta=%d/%d\n",
                       h, H->pos.x, H->pos.y, H->hp, H->vivo, H->indice_ruta, H->largo_ruta);
            }

            for (int m = 0; m < mundo->cantidad_monstruos; m++) {
                Monstruo *M = &mundo->monstruos[m];
                printf("  Monstruo%d: pos=(%d,%d) vida=%d despierto=%d vivo=%d\n",
                       m + 1, M->pos.x, M->pos.y, M->hp, M->despierto, M->vivo);
            }
        }

        evaluar_termino(mundo);
        int termino = g_finalizado;
        MotivoFinal motivo = g_motivo_final;
        pthread_mutex_unlock(&g_bloqueo);

        if (termino) {
            printf("== Fin de la simulación ==\n");
            if (motivo == FIN_HEROES_MUERTOS)
                printf("Motivo: todos los héroes han muerto.\n");
            else if (motivo == FIN_HEROES_COMPLETARON)
                printf("Motivo: todos los héroes completaron su camino.\n");
            else
                printf("Motivo: condición de término no especificada.\n");
            break;
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s config.txt\n", argv[0]);
        return 1;
    }

    Mundo mundo = (Mundo){0};
    if (parsear_config(argv[1], &mundo) != 0) {
        fprintf(stderr, "Error al parsear %s\n", argv[1]);
        return 2;
    }

    setvbuf(stdout, NULL, _IOLBF, 0);

    // Banner y resumen inicial en español
    printf("=== Simulador Doom (barrera de ticks) ===\n");
    printf("Mapa: %dx%d  Héroes:%d  Monstruos:%d\n",
           mundo.grilla.ancho, mundo.grilla.alto,
           mundo.cantidad_heroes, mundo.cantidad_monstruos);

    for (int h = 0; h < mundo.cantidad_heroes; h++) {
        Heroe *HH = &mundo.heroes[h];
        printf("Héroe%d inicio=(%d,%d) vida=%d daño=%d rango=%d pasos=%d\n",
               h, HH->pos.x, HH->pos.y, HH->hp,
               HH->danio_ataque, HH->rango_ataque, HH->largo_ruta);
    }

    for (int m = 0; m < mundo.cantidad_monstruos; m++) {
        Monstruo *MM = &mundo.monstruos[m];
        printf("Monstruo%d en (%d,%d) vida=%d visión=%d daño=%d rango=%d\n",
               m + 1, MM->pos.x, MM->pos.y, MM->hp,
               MM->rango_vision, MM->danio_ataque, MM->rango_ataque);
    }

   
    g_mundo = &mundo;
    g_finalizado = 0;
    g_motivo_final = FIN_NINGUNO;
    g_tick = 0;
    g_hilos_totales = mundo.cantidad_heroes + mundo.cantidad_monstruos;

    /**********************LANZAMIENTO INSANO DE LOS HILOS**********************/
    pthread_t *hilos_heroes = calloc(mundo.cantidad_heroes, sizeof(pthread_t));
    pthread_t *hilos_monstruos = calloc(mundo.cantidad_monstruos, sizeof(pthread_t));
    pthread_t hilo_monitor_principal;

    for (int i = 0; i < mundo.cantidad_heroes; i++) {
        Argumentos *args = malloc(sizeof(*args));
        args->mundo = &mundo;
        args->indice = i;
        pthread_create(&hilos_heroes[i], NULL, hilo_heroe, args);
    }

    for (int i = 0; i < mundo.cantidad_monstruos; i++) {
        Argumentos *args = malloc(sizeof(*args));
        args->mundo = &mundo;
        args->indice = i;
        pthread_create(&hilos_monstruos[i], NULL, hilo_monstruo, args);
    }

    pthread_create(&hilo_monitor_principal, NULL, hilo_monitor, &mundo);

    /**********************FULL FIN**********************/
    pthread_join(hilo_monitor_principal, NULL);

    /**********************ESPERANDING A LOS HEROES**********************/
    for (int i = 0; i < mundo.cantidad_heroes; ++i) {
        pthread_join(hilos_heroes[i], NULL);
    }

    /**********************ESPERANDING A LOS MONTRUOS**********************/
    for (int i = 0; i < mundo.cantidad_monstruos; ++i) {
        pthread_join(hilos_monstruos[i], NULL);
    }

    free(hilos_heroes);
    free(hilos_monstruos);
    liberar_mundo(&mundo);
    return 0;
}
