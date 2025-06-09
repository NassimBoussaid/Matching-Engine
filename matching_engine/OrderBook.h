#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "Order.h"
#include <map>
#include <queue>
#include <unordered_map>
#include <vector>
#include <algorithm>

// File d'attente d'ordres pour une même limite de prix
struct OrderQueue {
    std::queue<Order> orders;
    uint64_t total_quantity;

    OrderQueue() : total_quantity(0) {}

    void add_order(const Order& order) {
        orders.push(order);
        total_quantity += order.quantity;
    }

    bool empty() const {
        return orders.empty();
    }

    Order& front() {
        return orders.front();
    }

    void pop() {
        if (!orders.empty()) {
            total_quantity -= orders.front().quantity;
            orders.pop();
        }
    }

    void update_quantity(uint64_t executed_qty) {
        total_quantity -= executed_qty;
    }
};

// Carnet d'ordres pour un instrument donné
class OrderBook {
private:
    // Carnet d'ordres BUY : trié par prix décroissant
    std::map<double, OrderQueue, std::greater<double>> buy_orders;
    // Carnet d'ordres SELL : trié par prix croissant
    std::map<double, OrderQueue> sell_orders;
    // Accès rapide aux ordres par ID
    std::unordered_map<uint64_t, Order> order_lookup;
    // Suivi des IDs d'ordres existants
    std::unordered_map<uint64_t, bool> existing_order_ids;
    // Suivi des quantités exécutées par ordre (utile pour MODIFY)
    std::unordered_map<uint64_t, uint64_t> order_total_executed;
    // Horodatage global pour les exécutions
    static uint64_t global_timestamp_counter;

public:
    std::vector<Order> results;
    // Résultats des traitements d'ordres
    OrderBook() {}

    static void reset_global_counter() { global_timestamp_counter = 0; }

    void add_order(Order order);
    void modify_order(const Order& modify_request);
    void cancel_order(const Order& cancel_request);

private:
    void execute_market_order(Order order);
    void execute_buy_market_order(Order order);
    void execute_sell_market_order(Order order);
    void execute_limit_order(Order order);
    void execute_buy_limit_order(Order order);
    void execute_sell_limit_order(Order order);
    void cancel_order_from_book(const Order& order);
    void record_execution(const Order& order, uint64_t executed_qty);
    uint64_t get_next_execution_timestamp(uint64_t base_timestamp);
};

// Implémentation du compteur global
uint64_t OrderBook::global_timestamp_counter = 0;
// Calcule le prochain timestamp pour une exécution
uint64_t OrderBook::get_next_execution_timestamp(uint64_t base_timestamp) {
    static uint64_t last_execution_timestamp = 0;
    uint64_t next_timestamp = std::max(base_timestamp, last_execution_timestamp + 100);
    last_execution_timestamp = next_timestamp;
    return next_timestamp;
}

// Ajoute un ordre dans le carnet
void OrderBook::add_order(Order order) {
    // Vérifie si l'ID existe déjà
    if (order.action == "NEW" && existing_order_ids.find(order.order_id) != existing_order_ids.end()) {
        order.status = "REJECTED";
        order.executed_quantity = 0;
        order.execution_price = 0.0;
        order.counterparty_id = 0;
        results.push_back(order);
        return;
    }

    // Marque l'ID comme existant pour les NEW
    if (order.action == "NEW") {
        existing_order_ids[order.order_id] = true;
        order_total_executed[order.order_id] = 0;
    }

    // Initialise l'état de l'ordre
    order.status = "PENDING";
    order.executed_quantity = 0;
    order.execution_price = 0.0;
    order.counterparty_id = 0;

    // Sauvegarde de l'ordre
    order_lookup[order.order_id] = order;

    // Exécute selon le type d'ordre
    if (order.type == "MARKET") {
        execute_market_order(order);
    }
    else if (order.type == "LIMIT") {
        execute_limit_order(order);
    }
}

