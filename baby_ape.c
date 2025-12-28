/*
void* baby_ape_thread(void* arg);
// Logic:
- Watch for male fights
- Steal bananas during fights
- Eat or add to family basket
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "family.h"
#include "config.h"
#include "simulation.h"
#include "utils.h"

void* baby_ape_thread(void *arg)
{
    BabyApe *baby = (BabyApe *)arg;
    int family_id = baby->family_id;
    
    log_event("Baby %d (Family %d) started", baby->id, family_id);
     while (is_simulation_running() && baby->active) {
        
        if (families[family_id].withdrawn) {
            break;
        }
        
        if (baby->bananas_eaten >= config.max_baby_eaten) {
            log_event("Baby %d (Family %d) is full (eaten %d bananas)", 
                     baby->id, family_id, baby->bananas_eaten);
            break;
        }
        
        bool found_fight = false;
        for (int i = 0; i < total_families; i++) {
            if (families[i].withdrawn) continue;
            
            if (families[i].male->fighting && i != family_id) {
                
                int stolen = steal_from_basket(i, config.baby_eat_rate);
                
                if (stolen > 0) {
                    found_fight = true;
                    
                    if (rand() % 100 < 50) {
                        baby->bananas_eaten += stolen;
                        log_event("Baby %d (Family %d) STOLE and ATE %d bananas from Family %d (Total eaten: %d)",
                               baby->id, family_id, stolen, i, baby->bananas_eaten);
                    } else {
                        add_to_basket(family_id, stolen);
                        log_event("Baby %d (Family %d) STOLE %d bananas from Family %d and gave to dad",
                                 baby->id, family_id, stolen, i);
                    }
                     break;
                }
            }
        }
        
        if (found_fight) {
            sleep_seconds(1);
        } else {
            sleep_seconds(2);
        }
    }
    
    log_event("Baby %d (Family %d) stopped (Total eaten: %d)", 
             baby->id, family_id, baby->bananas_eaten);
    return NULL;
}
