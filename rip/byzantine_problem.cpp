#include<iostream>
#include <omp.h>
#include <cstdlib>
#include <time.h>

namespace multi_core {
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
using namespace multi_core;
int main() {
    srand(static_cast<unsigned int>(time(0)));

    General* commander = new General();
    commander->id = 0;
    commander->is_loyal = false;
    commander->proposed_action = Action::ATTACK;

    General** lieutenants = new General * [num_lieutenants];
    int m_traitors = 0;
    for (int i = 0; i < num_lieutenants; i++) {
        lieutenants[i] = new General();
        lieutenants[i]->id = i + 1;
        lieutenants[i]->is_loyal = (rand() % 10) > 3;
        if (!lieutenants[i]->is_loyal) {
            m_traitors++;
        }
        std::cout << "ID: " << lieutenants[i]->id << std::endl;
        std::cout << "Is Loyal: " << lieutenants[i]->is_loyal << std::endl;
    }
    Action action = om_algorithm(commander, lieutenants, num_lieutenants, m_traitors);
    std::cout << action << std::endl;
    for (int i = 0; i < num_lieutenants; i++) {
        std::cout << "ID: " << lieutenants[i]->id << std::endl;
        std::cout << "Is Loyal: " << lieutenants[i]->is_loyal << std::endl;
        std::cout << "Action: " << lieutenants[i]->proposed_action << std::endl;
    }
    return 0;
}