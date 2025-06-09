#include "Order.h"
#include "Validator.h"
#include "CSVParser.h"
#include "OrderBook.h"
#include "MatchingEngine.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>

class TestFramework {
    int tests_run = 0, tests_passed = 0;
public:
    template<typename T>
    void assert_equal(const std::string& test_name, T expected, T actual) {
        tests_run++;
        if (expected == actual) {
            tests_passed++;
            std::cout << "✓ " << test_name << " PASSED\n";
        } else {
            std::cout << "✗ " << test_name << " FAILED: expected " << expected << ", got " << actual << "\n";
        }
    }
    
    void assert_true(const std::string& test_name, bool condition) {
        tests_run++;
        if (condition) {
            tests_passed++;
            std::cout << "✓ " << test_name << " PASSED\n";
        } else {
            std::cout << "✗ " << test_name << " FAILED\n";
        }
    }
    
    void print_summary() {
        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "Test Summary: " << tests_passed << "/" << tests_run << " tests passed\n";
        std::cout << (tests_passed == tests_run ? "All tests PASSED! ✓" : "Some tests FAILED! ✗") << "\n";
        std::cout << std::string(50, '=') << "\n";
    }
    
    int get_passed_tests() const { return tests_passed; }
    int get_total_tests() const { return tests_run; }
};

void create_test_input_csv() {
    std::ofstream file("input.csv");
    file << "timestamp,order_id,instrument,side,type,quantity,price,action\n";
    file << "1617278400000000000,1,AAPL,BUY,LIMIT,100,150.25,NEW\n";
    file << "1617278400000000100,2,AAPL,SELL,LIMIT,50,150.25,NEW\n";
    file << "1617278400000000200,3,AAPL,SELL,LIMIT,60,150.30,NEW\n";
    file << "1617278400000000300,4,AAPL,BUY,LIMIT,40,150.20,NEW\n";
    file << "1617278400000000400,1,AAPL,BUY,LIMIT,100,150.30,MODIFY\n";
    file << "1617278400000000500,3,AAPL,SELL,LIMIT,60,0,CANCEL\n";
    file.close();
}

std::vector<std::string> parse_expected_output() {
    std::vector<std::string> expected_lines;
    expected_lines.push_back("1617278400000000000,1,AAPL,BUY,LIMIT,100,150.25,NEW,PENDING,0,0.00,0");
    expected_lines.push_back("1617278400000000100,2,AAPL,SELL,LIMIT,0,150.25,NEW,EXECUTED,50,150.25,1");
    expected_lines.push_back("1617278400000000100,1,AAPL,BUY,LIMIT,50,150.25,NEW,PARTIALLY_EXECUTED,50,150.25,2");
    expected_lines.push_back("1617278400000000200,3,AAPL,SELL,LIMIT,60,150.30,NEW,PENDING,0,0.00,0");
    expected_lines.push_back("1617278400000000300,4,AAPL,BUY,LIMIT,40,150.20,NEW,PENDING,0,0.00,0");
    expected_lines.push_back("1617278400000000400,1,AAPL,BUY,LIMIT,0,150.30,MODIFY,EXECUTED,50,150.30,3");
    expected_lines.push_back("1617278400000000400,3,AAPL,SELL,LIMIT,10,150.30,NEW,PARTIALLY_EXECUTED,50,150.30,1");
    expected_lines.push_back("1617278400000000500,3,AAPL,SELL,LIMIT,0,0.00,CANCEL,CANCELED,0,0.00,0");
    return expected_lines;
}

Order create_order(uint64_t timestamp, uint64_t id, const std::string& instrument, 
                   const std::string& side, const std::string& type, uint64_t quantity, 
                   double price, const std::string& action) {
    Order order;
    order.timestamp = timestamp;
    order.order_id = id;
    order.instrument = instrument;
    order.side = side;
    order.type = type;
    order.quantity = quantity;
    order.price = price;
    order.action = action;
    return order;
}

