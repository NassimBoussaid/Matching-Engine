#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H

#include "Order.h"
#include "OrderBook.h"
#include <unordered_map>
#include <vector>
#include <algorithm>

// Moteur de matching des ordres
class MatchingEngine {
private:
    // Carnets d'ordres par instrument (ex : AAPL, EURUSD, etc.)
    std::unordered_map<std::string, OrderBook> order_books;
    
public:
    // Constructeur
    MatchingEngine() {
        // Réinitialise le compteur global de timestamps
        OrderBook::reset_global_counter();
    }
    
    // Traite un ordre (NEW, MODIFY, CANCEL)
    void process_order(const Order& order);
    
    // Récupère tous les résultats
    std::vector<Order> get_all_results();
    
    // Efface les résultats
    void clear_results();
};

// Implémentation

void MatchingEngine::process_order(const Order& order) {
    // Ignore les ordres rejetés
    if (order.status == "REJECTED") {
        order_books[order.instrument].results.push_back(order);
        return;
    }
    
    // Récupère le carnet d'ordres de l'instrument
    OrderBook& book = order_books[order.instrument];
    
    // Traite l'action de l'ordre
    if (order.action == "NEW") {
        book.add_order(order);
    } else if (order.action == "MODIFY") {
        book.modify_order(order);
    } else if (order.action == "CANCEL") {
        book.cancel_order(order);
    }
}

std::vector<Order> MatchingEngine::get_all_results() {
    std::vector<Order> all_results;
    
    // Rassemble les résultats de tous les carnets
    for (auto& [instrument, book] : order_books) {
        for (const auto& result : book.results) {
            all_results.push_back(result);
        }
    }
    
    // Trie les résultats par timestamp
    std::stable_sort(all_results.begin(), all_results.end(),
              [](const Order& a, const Order& b) {
                  if (a.timestamp == b.timestamp) {
                      // Si timestamps identiques, conserve l'ordre d'apparition
                      return false;
                  }
                  return a.timestamp < b.timestamp;
              });
    
    return all_results;
}

void MatchingEngine::clear_results() {
    // Efface les résultats dans chaque carnet
    for (auto& [instrument, book] : order_books) {
        book.results.clear();
    }
}

#endif // MATCHING_ENGINE_H