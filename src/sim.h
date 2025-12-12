#pragma once
#include <pthread.h>
#include <stdbool.h>


typedef struct { int x, y; } Coordenada;

typedef struct {
    int ancho, alto;
} Grilla;

typedef struct {
    int hp, danio_ataque, rango_ataque;
    Coordenada pos;
    Coordenada *ruta; 
    int largo_ruta; 
    int indice_ruta;
    bool vivo;
} Heroe;

typedef struct {
    int hp, danio_ataque, rango_vision, rango_ataque;
    Coordenada pos;
    bool despierto, vivo;
} Monstruo;

typedef struct Mundo {
    Grilla grilla;
    int cantidad_heroes;
    Heroe *heroes;
    int cantidad_monstruos;
    Monstruo *monstruos;
} Mundo;


extern pthread_mutex_t g_bloqueo;
extern pthread_cond_t  g_tick_cv;
extern long g_tick;

extern Mundo *g_mundo;
extern int g_finalizado;
extern int g_hilos_totales;


typedef enum {
    FIN_NINGUNO = 0,
    FIN_HEROES_MUERTOS = 1,
    FIN_HEROES_COMPLETARON = 2
} MotivoFinal;

extern MotivoFinal g_motivo_final;


int manhattan(Coordenada a, Coordenada b);
static inline bool in_range(Coordenada a, Coordenada b, int r){ return manhattan(a,b) <= r; }


void paso_heroe(Mundo *m, int indice_heroe);
void paso_monstruo(Mundo *m, int indice_monstruo);
void barrera_inicio(void);
void barrera_fin(void);


int parsear_config(const char *ruta, Mundo *m);
void liberar_mundo(Mundo *m);