void test_exact_expected_output(TestFramework& tf) {
    std::cout << "\n=== Testing Exact Expected Output Match ===\n";
    
    create_test_input_csv();
    
    // Analyse du fichier d'entrée
    std::vector<Order> orders = CSVParser::parse_input_file("input.csv");
    
    // Traitement avec le Matching Engine
    MatchingEngine engine;
    for (size_t i = 0; i < orders.size(); ++i) {
        engine.process_order(orders[i]);
    }
    
    // Obtention des résultats et écriture de la sortie
    std::vector<Order> results = engine.get_all_results();
    CSVParser::write_output_file("output.csv", results);
    
    // Lecture du fichier de sortie généré
    std::ifstream output_file("output.csv");
    std::string line;
    std::vector<std::string> actual_lines;
    
    // Skip header
    if (std::getline(output_file, line)) {
        // Header line
    }
    
    while (std::getline(output_file, line)) {
        if (!line.empty()) {
            actual_lines.push_back(line);
        }
    }
    output_file.close();
    
    // Récupérer la sortie attendue
    std::vector<std::string> expected_lines = parse_expected_output();
    
    tf.assert_equal("Output line count", (int)expected_lines.size(), (int)actual_lines.size());
    
    // Comparer chaque ligne
    for (size_t i = 0; i < std::min(expected_lines.size(), actual_lines.size()); ++i) {
        std::stringstream test_name;
        test_name << "Line " << (i + 1) << " match";
        tf.assert_equal(test_name.str(), expected_lines[i], actual_lines[i]);
        
        if (expected_lines[i] != actual_lines[i]) {
            std::cout << "Expected: " << expected_lines[i] << std::endl;
            std::cout << "Actual:   " << actual_lines[i] << std::endl;
        }
    }
    
    std::cout << "\nGenerated output.csv file contents:\n";
    std::cout << "timestamp,order_id,instrument,side,type,quantity,price,action,status,executed_quantity,execution_price,counterparty_id\n";
    for (const std::string& line : actual_lines) {
        std::cout << line << std::endl;
    }
}

void test_validation_comprehensive(TestFramework& tf) {
    std::cout << "\n=== Testing Comprehensive Validation ===\n";
    
    // Test d'un ordre valide
    Order valid = create_order(1617278400000000000ULL, 1, "AAPL", "BUY", "LIMIT", 100, 150.25, "NEW");
    tf.assert_equal("Valid order validation", (int)Validator::ValidationResult::VALID, 
                    (int)Validator::validate_order(valid));
    
    // Test d'un side invalide
    Order invalid_side = valid;
    invalid_side.side = "INVALID";
    tf.assert_equal("Invalid side validation", (int)Validator::ValidationResult::INVALID_SIDE,
                    (int)Validator::validate_order(invalid_side));
    
    // Test d'un champ vide
    Order empty_field = valid;
    empty_field.instrument = "";
    tf.assert_equal("Empty field validation", (int)Validator::ValidationResult::EMPTY_FIELD,
                    (int)Validator::validate_order(empty_field));
    
    // Test des fonctions utilitaires
    tf.assert_equal("Upper case conversion", std::string("BUY"), Validator::to_upper("buy"));
    tf.assert_true("Valid integer check", Validator::is_valid_integer("123"));
    tf.assert_true("Invalid integer check", !Validator::is_valid_integer("12.3"));
    tf.assert_true("Valid number check", Validator::is_valid_number("123.45"));
    tf.assert_true("Invalid number check", !Validator::is_valid_number("12.3.4"));
}

void test_csv_parsing_errors(TestFramework& tf) {
    std::cout << "\n=== Testing CSV Parsing Error Handling ===\n";
    
    // Création d'un fichier de test contenant divers cas d'erreur
    std::ofstream error_file("error_test.csv");
    error_file << "timestamp,order_id,instrument,side,type,quantity,price,action\n";
    error_file << "1617278400000000000,1,AAPL,BUY,LIMIT,100,150.25,NEW\n";  // Valide
    error_file << "1617278400000000100,2,AAPL,SELL,LIMIT,-50,150.25,NEW\n"; // Quantité négative
    error_file << "1617278400000000200,3,AAPL,SELL,LIMIT,50,-150,NEW\n";    // Prix négatif
    error_file << "1617278400000000300,4,AAPL,INVALID,LIMIT,40,150.20,NEW\n"; // Side invalide
    error_file << "1617278400000000400,5,AAPL,BUY,INVALID,100,150.30,NEW\n"; // Type invalide
    error_file << "1617278400000000500,6,AAPL,BUY,LIMIT,60,0,INVALID\n";     // Action invalide
    error_file << "1617278400000000600,7,AAPL, ,LIMIT,100,150.25,NEW\n";     // Side vide
    error_file << "1617278400000000700,1,AAPL,BUY,LIMIT,200,150.25,NEW\n";   // ID dupliqué
    error_file.close();
    
    std::vector<Order> orders = CSVParser::parse_input_file("error_test.csv");
    
    int valid_count = 0, rejected_count = 0;
    for (size_t i = 0; i < orders.size(); ++i) {
        if (orders[i].status == "REJECTED") {
            rejected_count++;
        } else {
            valid_count++;
        }
    }
    
    tf.assert_equal("Valid orders from error test", 1, valid_count);
    tf.assert_equal("Rejected orders from error test", 7, rejected_count);
}

