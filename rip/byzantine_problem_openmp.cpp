#include <iostream>
#include <omp.h>
#include <cstdlib>
#include <vector>
#include <map>
#include <time.h>

namespace multi_core {
    const int num_lieutenants = 6;

    typedef enum { ATTACK, RETREAT, UNKNOWN } Action;

    typedef struct {
        int id;
        bool is_loyal;
        Action proposed_action;
    } General;

    // Message structure for multi-round communication
    typedef struct {
        int sender_id;
        Action order;
        std::vector<int> path; // Who relayed this message
    } Message;

    class ByzantineConsensus {
    private:
        General* commander;
        std::vector<General*> lieutenants;
        int num_generals;
        int max_traitors;

        // Thread-safe message storage for each round
        std::vector<std::map<int, std::vector<Message>>> message_rounds;

    public:
        ByzantineConsensus(General* cmd, std::vector<General*> lts, int m)
            : commander(cmd), lieutenants(lts), max_traitors(m) {
            num_generals = lieutenants.size();
            message_rounds.resize(m + 1);
        }

        Action send_message(General* sender, Action order) {
            if (sender->is_loyal) {
                return order;
            }
            else {
                // Traitor sends conflicting messages
                return (rand() % 2 == 0) ? ATTACK : RETREAT;
            }
        }

        // Round 0: Commander sends to all lieutenants
        void round_zero() {
            Action commander_order = commander->proposed_action;

#pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < num_generals; i++) {
                Message msg;
                msg.sender_id = commander->id;
                msg.order = send_message(commander, commander_order);
                msg.path.push_back(commander->id);

#pragma omp critical
                {
                    message_rounds[0][i].push_back(msg);
                }

                lieutenants[i]->proposed_action = msg.order;
            }
        }

        // Subsequent rounds: Lieutenants relay messages to each other
        void relay_round(int round) {
            if (round > max_traitors) return;

            // Each lieutenant relays what they received to others
#pragma omp parallel for schedule(dynamic) collapse(2)
            for (int i = 0; i < num_generals; i++) {
                for (int j = 0; j < num_generals; j++) {
                    if (i == j) continue; // Don't send to yourself

                    // Get messages from previous round
                    std::vector<Message> prev_messages;
#pragma omp critical
                    {
                        if (message_rounds[round - 1].find(i) != message_rounds[round - 1].end()) {
                            prev_messages = message_rounds[round - 1][i];
                        }
                    }

                    // Relay each message
                    for (const auto& prev_msg : prev_messages) {
                        // Avoid cycles in message path
                        bool already_in_path = false;
                        for (int node : prev_msg.path) {
                            if (node == lieutenants[j]->id) {
                                already_in_path = true;
                                break;
                            }
                        }

                        if (!already_in_path) {
                            Message new_msg;
                            new_msg.sender_id = lieutenants[i]->id;
                            new_msg.order = send_message(lieutenants[i], prev_msg.order);
                            new_msg.path = prev_msg.path;
                            new_msg.path.push_back(lieutenants[i]->id);

#pragma omp critical
                            {
                                message_rounds[round][j].push_back(new_msg);
                            }
                        }
                    }
                }
            }
        }

        // Majority voting with all received messages
        Action compute_consensus(int lieutenant_idx) {
            std::map<Action, int> vote_count;
            vote_count[ATTACK] = 0;
            vote_count[RETREAT] = 0;

            // Count votes from all rounds
            for (int round = 0; round <= max_traitors; round++) {
                if (message_rounds[round].find(lieutenant_idx) != message_rounds[round].end()) {
                    for (const auto& msg : message_rounds[round][lieutenant_idx]) {
                        vote_count[msg.order]++;
                    }
                }
            }

            // Majority decision (default to RETREAT on tie)
            return (vote_count[ATTACK] > vote_count[RETREAT]) ? ATTACK : RETREAT;
        }

        Action run_protocol() {
            // Round 0: Commander broadcasts
            round_zero();

            // Additional rounds for fault tolerance
            for (int round = 1; round <= max_traitors && round <= 2; round++) {
                relay_round(round);
            }

            // Each lieutenant computes consensus
            std::vector<Action> decisions(num_generals);

#pragma omp parallel for
            for (int i = 0; i < num_generals; i++) {
                decisions[i] = compute_consensus(i);
            }

            // Final majority vote among loyal lieutenants
            int attack_count = 0;
            int retreat_count = 0;

            for (int i = 0; i < num_generals; i++) {
                if (lieutenants[i]->is_loyal) {
                    if (decisions[i] == ATTACK) attack_count++;
                    else retreat_count++;
                }
            }

            return (attack_count > retreat_count) ? ATTACK : RETREAT;
        }

        void print_status() {
            std::cout << "Commander (ID " << commander->id << "): "
                << (commander->is_loyal ? "Loyal" : "Traitor")
                << ", Order: " << (commander->proposed_action == ATTACK ? "ATTACK" : "RETREAT")
                << std::endl;

            for (int i = 0; i < num_generals; i++) {
                std::cout << "Lieutenant " << lieutenants[i]->id << ": "
                    << (lieutenants[i]->is_loyal ? "Loyal" : "Traitor") << std::endl;
            }
        }
    };
}

//using namespace multi_core;
//
//int main() {
//    srand(static_cast<unsigned int>(time(0)));
//    omp_set_num_threads(4); // Use 4 threads
//
//    General* commander = new General();
//    commander->id = 0;
//    commander->is_loyal = true;
//    commander->proposed_action = Action::ATTACK;
//
//    std::vector<General*> lieutenants;
//    int traitor_count = 0;
//
//    for (int i = 0; i < num_lieutenants; i++) {
//        General* lt = new General();
//        lt->id = i + 1;
//        lt->is_loyal = (rand() % 10) > 2; // 70% chance of being loyal
//        lt->proposed_action = UNKNOWN;
//
//        if (!lt->is_loyal) traitor_count++;
//        lieutenants.push_back(lt);
//    }
//
//    std::cout << "=== Byzantine Generals Problem ===" << std::endl;
//    std::cout << "Number of traitors: " << traitor_count << std::endl << std::endl;
//
//    ByzantineConsensus consensus(commander, lieutenants, std::min(2, traitor_count));
//    consensus.print_status();
//
//    std::cout << "\nRunning consensus protocol..." << std::endl;
//    double start_time = omp_get_wtime();
//
//    Action final_decision = consensus.run_protocol();
//
//    double end_time = omp_get_wtime();
//
//    std::cout << "\nFinal Decision: " << (final_decision == ATTACK ? "ATTACK" : "RETREAT") << std::endl;
//    std::cout << "Time taken: " << (end_time - start_time) * 1000 << " ms" << std::endl;
//
//    // Cleanup
//    delete commander;
//    for (auto lt : lieutenants) {
//        delete lt;
//    }
//
//    return 0;
//}