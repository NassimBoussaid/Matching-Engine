#ifndef VALIDATOR_H
#define VALIDATOR_H

#include "Order.h"
#include <string>
#include <algorithm>
#include <cctype>

// Classe utilitaire de validation des ordres
class Validator {
public:
    // Enumération des résultats possibles de la validation
    enum class ValidationResult {
        VALID,
        INVALID_SIDE,
        INVALID_TYPE,
        INVALID_ACTION,
        NEGATIVE_QUANTITY,
        NEGATIVE_PRICE,
        EMPTY_FIELD,
        INVALID_FORMAT,
        DUPLICATE_ORDER
    };
    
    // Valide un ordre
    static ValidationResult validate_order(const Order& order);
    // Convertit une chaîne en majuscules
    static std::string to_upper(const std::string& str);
    // Vérifie si une chaîne représente un nombre valide
    static bool is_valid_number(const std::string& str);
    // Vérifie si une chaîne représente un entier valide
    static bool is_valid_integer(const std::string& str);
    // Vérifie si une chaîne est vide ou contient uniquement des espaces
    static bool is_empty_or_whitespace(const std::string& str);
    
private:
    // Vérifie si le champ side est valide (BUY / SELL)
    static bool is_valid_side(const std::string& side);
    // Vérifie si le champ type est valide (LIMIT / MARKET)
    static bool is_valid_type(const std::string& type);
    // Vérifie si le champ action est valide (NEW / MODIFY / CANCEL)
    static bool is_valid_action(const std::string& action);
};

// Fonction principale de validation d'un ordre
Validator::ValidationResult Validator::validate_order(const Order& order) {
    // Vérifie les champs obligatoires non vides
    if (is_empty_or_whitespace(order.instrument) ||
        is_empty_or_whitespace(order.side) ||
        is_empty_or_whitespace(order.type) ||
        is_empty_or_whitespace(order.action)) {
        return ValidationResult::EMPTY_FIELD;
    }
    
    // Validation du side
    if (!is_valid_side(order.side)) {
        return ValidationResult::INVALID_SIDE;
    }
    
    // Validation du type
    if (!is_valid_type(order.type)) {
        return ValidationResult::INVALID_TYPE;
    }
    
    // Validation de l'action
    if (!is_valid_action(order.action)) {
        return ValidationResult::INVALID_ACTION;
    }
    
    // Validation de la quantité (quantité négative ou débordement)
    if (order.quantity == 0 || order.quantity > 1000000000000ULL) {
        return ValidationResult::NEGATIVE_QUANTITY;
    }
    
    // Validation du prix pour les ordres LIMIT
    if (order.type == "LIMIT" && order.price < 0) {
        return ValidationResult::NEGATIVE_PRICE;
    }
    
    return ValidationResult::VALID;
}

// Conversion en majuscules
std::string Validator::to_upper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// Vérifie si la chaîne est un nombre valide
bool Validator::is_valid_number(const std::string& str) {
    if (str.empty()) return false;
    
    bool has_dot = false;
    bool has_digit = false;
    size_t start = 0;
    
    // Signe optionnel en début
    if (str[0] == '-' || str[0] == '+') {
        start = 1;
        if (str.length() == 1) return false;
    }
    
    for (size_t i = start; i < str.length(); ++i) {
        if (std::isdigit(str[i])) {
            has_digit = true;
        } else if (str[i] == '.' && !has_dot) {
            has_dot = true;
        } else {
            return false;
        }
    }
    
    return has_digit;
}

// Vérifie si la chaîne est un entier valide
bool Validator::is_valid_integer(const std::string& str) {
    if (str.empty()) return false;
    
    size_t start = 0;
    if (str[0] == '-' || str[0] == '+') {
        start = 1;
        if (str.length() == 1) return false;
    }
    
    for (size_t i = start; i < str.length(); ++i) {
        if (!std::isdigit(str[i])) {
            return false;
        }
    }
    
    return true;
}

// Vérifie si la chaîne est vide ou ne contient que des espaces
bool Validator::is_empty_or_whitespace(const std::string& str) {
    return str.empty() || std::all_of(str.begin(), str.end(), ::isspace);
}

// Vérifie si le side est valide
bool Validator::is_valid_side(const std::string& side) {
    std::string upper_side = to_upper(side);
    return upper_side == "BUY" || upper_side == "SELL";
}

// Vérifie si le type est valide
bool Validator::is_valid_type(const std::string& type) {
    std::string upper_type = to_upper(type);
    return upper_type == "LIMIT" || upper_type == "MARKET";
}

// Vérifie si l'action est valide
bool Validator::is_valid_action(const std::string& action) {
    std::string upper_action = to_upper(action);
    return upper_action == "NEW" || upper_action == "MODIFY" || upper_action == "CANCEL";
}

#endif // VALIDATOR_H