void test_order_book_matching(TestFramework& tf) {
    std::cout << "\n=== Testing Order Book Matching Logic ===\n";
    
    MatchingEngine engine;
    
    // Test d'un matching de base
    Order buy1 = create_order(1617278400000000000ULL, 1, "AAPL", "BUY", "LIMIT", 100, 150.25, "NEW");
    Order sell1 = create_order(1617278400000000100ULL, 2, "AAPL", "SELL", "LIMIT", 50, 150.25, "NEW");
    
    engine.process_order(buy1);
    engine.process_order(sell1);
    
    std::vector<Order> results = engine.get_all_results();
    
    tf.assert_equal("Basic matching result count", 3, (int)results.size());
    
    bool found_executed_sell = false, found_partial_buy = false;
    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i].order_id == 2 && results[i].status == "EXECUTED") {
            found_executed_sell = true;
        }
        if (results[i].order_id == 1 && results[i].status == "PARTIALLY_EXECUTED") {
            found_partial_buy = true;
        }
    }
    
    tf.assert_true("Found executed sell order", found_executed_sell);
    tf.assert_true("Found partially executed buy order", found_partial_buy);
}

void test_modify_order_behavior(TestFramework& tf) {
    std::cout << "\n=== Testing MODIFY Order Behavior ===\n";
    
    MatchingEngine engine;
    
    // Création de l'ordre initial
    Order initial = create_order(1617278400000000000ULL, 1, "AAPL", "BUY", "LIMIT", 100, 150.25, "NEW");
    engine.process_order(initial);
    
    // Execution partielle
    Order sell1 = create_order(1617278400000000100ULL, 2, "AAPL", "SELL", "LIMIT", 50, 150.25, "NEW");
    engine.process_order(sell1);
    
    // Modification de l'ordre initial
    Order modify = create_order(1617278400000000200ULL, 1, "AAPL", "BUY", "LIMIT", 100, 150.30, "MODIFY");
    engine.process_order(modify);
    
    std::vector<Order> results = engine.get_all_results();
    
    bool found_modify_pending = false;
    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i].action == "MODIFY" && results[i].order_id == 1) {
            found_modify_pending = true;
        }
    }
    
    tf.assert_true("Found MODIFY action result", found_modify_pending);
}

void test_cancel_order_behavior(TestFramework& tf) {
    std::cout << "\n=== Testing CANCEL Order Behavior ===\n";
    
    MatchingEngine engine;
    
    // Création et suppression d'un ordre
    Order initial = create_order(1617278400000000000ULL, 1, "AAPL", "BUY", "LIMIT", 100, 150.25, "NEW");
    Order cancel = create_order(1617278400000000100ULL, 1, "AAPL", "BUY", "LIMIT", 100, 0, "CANCEL");
    
    engine.process_order(initial);
    engine.process_order(cancel);
    
    std::vector<Order> results = engine.get_all_results();
    
    bool found_canceled = false;
    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i].action == "CANCEL" && results[i].status == "CANCELED") {
            found_canceled = true;
        }
    }
    
    tf.assert_true("Found CANCEL action result", found_canceled);
}

void test_duplicate_order_handling(TestFramework& tf) {
    std::cout << "\n=== Testing Duplicate Order Handling ===\n";
    
    MatchingEngine engine;
    
    Order order1 = create_order(1617278400000000000ULL, 1, "AAPL", "BUY", "LIMIT", 100, 150.25, "NEW");
    Order order2 = create_order(1617278400000000100ULL, 1, "AAPL", "BUY", "LIMIT", 200, 150.20, "NEW"); // Same ID
    
    engine.process_order(order1);
    engine.process_order(order2);
    
    std::vector<Order> results = engine.get_all_results();
    
    bool found_rejected = false;
    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i].order_id == 1 && results[i].status == "REJECTED" && 
            results[i].timestamp == order2.timestamp) {
            found_rejected = true;
        }
    }
    
    tf.assert_true("Duplicate order rejected", found_rejected);
}

