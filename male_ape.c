#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include "family.h"
#include "config.h"
#include "simulation.h"
#include "utils.h"

/*
Male Ape responsibilities (high level):
- Protect family basket
- Occasionally fight a neighbor
- Lose energy over time / fighting
- Withdraw family if energy too low
*/

static inline int clamp_int(int x, int lo, int hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static inline float clamp_float(float x, float lo, float hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

void* male_ape_thread(void *arg)
{
    MaleApe *male = (MaleApe *)arg;
    if (!male) return NULL;

    const int family_id = male->family_id;

    log_event("Male %d (Family %d) started protecting basket", male->id, family_id);

    while (is_simulation_running() && male->active) {

        // Stop quickly if family already withdrawn
        if (families[family_id].withdrawn) {
            break;
        }

        // Withdraw condition
        if (male->energy < config.male_fight_energy_threshold) {
            log_event("Male %d energy too low (%d) - Family %d WITHDRAWING",
                      male->id, male->energy, family_id);
            withdraw_family(family_id);
            break;
        }

        // Read current bananas (assumed thread-safe inside helper)
        int my_bananas = get_family_total_bananas(family_id);

        // Fight probability grows with bananas
        float fight_prob = config.base_fight_probability +
                           (my_bananas * config.banana_fight_factor);
        fight_prob = clamp_float(fight_prob, 0.0f, 0.9f);

        // Only attempt fight if we have bananas to lose/win
        float roll = random_float(0.0f, 1.0f);
        if (roll < fight_prob && my_bananas > 0) {

            int opponent_id = random_int(0, total_families - 1);

            if (opponent_id != family_id &&
                opponent_id >= 0 && opponent_id < total_families &&
                !families[opponent_id].withdrawn &&
                families[opponent_id].male != NULL &&
                families[opponent_id].male->active) {

                int opponent_bananas = get_family_total_bananas(opponent_id);

                // sometimes fight even if opponent has 0 bananas
                if (opponent_bananas > 0 || random_float(0.0f, 1.0f) < 0.3f) {

                    // Lock ordering to avoid deadlocks:
                    // always lock smaller fight_mutex first
                    int first = family_id < opponent_id ? family_id : opponent_id;
                    int second = family_id < opponent_id ? opponent_id : family_id;

                    pthread_mutex_lock(&families[first].fight_mutex);
                    pthread_mutex_lock(&families[second].fight_mutex);

                    // Re-check after locking (state may change)
                    if (!families[family_id].withdrawn &&
                        !families[opponent_id].withdrawn &&
                        families[opponent_id].male != NULL &&
                        families[opponent_id].male->active) {

                        male->fighting = true;
                        families[opponent_id].male->fighting = true;

                        // Update bananas again (fresh values)
                        my_bananas = get_family_total_bananas(family_id);
                        opponent_bananas = get_family_total_bananas(opponent_id);

                        log_event("FIGHT: Male %d (Family %d, %d bananas) vs Male %d (Family %d, %d bananas)",
                                  male->id, family_id, my_bananas,
                                  families[opponent_id].male->id, opponent_id, opponent_bananas);

                        // Fight duration (babies might steal during this time)
                        sleep_seconds(2);

                        // Decide winner (energy + randomness)
                        int my_score = male->energy + random_int(0, 20);
                        int opp_score = families[opponent_id].male->energy + random_int(0, 20);
                        bool i_win = (my_score >= opp_score);

                        if (i_win) {
                            // Winner takes ALL from loser (snapshot amount)
                            int stolen = 0;
                            if (opponent_bananas > 0) {
                                stolen = steal_from_basket(opponent_id, opponent_bananas);
                            }
                            if (stolen > 0) {
                                add_to_basket(family_id, stolen);
                                log_event("Male %d WON and took ALL %d bananas from Family %d",
                                          male->id, stolen, opponent_id);
                            }
                            male->energy -= 10;
                            families[opponent_id].male->energy -= 20;
                        } else {
                            // Loser gives ALL to winner (snapshot amount)
                            int given = 0;
                            if (my_bananas > 0) {
                                given = steal_from_basket(family_id, my_bananas);
                            }
                            if (given > 0) {
                                add_to_basket(opponent_id, given);
                                log_event("Male %d LOST and gave ALL %d bananas to Family %d",
                                          male->id, given, opponent_id);
                            }
                            male->energy -= 20;
                            families[opponent_id].male->energy -= 10;
                        }

                        male->energy = clamp_int(male->energy, 0, 1000000);
                        families[opponent_id].male->energy =
                            clamp_int(families[opponent_id].male->energy, 0, 1000000);

                        male->fighting = false;
                        families[opponent_id].male->fighting = false;
                    }

                    pthread_mutex_unlock(&families[second].fight_mutex);
                    pthread_mutex_unlock(&families[first].fight_mutex);
                }
            }
        }

        // Passive energy drain
        male->energy -= 1;
        if (male->energy < 0) male->energy = 0;

        sleep_seconds(2);
    }

    log_event("Male %d (Family %d) stopped", male->id, family_id);
    return NULL;
}
