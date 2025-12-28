/*
typedef struct {
    int id;
    FemaleApe* female;
    MaleApe* male;
    BabyApe* babies;
    int num_babies;
    bool withdrawn;
    pthread_mutex_t basket_mutex;
} Family;

// Functions:
- init_families()
- withdraw_family()
- get_family_total_bananas()
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "family.h"
#include "config.h"

Family *families = NULL;
int total_families = 0;

void init_families(void)
{
    total_families = config.num_families;
    families = (Family *)malloc(total_families * sizeof(Family));
    
    for (int i = 0; i < total_families; i++) {
        families[i].id = i;
        families[i].basket_bananas = 0;
        families[i].withdrawn = false;
        families[i].num_babies = config.babies_per_family;
        
        pthread_mutex_init(&families[i].basket_mutex, NULL);
        pthread_mutex_init(&families[i].fight_mutex, NULL);
        
        families[i].female = (FemaleApe *)malloc(sizeof(FemaleApe));
        families[i].female->id = i;
        families[i].female->family_id = i;
        families[i].female->energy = config.female_initial_energy;
        families[i].female->bananas_collected = 0;
        families[i].female->in_maze = false;
        families[i].female->resting = false;
        families[i].female->active = true;
        families[i].female->position_row = 0;
        families[i].female->position_col = 0;
        
        families[i].male = (MaleApe *)malloc(sizeof(MaleApe));
        families[i].male->id = i;
        families[i].male->family_id = i;
        families[i].male->energy = config.male_initial_energy;
        families[i].male->fighting = false;
        families[i].male->active = true;
        
        families[i].babies = (BabyApe *)malloc(config.babies_per_family * sizeof(BabyApe));
        for (int j = 0; j < config.babies_per_family; j++) {
            families[i].babies[j].id = j;
            families[i].babies[j].family_id = i;
            families[i].babies[j].bananas_eaten = 0;
            families[i].babies[j].active = true;
        }
    }
    
    printf("=== Families Initialized ===\n");
    printf("Total Families: %d\n", total_families);
    printf("Babies per Family: %d\n", config.babies_per_family);
    printf("============================\n\n");
}

void cleanup_families(void)
{
    for (int i = 0; i < total_families; i++) {
        pthread_mutex_destroy(&families[i].basket_mutex);
        pthread_mutex_destroy(&families[i].fight_mutex);
        
        free(families[i].female);
        free(families[i].male);
        free(families[i].babies);
    }
    
    free(families);
    printf("Families cleanup completed.\n");
}

void withdraw_family(int family_id)
{
    if (family_id < 0 || family_id >= total_families)
        return;
    
    Family *fam = &families[family_id];
    
    pthread_mutex_lock(&fam->basket_mutex);
    
    if (!fam->withdrawn) {
        fam->withdrawn = true;
        fam->female->active = false;
        fam->male->active = false;
        
        for (int i = 0; i < fam->num_babies; i++) {
            fam->babies[i].active = false;
        }
        
        printf("\n*** Family %d WITHDRAWN with %d bananas ***\n", 
               family_id, fam->basket_bananas);
    }
    
    pthread_mutex_unlock(&fam->basket_mutex);
}

int get_family_total_bananas(int family_id)
{
    if (family_id < 0 || family_id >= total_families)
        return 0;
    
    pthread_mutex_lock(&families[family_id].basket_mutex);
    int total = families[family_id].basket_bananas;
    pthread_mutex_unlock(&families[family_id].basket_mutex);
    
    return total;
}

void add_to_basket(int family_id, int bananas)
{
    if (family_id < 0 || family_id >= total_families || bananas <= 0)
        return;
    
    pthread_mutex_lock(&families[family_id].basket_mutex);
    families[family_id].basket_bananas += bananas;
    pthread_mutex_unlock(&families[family_id].basket_mutex);
}

int steal_from_basket(int family_id, int amount)
{
    if (family_id < 0 || family_id >= total_families)
        return 0;
    
    pthread_mutex_lock(&families[family_id].basket_mutex);
    
    int stolen = 0;
    if (families[family_id].basket_bananas > 0) {
        stolen = (amount < families[family_id].basket_bananas) ? 
                 amount : families[family_id].basket_bananas;
        families[family_id].basket_bananas -= stolen;
    }
    
    pthread_mutex_unlock(&families[family_id].basket_mutex);
    
    return stolen;
}

void print_family_stats(void)
{
    printf("\n========================================\n");
    printf("        FAMILY STATISTICS\n");
    printf("========================================\n\n");
    
    int withdrawn_count = 0;
    int total_bananas = 0;
    
    for (int i = 0; i < total_families; i++) {
        Family *fam = &families[i];
        
        pthread_mutex_lock(&fam->basket_mutex);
        
        printf("Family %d: ", i);
        if (fam->withdrawn) {
            printf("WITHDRAWN");
            withdrawn_count++;
        } else {
            printf("ACTIVE");
        }
        printf(" | Basket: %d bananas | Female Energy: %d | Male Energy: %d\n",
               fam->basket_bananas, fam->female->energy, fam->male->energy);
        
        total_bananas += fam->basket_bananas;
        
        for (int j = 0; j < fam->num_babies; j++) {
            printf("  Baby %d: Eaten %d bananas\n", 
                   j, fam->babies[j].bananas_eaten);
        }
        
        pthread_mutex_unlock(&fam->basket_mutex);
    }
    
    printf("\n----------------------------------------\n");
    printf("Total Active Families: %d\n", total_families - withdrawn_count);
    printf("Total Withdrawn Families: %d\n", withdrawn_count);
    printf("Total Bananas Collected: %d\n", total_bananas);
    printf("========================================\n\n");
}