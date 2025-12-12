#include "sim.h"
#include <stdio.h>
#include <stdlib.h>

/**********************VAR GLOB**********************/
pthread_mutex_t g_bloqueo = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  g_tick_cv = PTHREAD_COND_INITIALIZER;
long g_tick = 0;

Mundo *g_mundo = NULL;
int g_finalizado = 0;
int g_hilos_totales = 0;
MotivoFinal g_motivo_final = FIN_NINGUNO;

static int llegadas_inicio = 0, fase_inicio = 0;
static int llegadas_fin    = 0, fase_fin    = 0;

int manhattan(Coordenada a, Coordenada b){
    int dx = a.x - b.x; if(dx<0) dx = -dx;
    int dy = a.y - b.y; if(dy<0) dy = -dy;
    return dx + dy;
}

static void esperar_barrera_(int *llegadas, int *fase, int incrementar_tick) {

    int mi_fase = *fase;
    (*llegadas)++;
    if(*llegadas == g_hilos_totales) {
        *llegadas = 0;
        (*fase)++;
        if(incrementar_tick) g_tick++;    
        pthread_cond_broadcast(&g_tick_cv);
    }
    else {
        while(mi_fase == *fase) {
            pthread_cond_wait(&g_tick_cv, &g_bloqueo);
        }
    }
}

void barrera_inicio(void) {
    pthread_mutex_lock(&g_bloqueo);
    esperar_barrera_(&llegadas_inicio, &fase_inicio, 0);
    pthread_mutex_unlock(&g_bloqueo);
}
void barrera_fin(void) {
    pthread_mutex_lock(&g_bloqueo);
    esperar_barrera_(&llegadas_fin, &fase_fin, 1);
    pthread_mutex_unlock(&g_bloqueo);
}

/**********************LOGICAAAAAAAAAAA**********************/
static void alertar_cercanos(Mundo *mundo, int idx_monstruo) {
    Monstruo *M = &mundo->monstruos[idx_monstruo];

    for (int j = 0; j < mundo->cantidad_monstruos; j++) {
        // Si es el mismo monstruo, saltar la iteración
        if (j == idx_monstruo) {
            continue;
        }

        Monstruo *N = &mundo->monstruos[j];

        if (!N->vivo) {
            continue;
        }
        /**********************DESPIERTA A LA GANGG**********************/
        if (manhattan(M->pos, N->pos) <= M->rango_vision) {
            if (!N->despierto) {
                N->despierto = true;
                printf("  [tick %ld] Monstruo%d alerta a Monstruo%d\n", g_tick, idx_monstruo + 1, j + 1);
            }
        }
    }
}

void paso_heroe(Mundo *mundo, int idx_heroe) {
    Heroe *H = &mundo->heroes[idx_heroe];
    if(!H->vivo) return;

    int objetivo = -1;

    for(int m = 0; m < mundo->cantidad_monstruos; m++) {
        Monstruo *MM = &mundo->monstruos[m];

        if(!MM->vivo) {
            continue;
        }
        if(in_range(H->pos, MM->pos, H->rango_ataque)) {
            objetivo = m;
            break;
        }
    }

    if(objetivo >= 0) {
        Monstruo *M = &mundo->monstruos[objetivo];
        M->hp -= H->danio_ataque;

        printf("  [tick %ld] Héroe%d golpea a Monstruo%d: -%d vida (Monstruo%d vida=%d)\n",
               g_tick, idx_heroe, objetivo+1, H->danio_ataque, objetivo+1, (M->hp>0?M->hp:0));

        if(M->hp <= 0) {
            M->vivo = false; 
            printf("  [tick %ld] Monstruo%d muere\n", g_tick, objetivo+1);
        }
        return; 
    }

    /**********************AVANZAAAA EL PATH**********************/
    if(H->indice_ruta < H->largo_ruta) {
        H->pos = H->ruta[H->indice_ruta++];
        printf("  [tick %ld] Héroe%d avanza a (%d,%d)\n", g_tick, idx_heroe, H->pos.x, H->pos.y);
    }
}

void paso_monstruo(Mundo *mundo, int idx_monstruo){
    Monstruo *M = &mundo->monstruos[idx_monstruo];
    if(!M->vivo) {
        return;
    }

    int objetivo=-1, mejor_d=1000000000;

    for(int h = 0; h < mundo->cantidad_heroes; h++) {
        Heroe *H = &mundo->heroes[h];
        if(!H->vivo) {
            continue;
        }
        int d = manhattan(M->pos, H->pos);
        
        if(d < mejor_d) {
            mejor_d = d; 
            objetivo = h;
        }
    }

    if(objetivo < 0) {
        return;
    }

    Heroe *H = &mundo->heroes[objetivo];

    /**********************ALERTAAAAAAAAAAAAAAAAAAAAA**********************/
    if(!M->despierto && in_range(M->pos, H->pos, M->rango_vision)) {
        M->despierto = true;
        printf("  [tick %ld] Monstruo%d se despierta (ve al héroe)\n", g_tick, idx_monstruo+1);
        alertar_cercanos(mundo, idx_monstruo);
    }

    if(!M->despierto) {
        return;
    }

    /**********************ATAQUEEEEEEEEEEEEEEEEEEEE**********************/
    if(in_range(M->pos, H->pos, M->rango_ataque)) {

        H->hp -= M->danio_ataque;

        printf("  [tick %ld] Monstruo%d golpea a Héroe%d: -%d vida (Héroe%d vida=%d)\n",
               g_tick, idx_monstruo+1, objetivo, M->danio_ataque, objetivo, (H->hp>0?H->hp:0));

        if(H->hp <= 0){
            H->vivo = false; 
            printf("  [tick %ld] Héroe%d muere\n", g_tick, objetivo);
        }
        return;
    }

    Coordenada anterior = M->pos;

    if(M->pos.x != H->pos.x) {
        M->pos.x += (H->pos.x > M->pos.x) ? 1 : -1;
    }
    else if(M->pos.y != H->pos.y) {
        M->pos.y += (H->pos.y > M->pos.y) ? 1 : -1;
    }

    if(anterior.x != M->pos.x || anterior.y != M->pos.y){
        printf("  [tick %ld] Monstruo%d avanza a (%d,%d)\n", g_tick, idx_monstruo+1, M->pos.x, M->pos.y);
    }

    
}

void liberar_mundo(Mundo *mundo) {
    if(!mundo) {
        return;
    }

    for(int i = 0; i < mundo->cantidad_heroes; i++) {
        free(mundo->heroes[i].ruta);
    }
    free(mundo->heroes);
    free(mundo->monstruos);
}