// Modifie un ordre existant
void OrderBook::modify_order(const Order& modify_request) {
    auto it = order_lookup.find(modify_request.order_id);
    // Si l'ordre n'existe pas, rejeté
    if (it == order_lookup.end()) {
        Order rejected = modify_request;
        rejected.status = "REJECTED";
        results.push_back(rejected);
        return;
    }

    // Retire l'ancien ordre du carnet
    cancel_order_from_book(it->second);

    // Récupère le total exécuté jusque-là
    uint64_t total_executed = order_total_executed[modify_request.order_id];

    // Quantité restante après modification
    uint64_t new_total_quantity = modify_request.quantity;
    uint64_t remaining_quantity = (new_total_quantity > total_executed) ?
        (new_total_quantity - total_executed) : 0;

    // Prépare un ordre temporaire pour traitement
    Order processing_order = it->second;
    processing_order.quantity = remaining_quantity;
    processing_order.price = modify_request.price;
    processing_order.timestamp = modify_request.timestamp;
    processing_order.action = "MODIFY";

    // Met à jour l'ordre d'origine
    it->second.quantity = modify_request.quantity;
    it->second.price = modify_request.price;
    order_lookup[modify_request.order_id] = it->second;

    // Si il reste des quantités : on le traite
    if (remaining_quantity > 0) {
        if (processing_order.type == "MARKET") {
            execute_market_order(processing_order);
        }
        else if (processing_order.type == "LIMIT") {
            execute_limit_order(processing_order);
        }
    }
    else {
        // Sinon on le marque comme EXECUTED
        Order result_order = order_lookup[modify_request.order_id];
        result_order.timestamp = get_next_execution_timestamp(modify_request.timestamp);
        result_order.action = "MODIFY";
        result_order.status = "EXECUTED";
        result_order.executed_quantity = 0;
        result_order.execution_price = 0.0;
        result_order.counterparty_id = 0;
        results.push_back(result_order);
    }
}

// Annule un ordre existant
void OrderBook::cancel_order(const Order& cancel_request) {
    auto it = order_lookup.find(cancel_request.order_id);
    // Si l'ordre n'existe pas : rejeté
    if (it == order_lookup.end()) {
        Order rejected = cancel_request;
        rejected.status = "REJECTED";
        results.push_back(rejected);
        return;
    }

    // Retire l'ordre du carnet
    cancel_order_from_book(it->second);

    // Prépare l'ordre CANCEL pour la sortie
    Order cancelled = it->second;
    cancelled.timestamp = get_next_execution_timestamp(cancel_request.timestamp);
    cancelled.action = "CANCEL";
    cancelled.status = "CANCELED";
    cancelled.quantity = 0;
    cancelled.price = cancel_request.price; // Use price from cancel request
    cancelled.executed_quantity = 0;
    cancelled.execution_price = 0.0;
    cancelled.counterparty_id = 0;
    results.push_back(cancelled);

    // Retire l'ordre du lookup
    order_lookup.erase(it);
}

// Met à jour la quantité exécutée pour un ordre
void OrderBook::record_execution(const Order& order, uint64_t executed_qty) {
    order_total_executed[order.order_id] += executed_qty;
}

// Exécute un ordre MARKET (BUY ou SELL)
void OrderBook::execute_market_order(Order order) {
    if (order.side == "BUY") {
        execute_buy_market_order(order);
    }
    else {
        execute_sell_market_order(order);
    }
}

