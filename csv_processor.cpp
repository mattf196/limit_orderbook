/**
 * CSV Processing Implementation
 * Contains the implementation of CSV file processing functionality
 */

#include "csv_processor.h"
#include <fstream>      // File I/O for CSV processing
#include <sstream>      // String stream for CSV parsing
#include <limits>       // For numeric limits checking

template<typename T>
T safeStringToNumber(const std::string& str) {
    if constexpr (std::is_same_v<T, std::uint64_t>) {
        unsigned long long val = std::stoull(str);
        if (val > std::numeric_limits<std::uint64_t>::max()) {
            throw std::out_of_range("Value too large for uint64_t: " + str);
        }
        return static_cast<T>(val);
    } else if constexpr (std::is_same_v<T, std::uint32_t>) {
        unsigned long val = std::stoul(str);
        if (val > std::numeric_limits<std::uint32_t>::max()) {
            throw std::out_of_range("Value too large for uint32_t: " + str);
        }
        return static_cast<T>(val);
    } else if constexpr (std::is_same_v<T, std::int32_t>) {
        int val = std::stoi(str);
        if (val < std::numeric_limits<std::int32_t>::min() || 
            val > std::numeric_limits<std::int32_t>::max()) {
            throw std::out_of_range("Value out of range for int32_t: " + str);
        }
        return static_cast<T>(val);
    }
    // Fallback for other types
    return static_cast<T>(std::stoll(str));
}

void processCsvFile(const std::string& filename, OrderBook& orderBook) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }

    std::string line;
    int lineNumber = 0;
    int totalTrades = 0;

    std::cout << "Processing CSV file: " << filename << "\n";
    std::cout << "=================================================\n";

    while (std::getline(file, line)) {
        lineNumber++;

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        std::string action, orderIdStr, side, type, priceStr, quantityStr;

        // Parse CSV: action,order_id,side,type,price,quantity
        if (!std::getline(ss, action, ',') ||
            !std::getline(ss, orderIdStr, ',')) {
            std::cerr << "Error parsing line " << lineNumber << ": " << line << std::endl;
            continue;
        }

        // For CANCEL operations, we only need action and order_id
        if (action != "CANCEL") {
            if (!std::getline(ss, side, ',') ||
                !std::getline(ss, type, ',') ||
                !std::getline(ss, priceStr, ',') ||
                !std::getline(ss, quantityStr, ',')) {
                std::cerr << "Error parsing line " << lineNumber << ": " << line << std::endl;
                continue;
            }
        }

        try {
            // Use range-checked conversion to prevent silent truncation
            OrderId orderId = safeStringToNumber<OrderId>(orderIdStr);

            if (action == "CREATE") {
                OrderSide orderSide = (side == "BUY") ? OrderSide::BUY : OrderSide::SELL;
                OrderType orderType = (type == "GTC") ? OrderType::GTC : OrderType::FOK;
                Price price = safeStringToNumber<Price>(priceStr);
                Quantity quantity = safeStringToNumber<Quantity>(quantityStr);

                auto order = std::make_shared<Order>(orderId, orderSide, orderType, price, quantity);
                auto trades = orderBook.addOrder(order);
                totalTrades += trades.size();

            } else if (action == "MODIFY") {
                OrderSide orderSide = (side == "BUY") ? OrderSide::BUY : OrderSide::SELL;
                OrderType orderType = (type == "GTC") ? OrderType::GTC : OrderType::FOK;
                Price price = safeStringToNumber<Price>(priceStr);
                Quantity quantity = safeStringToNumber<Quantity>(quantityStr);

                OrderModifier modifier(orderId, orderSide, orderType, price, quantity);
                auto trades = orderBook.matchOrder(modifier);
                totalTrades += trades.size();

            } else if (action == "CANCEL") {
                orderBook.cancelOrder(orderId);

            } else {
                std::cerr << "Unknown action '" << action << "' on line " << lineNumber << std::endl;
            }

        } catch (const std::exception& e) {
            std::cerr << "Error processing line " << lineNumber << ": " << e.what() << std::endl;
        }
    }

    std::cout << "=================================================\n";
    std::cout << "CSV Processing Complete!\n";
    std::cout << "Lines processed: " << lineNumber << "\n";
    std::cout << "Total trades executed: " << totalTrades << "\n";
    std::cout << "Final order book size: " << orderBook.getSize() << " orders\n";

    file.close();
}
