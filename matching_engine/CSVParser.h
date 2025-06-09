#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include "Order.h"
#include "Validator.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <unordered_set>

// Classe utilitaire pour gérer les fichiers CSV
class CSVParser {
public:
    static std::vector<Order> parse_input_file(const std::string& filename);
    static void write_output_file(const std::string& filename, const std::vector<Order>& orders);

private:
    static Order parse_order_line(const std::string& line, int line_number);
    static std::vector<std::string> split_csv_line(const std::string& line);
    static std::string trim(const std::string& str);
};

// Lecture du fichier CSV d'entrée
std::vector<Order> CSVParser::parse_input_file(const std::string& filename) {
    std::vector<Order> orders;
    std::ifstream file(filename);
    std::string line;
    int line_number = 0;
    std::unordered_set<uint64_t> seen_order_ids; // Pour vérifier les doublons d'order_id

    if (!file.is_open()) {
        std::cerr << "Error: Could not open input file: " << filename << std::endl;
        return orders;
    }

    // Sauter l'en-tête
    if (std::getline(file, line)) {
        line_number++;
    }

    while (std::getline(file, line)) {
        line_number++;
        if (line.empty() || std::all_of(line.begin(), line.end(), ::isspace)) {
            continue;
        }

        Order order = parse_order_line(line, line_number);

        // Vérifie les doublons sur les ordres "NEW"
        if (order.status != "REJECTED" && order.action == "NEW") {
            if (seen_order_ids.find(order.order_id) != seen_order_ids.end()) {
                order.status = "REJECTED";
            } else {
                seen_order_ids.insert(order.order_id);
            }
        }

        // Ajoute l'ordre à la liste si valide
        if (order.order_id != 0 || order.status == "REJECTED") {
            orders.push_back(order);
        }
    }

    file.close();
    return orders;
}

// Écriture du fichier CSV de sortie
void CSVParser::write_output_file(const std::string& filename, const std::vector<Order>& orders) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open output file: " << filename << std::endl;
        return;
    }

    // Écrit l'en-tête
    file << "timestamp,order_id,instrument,side,type,quantity,price,action,"
         << "status,executed_quantity,execution_price,counterparty_id\n";

    // Écrit les ordres
    for (const auto& order : orders) {
        file << order.timestamp << ","
             << order.order_id << ","
             << order.instrument << ","
             << order.side << ","
             << order.type << ","
             << order.quantity << ","
             << std::fixed << std::setprecision(2) << order.price << ","
             << order.action << ","
             << order.status << ","
             << order.executed_quantity << ","
             << std::fixed << std::setprecision(2) << order.execution_price << ","
             << order.counterparty_id << "\n";
    }

    file.close();
}

// Parse une ligne CSV en Order
Order CSVParser::parse_order_line(const std::string& line, int line_number) {
    Order order;
    std::vector<std::string> fields = split_csv_line(line);

    // Vérifie que la ligne a le bon nombre de champs
    if (fields.size() != 8) {
        std::cerr << "Warning: Line " << line_number << " has " << fields.size() 
                  << " fields instead of 8, rejecting order" << std::endl;

        // Tente de récupérer partiellement les champs pour un ordre rejeté
        if (fields.size() >= 2) {
            if (Validator::is_valid_integer(fields[0])) {
                order.timestamp = std::stoull(fields[0]);
            }
            if (Validator::is_valid_integer(fields[1])) {
                order.order_id = std::stoull(fields[1]);
            }
        }
        if (fields.size() >= 3) order.instrument = trim(fields[2]);
        if (fields.size() >= 4) order.side = trim(fields[3]);
        if (fields.size() >= 5) order.type = trim(fields[4]);
        if (fields.size() >= 6 && Validator::is_valid_integer(fields[5])) {
            order.quantity = std::stoull(fields[5]);
        }
        if (fields.size() >= 7 && Validator::is_valid_number(fields[6])) {
            order.price = std::stod(fields[6]);
        }
        if (fields.size() >= 8) order.action = trim(fields[7]);

        order.status = "REJECTED";
        return order;
    }

    try {
        // Validation des champs principaux
        if (!Validator::is_valid_integer(trim(fields[0]))) {
            order.status = "REJECTED";
            return order;
        }
        order.timestamp = std::stoull(trim(fields[0]));

        if (!Validator::is_valid_integer(trim(fields[1]))) {
            order.status = "REJECTED";
            return order;
        }
        order.order_id = std::stoull(trim(fields[1]));

        order.instrument = trim(fields[2]);
        order.side = Validator::to_upper(trim(fields[3]));
        order.type = Validator::to_upper(trim(fields[4]));

        std::string qty_str = trim(fields[5]);
        if (!Validator::is_valid_integer(qty_str) || qty_str[0] == '-') {
            order.status = "REJECTED";
            order.quantity = 0;
            order.instrument = trim(fields[2]);
            order.side = Validator::to_upper(trim(fields[3]));
            order.type = Validator::to_upper(trim(fields[4]));
            order.action = Validator::to_upper(trim(fields[7]));
            return order;
        }
        order.quantity = std::stoull(qty_str);

        std::string price_str = trim(fields[6]);
        if (!Validator::is_valid_number(price_str)) {
            order.status = "REJECTED";
            return order;
        }
        order.price = std::stod(price_str);

        order.action = Validator::to_upper(trim(fields[7]));

        // Validation finale de l'ordre
        Validator::ValidationResult validation = Validator::validate_order(order);
        if (validation != Validator::ValidationResult::VALID) {
            order.status = "REJECTED";
            return order;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error parsing line " << line_number << ": " << e.what() << std::endl;
        order.status = "REJECTED";
        return order;
    }

    return order;
}

// Découpe une ligne CSV en champs
std::vector<std::string> CSVParser::split_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;

    while (std::getline(ss, field, ',')) {
        fields.push_back(field);
    }

    return fields;
}

// Supprime les espaces en début et fin de chaîne
std::string CSVParser::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";

    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

#endif // CSV_PARSER_H