// Exécute un ordre MARKET côté BUY
void OrderBook::execute_buy_market_order(Order order) {
    uint64_t remaining_qty = order.quantity;

    // On parcourt les ordres SELL disponibles (meilleurs prix en premier)
    while (remaining_qty > 0 && !sell_orders.empty()) {
        auto& [price, order_queue] = *sell_orders.begin();
        if (order_queue.empty()) {
            sell_orders.erase(sell_orders.begin());
            continue;
        }

        Order& sell_order_ref = order_queue.front();
        Order original_sell_order = sell_order_ref;
        // Quantité à exécuter
        uint64_t trade_qty = std::min(remaining_qty, sell_order_ref.quantity);
        uint64_t exec_timestamp = get_next_execution_timestamp(order.timestamp);

        // Mise à jour des quantités
        remaining_qty -= trade_qty;
        sell_order_ref.quantity -= trade_qty;
        order_queue.update_quantity(trade_qty);

        // Enregistrement de l'exécution côté BUY
        Order buy_execution = order_lookup[order.order_id];
        buy_execution.timestamp = exec_timestamp;
        buy_execution.action = order.action;
        buy_execution.executed_quantity = trade_qty;
        buy_execution.execution_price = price;
        buy_execution.counterparty_id = original_sell_order.order_id;
        buy_execution.status = (remaining_qty == 0) ? "EXECUTED" : "PARTIALLY_EXECUTED";
        buy_execution.quantity = remaining_qty;
        results.push_back(buy_execution);
        record_execution(order, trade_qty);

        // Enregistrement de l'exécution côté SELL
        Order sell_execution = original_sell_order;
        sell_execution.timestamp = exec_timestamp;
        sell_execution.executed_quantity = trade_qty;
        sell_execution.execution_price = price;
        sell_execution.counterparty_id = order.order_id;
        sell_execution.status = (sell_order_ref.quantity == 0) ? "EXECUTED" : "PARTIALLY_EXECUTED";
        sell_execution.quantity = sell_order_ref.quantity;
        results.push_back(sell_execution);
        record_execution(sell_execution, trade_qty);

        // Si l'ordre SELL est terminé, on le retire
        if (sell_order_ref.quantity == 0) {
            order_lookup.erase(original_sell_order.order_id);
            order_queue.pop();
        }

        if (order_queue.empty()) {
            sell_orders.erase(sell_orders.begin());
        }
    }

    if (order.quantity == remaining_qty) {
        // Si aucune exécution = rejet
        Order rejected_order = order_lookup[order.order_id];
        rejected_order.timestamp = get_next_execution_timestamp(order.timestamp);
        rejected_order.action = order.action;
        rejected_order.status = "REJECTED";
        rejected_order.executed_quantity = 0;
        rejected_order.execution_price = 0.0;
        rejected_order.counterparty_id = 0;
        results.push_back(rejected_order);
    }
}

// Exécute un ordre MARKET côté SELL
void OrderBook::execute_sell_market_order(Order order) {
    uint64_t remaining_qty = order.quantity;

    // On parcourt les ordres BUY disponibles (meilleurs prix en premier)
    while (remaining_qty > 0 && !buy_orders.empty()) {
        auto& [price, order_queue] = *buy_orders.begin();
        if (order_queue.empty()) {
            buy_orders.erase(buy_orders.begin());
            continue;
        }

        Order& buy_order_ref = order_queue.front();
        Order original_buy_order = buy_order_ref;
        uint64_t trade_qty = std::min(remaining_qty, buy_order_ref.quantity);
        uint64_t exec_timestamp = get_next_execution_timestamp(order.timestamp);

        // Mise à jour des quantités
        remaining_qty -= trade_qty;
        buy_order_ref.quantity -= trade_qty;
        order_queue.update_quantity(trade_qty);

        // Enregistrement de l'exécution côté SELL
        Order sell_execution = order_lookup[order.order_id];
        sell_execution.timestamp = exec_timestamp;
        sell_execution.action = order.action;
        sell_execution.executed_quantity = trade_qty;
        sell_execution.execution_price = price;
        sell_execution.counterparty_id = original_buy_order.order_id;
        sell_execution.status = (remaining_qty == 0) ? "EXECUTED" : "PARTIALLY_EXECUTED";
        sell_execution.quantity = remaining_qty;
        results.push_back(sell_execution);
        record_execution(order, trade_qty);

        // Enregistrement de l'exécution côté BUY
        Order buy_execution = original_buy_order;
        buy_execution.timestamp = exec_timestamp;
        buy_execution.executed_quantity = trade_qty;
        buy_execution.execution_price = price;
        buy_execution.counterparty_id = order.order_id;
        buy_execution.status = (buy_order_ref.quantity == 0) ? "EXECUTED" : "PARTIALLY_EXECUTED";
        buy_execution.quantity = buy_order_ref.quantity;
        results.push_back(buy_execution);
        record_execution(buy_execution, trade_qty);

        if (buy_order_ref.quantity == 0) {
            order_lookup.erase(original_buy_order.order_id);
            order_queue.pop();
        }

        if (order_queue.empty()) {
            buy_orders.erase(buy_orders.begin());
        }
    }

    if (order.quantity == remaining_qty) {
        // No execution occurred
        Order rejected_order = order_lookup[order.order_id];
        rejected_order.timestamp = get_next_execution_timestamp(order.timestamp);
        rejected_order.action = order.action;
        rejected_order.status = "REJECTED";
        rejected_order.executed_quantity = 0;
        rejected_order.execution_price = 0.0;
        rejected_order.counterparty_id = 0;
        results.push_back(rejected_order);
    }
}

