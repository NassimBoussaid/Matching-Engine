# Makefile for Financial Matching Engine

CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -pedantic -I.
TARGET = matching_engine
SOURCES = main.cpp
HEADERS = Order.h Validator.h CSVParser.h OrderBook.h MatchingEngine.h
TEST_TARGET = test_matching_engine
TEST_SOURCES = test_matching_engine.cpp

# Default target
all: $(TARGET)

# Build main executable
$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

# Build test executable
test: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_SOURCES)

# Debug build
debug: CXXFLAGS = -std=c++17 -g -Wall -Wextra -pedantic -DDEBUG -I.
debug: $(TARGET)

# Run tests
run_tests: $(TEST_TARGET)
	./$(TEST_TARGET)

# Create sample input file
sample_input:
	@echo "timestamp,order_id,instrument,side,type,quantity,price,action" > input.csv
	@echo "1617278400000000000,1,AAPL,BUY,LIMIT,100,150.25,NEW" >> input.csv
	@echo "1617278400000000100,2,AAPL,SELL,LIMIT,50,150.25,NEW" >> input.csv
	@echo "1617278400000000200,3,AAPL,SELL,LIMIT,60,150.30,NEW" >> input.csv
	@echo "1617278400000000300,4,AAPL,BUY,LIMIT,40,150.20,NEW" >> input.csv
	@echo "1617278400000000400,1,AAPL,BUY,LIMIT,100,150.30,MODIFY" >> input.csv
	@echo "1617278400000000500,3,AAPL,SELL,LIMIT,60,0,CANCEL" >> input.csv
	@echo "Sample input file created: input.csv"

# Create validation test file
validation_test:
	@echo "timestamp,order_id,instrument,side,type,quantity,price,action" > validation_test.csv
	@echo "1617278400000000000,1,AAPL,BUY,LIMIT,100,150.25,NEW" >> validation_test.csv
	@echo "1617278400000000100,2,AAPL,SELL,LIMIT,-50,150.25,NEW" >> validation_test.csv
	@echo "1617278400000000200,3,AAPL,SELL,LIMIT,50,-150,NEW" >> validation_test.csv
	@echo "1617278400000000300,4,AAPL,INVALID,LIMIT,40,150.20,NEW" >> validation_test.csv
	@echo "1617278400000000400,5,AAPL,BUY,INVALID,100,150.30,NEW" >> validation_test.csv
	@echo "1617278400000000500,6,AAPL,BUY,LIMIT,60,0,INVALID" >> validation_test.csv
	@echo "1617278400000000600,7,AAPL, ,LIMIT,100,150.25,NEW" >> validation_test.csv
	@echo "1617278400000000700,1,AAPL,BUY,LIMIT,200,150.25,NEW" >> validation_test.csv
	@echo "1617278400000000800,8,AAPL,BUY,LIMIT,100,150.25,NEW,EXTRA" >> validation_test.csv
	@echo "Validation test file created: validation_test.csv"

# Run with sample data
run_sample: $(TARGET) sample_input
	./$(TARGET) input.csv output.csv
	@echo "Sample output:"
	@head -10 output.csv

# Run validation tests
run_validation: $(TARGET) validation_test
	./$(TARGET) validation_test.csv validation_output.csv
	@echo "Validation test output:"
	@cat validation_output.csv

# Clean build artifacts
clean:
	rm -f $(TARGET) $(TEST_TARGET) *.csv *.o

# Install dependencies (Ubuntu/Debian)
install_deps:
	sudo apt-get update
	sudo apt-get install -y build-essential g++ valgrind python3

# Help
help:
	@echo "Available targets:"
	@echo "  all               - Build main executable"
	@echo "  test              - Build test executable"
	@echo "  run_tests         - Build and run tests"
	@echo "  debug             - Build with debug symbols"
	@echo "  sample_input      - Create sample input file"
	@echo "  validation_test   - Create validation test file"
	@echo "  run_sample        - Run with sample data"
	@echo "  run_validation    - Run validation tests"
	@echo "  clean             - Remove build artifacts"
	@echo "  install_deps      - Install required dependencies"
	@echo "  help              - Show this help message"

.PHONY: all test debug run_tests sample_input validation_test run_sample run_validation clean install_deps help