#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <cstdint>

// Structure représentant un ordre (BUY ou SELL)
struct Order {
    uint64_t timestamp;         // Horodatage de l'ordre
    uint64_t order_id;          // Identifiant unique
    std::string instrument;     // Instrument financier (ex: AAPL)
    std::string side;           // Côté (BUY ou SELL)
    std::string type;           // Type d'ordre (LIMIT, MARKET, etc.)
    uint64_t quantity;          // Quantité
    double price;               // Prix
    std::string action;         // Action (NEW, MODIFY, CANCEL)
    
    // Champs supplémentaires pour la sortie
    std::string status;         // Statut de l'ordre (EXECUTED, REJECTED, etc.)
    uint64_t executed_quantity; // Quantité exécutée
    double execution_price;     // Prix d'exécution
    uint64_t counterparty_id;   // ID de l'ordre contrepartie
    
    // Constructeur par défaut
    Order() : timestamp(0), order_id(0), quantity(0), price(0.0), 
              executed_quantity(0), execution_price(0.0), counterparty_id(0) {}
              
    // Constructeur de copie par défaut
    Order(const Order& other) = default;
    Order& operator=(const Order& other) = default;
};

// Structure représentant une exécution (trade)
struct Trade {
    Order buy_order;    // Ordre acheteur
    Order sell_order;   // Ordre vendeur
    uint64_t quantity;  // Quantité échangée
    double price;       // Prix d'exécution
    uint64_t timestamp; // Horodatage de l'exécution
};

#endif // ORDER_H
