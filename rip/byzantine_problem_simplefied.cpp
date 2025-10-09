#include<iostream>
#include <omp.h>
#include <cstdlib>
#include <time.h>

namespace multi_core_simplefied {
    const int num_lieutenants = 5;
    typedef enum { ATTACK, RETREAT, UNKNOWN } Action;

    typedef struct {
        int id;
        bool is_loyal;
        Action proposed_action;
    } General;

    void send_message(General* sender, General* receiver, Action order) {
        if (sender->is_loyal) {
            receiver->proposed_action = order;
        }
        else {
            receiver->proposed_action = (order == ATTACK) ? RETREAT : ATTACK;
        }
    }

    Action om_algorithm(General* commander, General* lieutenants[], int num_lieutenants, int m_traitors) {
        if (m_traitors == 0) {
            return commander->proposed_action;
        }

#pragma omp parallel for
        for (int i = 0; i < num_lieutenants; i++) {
            send_message(commander, lieutenants[i], commander->proposed_action);
        }

        int attack_votes = 0;
        int retreat_votes = 0;

#pragma omp parallel for
        for (int i = 0; i < num_lieutenants; i++) {
            if (lieutenants[i]->proposed_action == ATTACK) {
#pragma omp atomic
                attack_votes++;
            }
            else if (lieutenants[i]->proposed_action == RETREAT) {
#pragma omp atomic
                retreat_votes++;
            }
        }

        if (attack_votes > retreat_votes) {
            return ATTACK;
        }
        else if (retreat_votes > attack_votes) {
            return RETREAT;
        }
        else {
            return RETREAT;
        }
    }
}