// Exécute un ordre LIMIT (BUY ou SELL)
void OrderBook::execute_limit_order(Order order) {
    if (order.side == "BUY") {
        execute_buy_limit_order(order);
    }
    else {
        execute_sell_limit_order(order);
    }
}

// Exécute un ordre LIMIT côté BUY
void OrderBook::execute_buy_limit_order(Order order) {
    uint64_t remaining_qty = order.quantity;

    // Si l'ordre est NEW ou MODIFY, on vérifie s'il va matcher immédiatement
    if (order.action == "NEW" || order.action == "MODIFY") {
        bool will_execute_immediately = false;
        auto it = sell_orders.begin();
        if (it != sell_orders.end() && it->first <= order.price && !it->second.empty()) {
            will_execute_immediately = true;
        }
        // Si pas de matching immédiat = PENDING
        if (!will_execute_immediately) {
            Order pending_order = order_lookup[order.order_id];
            pending_order.timestamp = get_next_execution_timestamp(order.timestamp);
            pending_order.action = order.action;
            pending_order.status = "PENDING";
            pending_order.executed_quantity = 0;
            pending_order.execution_price = 0.0;
            pending_order.counterparty_id = 0;
            results.push_back(pending_order);
        }
    }

    // Matching avec les ordres SELL
    auto it = sell_orders.begin();
    while (it != sell_orders.end() && remaining_qty > 0) {
        double sell_price = it->first;
        // Prix trop élevé, on s'arrête
        if (sell_price > order.price) break;

        OrderQueue& order_queue = it->second;
        if (order_queue.empty()) {
            it = sell_orders.erase(it);
            continue;
        }

        Order& sell_order_ref = order_queue.front();
        Order original_sell_order = sell_order_ref;
        uint64_t trade_qty = std::min(remaining_qty, sell_order_ref.quantity);
        uint64_t exec_timestamp = get_next_execution_timestamp(order.timestamp);

        // Mise à jour des quantités
        remaining_qty -= trade_qty;
        sell_order_ref.quantity -= trade_qty;
        order_queue.update_quantity(trade_qty);

        // Execution BUY
        Order buy_execution = order_lookup[order.order_id];
        buy_execution.timestamp = exec_timestamp;
        buy_execution.action = order.action;
        buy_execution.executed_quantity = trade_qty;
        buy_execution.execution_price = sell_price;
        buy_execution.counterparty_id = original_sell_order.order_id;
        buy_execution.status = (remaining_qty == 0) ? "EXECUTED" : "PARTIALLY_EXECUTED";
        buy_execution.quantity = remaining_qty;
        results.push_back(buy_execution);
        record_execution(order, trade_qty);

        // Execution SELL
        Order sell_execution = original_sell_order;
        sell_execution.timestamp = exec_timestamp;
        sell_execution.executed_quantity = trade_qty;
        sell_execution.execution_price = sell_price;
        sell_execution.counterparty_id = order.order_id;
        sell_execution.status = (sell_order_ref.quantity == 0) ? "EXECUTED" : "PARTIALLY_EXECUTED";
        sell_execution.quantity = sell_order_ref.quantity;
        results.push_back(sell_execution);
        record_execution(sell_execution, trade_qty);

        // Si quantité restante, on le place dans le carnet BUY
        if (sell_order_ref.quantity == 0) {
            order_lookup.erase(original_sell_order.order_id);
            order_queue.pop();
        }

        if (order_queue.empty()) {
            it = sell_orders.erase(it);
        }
        else {
        }
    }

    if (remaining_qty > 0) {
        Order remaining_order = order_lookup[order.order_id];
        remaining_order.quantity = remaining_qty;
        remaining_order.price = order.price;

        buy_orders[order.price].add_order(remaining_order);
        order_lookup[order.order_id] = remaining_order;
    }
}

