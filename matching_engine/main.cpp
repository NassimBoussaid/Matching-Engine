#include "Order.h"
#include "Validator.h"
#include "CSVParser.h"
#include "OrderBook.h"
#include "MatchingEngine.h"
#include <iostream>
#include <chrono>
#include <iomanip>

// Utilitaire pour mesurer le temps d'exécution
class PerformanceTimer {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    
public:
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    double stop() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        return duration.count() / 1000.0; // Temps en millisecondes
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }
    
    std::string input_file = argv[1];
    std::string output_file = argv[2];
    
    try {
        PerformanceTimer timer;
        timer.start();
        
        // Lecture du fichier d'entrée
        std::cout << "Reading input file: " << input_file << std::endl;
        std::vector<Order> orders = CSVParser::parse_input_file(input_file);
        std::cout << "Parsed " << orders.size() << " orders" << std::endl;
        
        // Comptage des ordres rejetés
        int rejected_count = 0;
        for (const auto& order : orders) {
            if (order.status == "REJECTED") {
                rejected_count++;
            }
        }
        
        if (rejected_count > 0) {
            std::cout << "Warning: " << rejected_count << " orders were rejected due to validation errors" << std::endl;
        }
        
        // Traitement des ordres par le moteur de matching
        std::cout << "Processing orders..." << std::endl;
        MatchingEngine engine;
        
        for (const auto& order : orders) {
            engine.process_order(order);
        }
        
        // Récupération des résultats et écriture du fichier de sortie
        std::vector<Order> results = engine.get_all_results();
        std::cout << "Generated " << results.size() << " result records" << std::endl;
        
        CSVParser::write_output_file(output_file, results);
        std::cout << "Output written to: " << output_file << std::endl;
        
        // Affichage du temps total de traitement
        double elapsed = timer.stop();
        std::cout << "Total processing time: " << std::fixed << std::setprecision(2) 
                  << elapsed << " ms" << std::endl;
        
        if (orders.size() > 0) {
            std::cout << "Average time per order: " << std::fixed << std::setprecision(3)
                      << (elapsed / orders.size()) << " ms" << std::endl;
        }
        
        // Statistiques d'exécution
        int executed = 0, partially_executed = 0, pending = 0, canceled = 0, rejected = 0;
        for (const auto& result : results) {
            if (result.status == "EXECUTED") executed++;
            else if (result.status == "PARTIALLY_EXECUTED") partially_executed++;
            else if (result.status == "PENDING") pending++;
            else if (result.status == "CANCELED") canceled++;
            else if (result.status == "REJECTED") rejected++;
        }
        
        std::cout << "\nExecution Statistics:" << std::endl;
        std::cout << "  Executed: " << executed << std::endl;
        std::cout << "  Partially Executed: " << partially_executed << std::endl;
        std::cout << "  Pending: " << pending << std::endl;
        std::cout << "  Canceled: " << canceled << std::endl;
        std::cout << "  Rejected: " << rejected << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
