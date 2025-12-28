/*
void* male_ape_thread(void* arg);
// Logic:
- Protect basket
- Fight with neighbors
- Check energy level
- Withdraw family if tired
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "family.h"
#include "config.h"
#include "simulation.h"
#include "utils.h"

void* male_ape_thread(void *arg)
{
    MaleApe *male = (MaleApe *)arg;
    int family_id = male->family_id;
    log_event("Male %d (Family %d) started protecting basket", male->id, family_id);
    
    while (is_simulation_running() && male->active) {
        
        if (families[family_id].withdrawn) {
            break;
        }
        
        // Check energy level - if too low, family withdraws
        if (male->energy < config.male_fight_energy_threshold) {
            log_event("Male %d energy too low (%d) - Family %d WITHDRAWING", 
                     male->id, male->energy, family_id);
            withdraw_family(family_id);
            break;
        }
        
        int my_bananas = get_family_total_bananas(family_id);
        
        // Fight probability increases with more bananas in basket
        float fight_prob = config.base_fight_probability + 
                          (my_bananas * config.banana_fight_factor);
        
        if (fight_prob > 0.9f) fight_prob = 0.9f;
        
        float roll = random_float(0.0f, 1.0f);
        
        if (roll < fight_prob && my_bananas > 0) {
            // Find a neighboring male ape to fight (random selection simulates "neighbors")
            int opponent_id = random_int(0, total_families - 1);
            
            if (opponent_id != family_id && !families[opponent_id].withdrawn) {
                
                int opponent_bananas = get_family_total_bananas(opponent_id);
                
                // More likely to fight if opponent also has bananas
                if (opponent_bananas > 0 || random_float(0.0f, 1.0f) < 0.3f) {
                    
                    pthread_mutex_lock(&families[family_id].fight_mutex);
                    
                    if (pthread_mutex_trylock(&families[opponent_id].fight_mutex) == 0) {
                        
                        male->fighting = true;
                        families[opponent_id].male->fighting = true;
                        
                        log_event("FIGHT: Male %d (Family %d, %d bananas) vs Male %d (Family %d, %d bananas)", 
                                 male->id, family_id, my_bananas,
                                 families[opponent_id].male->id, opponent_id, opponent_bananas);
                        
                        // Fight duration - gives babies time to steal
                        sleep_seconds(2);
                        
                        // Determine winner (energy + some randomness)
                        bool i_win = (male->energy + random_int(0, 20) > 
                                     families[opponent_id].male->energy + random_int(0, 20));
                        
                        if (i_win) {
                            // Winner gets ALL bananas from loser
                            int stolen = steal_from_basket(opponent_id, opponent_bananas);
                            if (stolen > 0) {
                                add_to_basket(family_id, stolen);
                                log_event("Male %d WON and took ALL %d bananas from Family %d", 
                                         male->id, stolen, opponent_id);
                            }
                            male->energy -= 10;
                            families[opponent_id].male->energy -= 20;
                        } else {
                            // Loser gives ALL bananas to winner
                            int stolen = steal_from_basket(family_id, my_bananas);
                            if (stolen > 0) {
                                add_to_basket(opponent_id, stolen);
                                log_event("Male %d LOST and gave ALL %d bananas to Family %d", 
                                         male->id, stolen, opponent_id);
                            }
                            male->energy -= 20;
                            families[opponent_id].male->energy -= 10;
                        }
                        
                        male->fighting = false;
                        families[opponent_id].male->fighting = false;
                        
                        pthread_mutex_unlock(&families[opponent_id].fight_mutex);
                    }
                    
                    pthread_mutex_unlock(&families[family_id].fight_mutex);
                }
            }
        }
        
        // Male gets tired from protecting basket
        male->energy -= 1;
        
        sleep_seconds(2);
    }
    
    log_event("Male %d (Family %d) stopped", male->id, family_id);
    return NULL;
}