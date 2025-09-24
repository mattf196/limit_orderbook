/**
 * Testing Framework Implementation
 * Contains the implementation of interactive testing functions
 */

#include "testing_framework.h"
#include <iostream>

void testingDisplayMenu() {
    std::cout << "\n=== Order Book Testing Framework ===\n";
    std::cout << "1. Create an order\n";
    std::cout << "2. Modify an existing order\n";
    std::cout << "3. Cancel an order\n";
    std::cout << "4. Display order book\n";
    std::cout << "5. Exit\n";
    std::cout << "Choose an option (1-5): ";
}

OrderSide testingGetOrderSide() {
    int choice;
    do {
        std::cout << "Order side (1 for BUY, 2 for SELL): ";
        std::cin >> choice;
        if (std::cin.fail() || (choice != 1 && choice != 2)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid input. Please enter 1 or 2.\n";
        }
    } while (choice != 1 && choice != 2);
    return (choice == 1) ? OrderSide::BUY : OrderSide::SELL;
}

OrderType testingGetOrderType() {
    int choice;
    do {
        std::cout << "Order type (1 for GTC, 2 for FOK): ";
        std::cin >> choice;
        if (std::cin.fail() || (choice != 1 && choice != 2)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid input. Please enter 1 or 2.\n";
        }
    } while (choice != 1 && choice != 2);
    return (choice == 1) ? OrderType::GTC : OrderType::FOK;
}

void testingCreateOrder(OrderBook& orderBook) {
    OrderId id;
    Price price(1);  // Initialize with valid value
    Quantity quantity(1);  // Initialize with valid value
    
    std::cout << "\n--- Create New Order ---\n";
    
    // Get Order ID with validation
    do {
        std::cout << "Order ID: ";
        std::cin >> id;
        if (std::cin.fail() || id == 0) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid input. Please enter a positive order ID.\n";
        }
    } while (std::cin.fail() || id == 0);
    
    OrderSide side = testingGetOrderSide();
    OrderType type = testingGetOrderType();
    
    // Get Price with validation
    bool validPrice = false;
    do {
        std::cout << "Price: ";
        try {
            std::cin >> price;
            validPrice = true;
        } catch (const std::exception& e) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid input. Please enter a positive price.\n";
        }
    } while (!validPrice);
    
    // Get Quantity with validation
    bool validQuantity = false;
    do {
        std::cout << "Quantity: ";
        try {
            std::cin >> quantity;
            validQuantity = true;
        } catch (const std::exception& e) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid input. Please enter a positive quantity.\n";
        }
    } while (!validQuantity);
    
    try {
        auto order = std::make_shared<Order>(id, side, type, price, quantity);
        auto trades = orderBook.addOrder(order);
        
        std::cout << "Order created successfully!\n";
        if (!trades.empty()) {
            std::cout << "Generated " << trades.size() << " trade(s):\n";
            for (const auto& trade : trades) {
                std::cout << "  Trade: Bid Order " << trade.getBid().orderId_ 
                         << " @ " << trade.getBid().price_ 
                         << " x " << trade.getBid().quantity_
                         << " vs Ask Order " << trade.getAsk().orderId_
                         << " @ " << trade.getAsk().price_
                         << " x " << trade.getAsk().quantity_ << "\n";
            }
        }
    } catch (const std::exception& e) {
        std::cout << "Error creating order: " << e.what() << "\n";
    }
}

void testingModifyOrder(OrderBook& orderBook) {
    OrderId id;
    Price price(1);  // Initialize with valid value
    Quantity quantity(1);  // Initialize with valid value
    
    std::cout << "\n--- Modify Existing Order ---\n";
    
    // Get Order ID with validation
    do {
        std::cout << "Order ID to modify: ";
        std::cin >> id;
        if (std::cin.fail() || id == 0) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid input. Please enter a positive order ID.\n";
        }
    } while (std::cin.fail() || id == 0);
    
    // Check if order exists before asking for other parameters
    if (!orderBook.orderExists(id)) {
        std::cout << "Error: Order ID " << id << " does not exist in the order book.\n";
        return;
    }
    
    OrderSide side = testingGetOrderSide();
    OrderType type = testingGetOrderType();
    
    // Get Price with validation
    bool validPrice = false;
    do {
        std::cout << "New price: ";
        try {
            std::cin >> price;
            validPrice = true;
        } catch (const std::exception& e) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid input. Please enter a positive price.\n";
        }
    } while (!validPrice);
    
    // Get Quantity with validation
    bool validQuantity = false;
    do {
        std::cout << "New quantity: ";
        try {
            std::cin >> quantity;
            validQuantity = true;
        } catch (const std::exception& e) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid input. Please enter a positive quantity.\n";
        }
    } while (!validQuantity);
    
    try {
        OrderModifier modifier(id, side, type, price, quantity);
        auto trades = orderBook.matchOrder(modifier);
        
        std::cout << "Order modified successfully!\n";
        if (!trades.empty()) {
            std::cout << "Generated " << trades.size() << " trade(s) from modification.\n";
        }
    } catch (const std::exception& e) {
        std::cout << "Error modifying order: " << e.what() << "\n";
    }
}

void testingCancelOrder(OrderBook& orderBook) {
    OrderId id;
    
    std::cout << "\n--- Cancel Order ---\n";
    
    // Get Order ID with validation
    do {
        std::cout << "Order ID to cancel: ";
        std::cin >> id;
        if (std::cin.fail() || id == 0) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid input. Please enter a positive order ID.\n";
        }
    } while (std::cin.fail() || id == 0);
    
    // Check if order exists before attempting cancellation
    if (!orderBook.orderExists(id)) {
        std::cout << "Error: Order ID " << id << " does not exist in the order book.\n";
        return;
    }
    
    try {
        orderBook.cancelOrder(id);
        std::cout << "Order " << id << " cancelled successfully.\n";
    } catch (const std::exception& e) {
        std::cout << "Error cancelling order: " << e.what() << "\n";
    }
}

void testingDisplayOrderBook(const OrderBook& orderBook) {
    std::cout << "\n--- Order Book Status ---\n";
    std::cout << "Total orders in book: " << orderBook.getSize() << "\n";
    
    auto levelInfos = orderBook.getOrderBookLevelInfos();
    std::cout << "\nOrder book levels created successfully.\n";
}