// Exécute un ordre LIMIT côté SELL
void OrderBook::execute_sell_limit_order(Order order) {
    uint64_t remaining_qty = order.quantity;

    // Si l'ordre est NEW ou MODIFY, on vérifie s'il va matcher immédiatement
    if (order.action == "NEW" || order.action == "MODIFY") {
        bool will_execute_immediately = false;
        auto it = buy_orders.begin();
        if (it != buy_orders.end() && it->first >= order.price && !it->second.empty()) {
            will_execute_immediately = true;
        }
        // Si pas de matching immédiat = PENDING
        if (!will_execute_immediately) {
            Order pending_order = order_lookup[order.order_id];
            pending_order.timestamp = get_next_execution_timestamp(order.timestamp);
            pending_order.action = order.action;
            pending_order.status = "PENDING";
            pending_order.executed_quantity = 0;
            pending_order.execution_price = 0.0;
            pending_order.counterparty_id = 0;
            results.push_back(pending_order);
        }
    }

    // Matching avec les ordres BUY
    auto it = buy_orders.begin();
    while (it != buy_orders.end() && remaining_qty > 0) {
        double buy_price = it->first;
        // Prix trop bas, on s'arrête
        if (buy_price < order.price) break;

        OrderQueue& order_queue = it->second;
        if (order_queue.empty()) {
            it = buy_orders.erase(it);
            continue;
        }

        Order& buy_order_ref = order_queue.front();
        Order original_buy_order = buy_order_ref;
        uint64_t trade_qty = std::min(remaining_qty, buy_order_ref.quantity);
        uint64_t exec_timestamp = get_next_execution_timestamp(order.timestamp);

        // Mise à jour des quantités
        remaining_qty -= trade_qty;
        buy_order_ref.quantity -= trade_qty;
        order_queue.update_quantity(trade_qty);

        // Execution SELL
        Order sell_execution = order_lookup[order.order_id];
        sell_execution.timestamp = exec_timestamp;
        sell_execution.action = order.action;
        sell_execution.executed_quantity = trade_qty;
        sell_execution.execution_price = buy_price;
        sell_execution.counterparty_id = original_buy_order.order_id;
        sell_execution.status = (remaining_qty == 0) ? "EXECUTED" : "PARTIALLY_EXECUTED";
        sell_execution.quantity = remaining_qty;
        results.push_back(sell_execution);
        record_execution(order, trade_qty);

        // Execution BUY
        Order buy_execution = original_buy_order;
        buy_execution.timestamp = exec_timestamp;
        buy_execution.executed_quantity = trade_qty;
        buy_execution.execution_price = buy_price;
        buy_execution.counterparty_id = order.order_id;
        buy_execution.status = (buy_order_ref.quantity == 0) ? "EXECUTED" : "PARTIALLY_EXECUTED";
        buy_execution.quantity = buy_order_ref.quantity;
        results.push_back(buy_execution);
        record_execution(buy_execution, trade_qty);

        if (buy_order_ref.quantity == 0) {
            order_lookup.erase(original_buy_order.order_id);
            order_queue.pop();
        }

        if (order_queue.empty()) {
            it = buy_orders.erase(it);
        }
        else {
        }
    }

    if (remaining_qty > 0) {
        Order remaining_order = order_lookup[order.order_id];
        remaining_order.quantity = remaining_qty;
        remaining_order.price = order.price;

        sell_orders[order.price].add_order(remaining_order);
        order_lookup[order.order_id] = remaining_order;
    }
}

// Retire un ordre du carnet (BUY ou SELL)
void OrderBook::cancel_order_from_book(const Order& order) {
    // Côté BUY
    if (order.side == "BUY") {
        auto price_it = buy_orders.find(order.price);
        if (price_it != buy_orders.end()) {
            std::queue<Order> new_queue;
            uint64_t new_total_quantity = 0;
            while (!price_it->second.orders.empty()) {
                Order& o = price_it->second.front();
                if (o.order_id != order.order_id) {
                    new_queue.push(o);
                    new_total_quantity += o.quantity;
                }
                price_it->second.pop();
            }
            price_it->second.orders = new_queue;
            price_it->second.total_quantity = new_total_quantity;

            if (price_it->second.empty()) {
                buy_orders.erase(price_it);
            }
        }
    }
    // Côté SELL
    else {
        auto price_it = sell_orders.find(order.price);
        if (price_it != sell_orders.end()) {
            std::queue<Order> new_queue;
            uint64_t new_total_quantity = 0;
            while (!price_it->second.orders.empty()) {
                Order& o = price_it->second.front();
                if (o.order_id != order.order_id) {
                    new_queue.push(o);
                    new_total_quantity += o.quantity;
                }
                price_it->second.pop();
            }
            price_it->second.orders = new_queue;
            price_it->second.total_quantity = new_total_quantity;

            if (price_it->second.empty()) {
                sell_orders.erase(price_it);
            }
        }
    }
}


#endif // ORDER_BOOK_H