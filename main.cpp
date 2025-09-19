/**
 * Naive Order Book Implementation - Main Entry Point
 * A price-time priority matching engine with comprehensive order lifecycle management
 */

#include <iostream>
#include "orderbook.h"
#include "csv_processor.h"
#include "testing_framework.h"

int main(int argc, char* argv[]) {
    OrderBook orderBook;

    // Check if CSV file is provided as command line argument
    if (argc == 2) {
        std::cout << "Welcome to the Order Book Testing Framework!\n";
        std::cout << "Running in CSV mode with file: " << argv[1] << "\n\n";

        processCsvFile(argv[1], orderBook);
        return 0;
    }

    // Interactive mode (original functionality)
    int choice;
    std::cout << "Welcome to the Order Book Testing Framework!\n";
    std::cout << "Running in interactive mode. To use CSV mode, run: ./orderbook <csvfile>\n\n";

    while (true) {
        testingDisplayMenu();
        std::cin >> choice;
        
        // Handle invalid menu input
        if (std::cin.fail() || choice < 1 || choice > 5) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid choice. Please enter a number between 1-5.\n";
            continue;
        }
        
        switch (choice) {
            case 1:
                testingCreateOrder(orderBook);
                break;
            case 2:
                testingModifyOrder(orderBook);
                break;
            case 3:
                testingCancelOrder(orderBook);
                break;
            case 4:
                testingDisplayOrderBook(orderBook);
                break;
            case 5:
                std::cout << "Exiting...\n";
                return 0;
            default:
                std::cout << "Invalid choice. Please try again.\n";
                break;
        }
    }
    
    return 0;
}