void test_multi_instrument_support(TestFramework& tf) {
    std::cout << "\n=== Testing Multi-Instrument Support ===\n";
    
    MatchingEngine engine;
    
    Order aapl_buy = create_order(1617278400000000000ULL, 1, "AAPL", "BUY", "LIMIT", 100, 150.25, "NEW");
    Order googl_sell = create_order(1617278400000000100ULL, 2, "GOOGL", "SELL", "LIMIT", 100, 150.25, "NEW");
    
    engine.process_order(aapl_buy);
    engine.process_order(googl_sell);
    
    std::vector<Order> results = engine.get_all_results();
    
    bool found_aapl = false, found_googl = false;
    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i].instrument == "AAPL") found_aapl = true;
        if (results[i].instrument == "GOOGL") found_googl = true;
    }
    
    tf.assert_true("Found AAPL order", found_aapl);
    tf.assert_true("Found GOOGL order", found_googl);
    tf.assert_equal("Multi-instrument isolation", 2, (int)results.size());
}

void test_timestamp_ordering(TestFramework& tf) {
    std::cout << "\n=== Testing Timestamp Ordering ===\n";
    
    MatchingEngine engine;
    
    // Ajout d'ordre avec différents timestamps
    Order order1 = create_order(1617278400000000300ULL, 1, "AAPL", "BUY", "LIMIT", 100, 150.25, "NEW");
    Order order2 = create_order(1617278400000000100ULL, 2, "AAPL", "BUY", "LIMIT", 100, 150.20, "NEW");
    Order order3 = create_order(1617278400000000200ULL, 3, "AAPL", "BUY", "LIMIT", 100, 150.30, "NEW");
    
    engine.process_order(order1);
    engine.process_order(order2);
    engine.process_order(order3);
    
    std::vector<Order> results = engine.get_all_results();
    
    // Les résultats devraient être ordonnés par timestamps
    bool correctly_ordered = true;
    for (size_t i = 1; i < results.size(); ++i) {
        if (results[i].timestamp < results[i-1].timestamp) {
            correctly_ordered = false;
            break;
        }
    }
    
    tf.assert_true("Results ordered by timestamp", correctly_ordered);
}

void run_performance_test(TestFramework& tf) {
    std::cout << "\n=== Performance Test ===\n";
    
    MatchingEngine engine;
    auto start = std::chrono::high_resolution_clock::now();
    
    // Calcul de 1000 ordres
    for (int i = 1; i <= 1000; i++) {
        Order order = create_order(1617278400000000000ULL + i, i, "AAPL", 
                                  (i % 2 == 0) ? "BUY" : "SELL", "LIMIT", 100, 
                                  150.0 + (i % 10) * 0.01, "NEW");
        engine.process_order(order);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Processed 1000 orders in " << duration.count() / 1000.0 << " ms\n";
    
    std::vector<Order> results = engine.get_all_results();
    tf.assert_true("Performance test completed", results.size() > 0);
    
    // Performance acceptable (moins de 100ms pour 1000 ordres)
    tf.assert_true("Performance acceptable", duration.count() < 100000);
}

int main() {
    std::cout << "Financial Matching Engine - Comprehensive Test Suite\n";
    std::cout << std::string(60, '=') << "\n";
    
    TestFramework tf;
    
    try {
        // Core functionality tests
        test_exact_expected_output(tf);
        test_validation_comprehensive(tf);
        test_csv_parsing_errors(tf);
        
        // Matching engine tests
        test_order_book_matching(tf);
        test_modify_order_behavior(tf);
        test_cancel_order_behavior(tf);
        test_duplicate_order_handling(tf);
        
        // Advanced tests
        test_multi_instrument_support(tf);
        test_timestamp_ordering(tf);
        run_performance_test(tf);
        
    } catch (const std::exception& e) {
        std::cerr << "Test execution error: " << e.what() << "\n";
        return 1;
    }
    
    tf.print_summary();
    
    // Cleanup
    std::cout << "\nCleaning up test files...\n";
    [[maybe_unused]] int cleanup_status = system("rm -f input.csv output.csv error_test.csv");

    return (tf.get_passed_tests() == tf.get_total_tests()) ? 0 : 1;
}