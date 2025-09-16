/**
 * Naive Order Book Implementation
 * A price-time priority matching engine with comprehensive order lifecycle management
 */

#include <iostream>     // Console I/O for debug output
#include <list>         // FIFO order queues at each price level
#include <map>          // Sorted price levels (bids descending, asks ascending)
#include <unordered_map> // Fast O(1) order ID lookups
#include <vector>       // Trade collections and order book snapshots
#include <numeric>      // Quantity aggregation for level summaries

/**
 * Order lifecycle behavior types
 */
enum class OrderType
{
    GTC, //good till cancelled - remains active until explicitly cancelled or filled
    FOK //fill or kill - must execute immediately and completely or be rejected
};

/**
 * Market side designation
 */
enum class OrderSide
{
    BUY,  // Bid - willing to purchase at price or better
    SELL  // Ask - willing to sell at price or better
};

// Core type definitions for type safety and clarity
using Price = std::int32_t;      // Price in smallest currency unit (e.g. cents)
using Quantity = std::uint32_t;  // Number of shares/units
using OrderId = std::uint64_t;   // Unique order identifier

/**
 * Aggregated price level for market data snapshots
 */
struct OrderBookLevel
{
    Price price_;       // Price level
    Quantity quantity_; // Total quantity available at this price
};

using OrderBookLevels = std::vector<OrderBookLevel>;

/**
 * Order book snapshot containing aggregated bid/ask levels
 * Used for market data distribution and book visualization
 */
class OrderBookBAA
{
    public:
    OrderBookBAA(const OrderBookLevels& bids, const OrderBookLevels& asks ):
    bids_{bids},
    asks_{asks}
    {}

    private:
    OrderBookLevels bids_; // Bid levels (highest to lowest price)
    OrderBookLevels asks_; // Ask levels (lowest to highest price)
};

/**
 * Individual order with partial fill tracking
 * Immutable after creation except for quantity fills
 */
class Order
{
    public:
    Order(OrderId id, OrderSide side, OrderType type, Price price, Quantity quantity):
    id_{id},
    side_{side},
    type_{type},
    price_{price},
    initialQuantity_{quantity},
    remainingQuantity_{quantity}
    {}
    
    // Accessors for order properties
    OrderId getOrderId() const { return id_; }
    OrderSide getOrderSide() const { return side_; }
    OrderType getOrderType() const { return type_; }
    Price getPrice() const { return price_; }
    Quantity getInitialQuantity() const { return initialQuantity_; }
    Quantity getRemainingQuantity() const { return remainingQuantity_; }
    Quantity getFilledQuantity() const { return initialQuantity_ - remainingQuantity_; }
    bool isFilled() const { return remainingQuantity_ == 0; }

    /**
     * Execute partial or complete fill against this order
     * @param quantity Amount to fill (must not exceed remaining quantity)
     * @throws std::invalid_argument if quantity exceeds remaining
     */
    void fill(Quantity quantity)
    {
        if (quantity > remainingQuantity_)
        {
            throw std::invalid_argument("Quantity to fill is greater than remaining quantity");
        }
        remainingQuantity_ -= quantity;
    }

    private:
    OrderId id_;                    // Unique identifier
    OrderSide side_;                // BUY or SELL
    OrderType type_;                // GTC or FOK
    Price price_;                   // Limit price
    Quantity initialQuantity_;      // Original order size
    Quantity remainingQuantity_;    // Unfilled portion
};

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>; //FIFO queue - could change to vector, will keep as list for now

/**
 * Order modification request containing new order parameters
 * Used to replace existing orders while preserving original order type
 */
class OrderModifier
{
    public:
    OrderModifier(OrderId id, OrderSide side, OrderType type, Price price, Quantity quantity):
    id_{id},
    side_{side},
    type_{type},
    price_{price},
    quantity_{quantity}
    {}
    
    // Accessors for modification parameters
    OrderId getOrderId() const { return id_; }
    OrderSide getOrderSide() const { return side_; }
    OrderType getOrderType() const { return type_; }
    Price getPrice() const { return price_; }
    Quantity getQuantity() const { return quantity_; }

    /**
     * Convert modification request to new Order instance
     * @param type Order type from existing order (preserves original type)
     * @return New order with modified parameters
     */
    OrderPointer toOrderPointer(OrderType type) const
    {
        return std::make_shared<Order>(getOrderId(), getOrderSide(), type, getPrice(), getQuantity());
    }

    private:
    OrderId id_;        // Order to modify
    OrderSide side_;    // New side
    OrderType type_;    // New type (may be overridden)
    Price price_;       // New price
    Quantity quantity_; // New quantity
};

/**
 * Trade execution details for one side of a matched trade
 */
struct TradeInfo
{
    OrderId orderId_;   // Order that participated in trade
    Price price_;       // Execution price
    Quantity quantity_; // Executed quantity
};

/**
 * Completed trade between two orders
 * Contains execution details for both bid and ask sides
 */
class Trade
{
    public:
    Trade(const TradeInfo& bid, const TradeInfo& ask):
    bid_{bid},
    ask_{ask}
    {}

    const TradeInfo& getBid() const { return bid_; }
    const TradeInfo& getAsk() const { return ask_; }

    private:
    TradeInfo bid_;  // Buyer side trade details
    TradeInfo ask_;  // Seller side trade details
};

using Trades = std::vector<Trade>;

/**
 * Central limit order book with price-time priority matching
 * Maintains separate bid/ask price levels with FIFO ordering within levels
 */
class OrderBook
{
    private:
    /**
     * Internal order tracking structure
     * Links orders to their position in price level queues
     */
    struct OrderEntry
    {
        OrderPointer order_{nullptr};        // Shared pointer to order
        OrderPointers::iterator location;    // Iterator to order's position in price level
    };

    // Core data structures for order book
    std::map<Price, OrderPointers, std::greater<Price>> bids_;  // Bids: highest price first
    std::map<Price, OrderPointers, std::less<Price>> asks_;     // Asks: lowest price first  
    std::unordered_map<OrderId, OrderEntry> orders_;           // Fast order ID lookup

    /**
     * Check if an order can potentially match against opposite side
     * @param side Order side (BUY or SELL)
     * @param price Order limit price
     * @return true if matching is possible, false otherwise
     */
    bool canMatch(OrderSide side, Price price) const
    {
        std::cout << "[CANMATCH] Checking if order can match - Side: " 
                  << (side == OrderSide::BUY ? "BUY" : "SELL") 
                  << ", Price: " << price << "\n";
        
        if (side == OrderSide::BUY){
            if (asks_.empty())
            {
                std::cout << "[CANMATCH] No asks available - cannot match BUY order" << "\n";
                return false;
            }
            bool canMatch = (price >= asks_.begin()->first);
            std::cout << "[CANMATCH] BUY order @ " << price << " vs best ask @ " 
                      << asks_.begin()->first << " - " 
                      << (canMatch ? "CAN MATCH" : "CANNOT MATCH") << "\n";
            return canMatch; 
        }else{
            if (bids_.empty())
            {
                std::cout << "[CANMATCH] No bids available - cannot match SELL order" << "\n";
                return false;
            }
            bool canMatch = price <= bids_.begin()->first;
            std::cout << "[CANMATCH] SELL order @ " << price << " vs best bid @ " 
                      << bids_.begin()->first << " - " 
                      << (canMatch ? "CAN MATCH" : "CANNOT MATCH") << "\n";
            return canMatch; 
        }
    }

    /**
     * Execute all possible trades using price-time priority matching
     * Continues until no more matches possible or FOK orders need cancellation
     * @return Vector of executed trades
     */
    Trades matchOrders()
    {
        std::cout << "[MATCHORDERS] Starting order matching process..." << "\n";
        Trades trades;
        trades.reserve(orders_.size());

        while(true){

            if (bids_.empty() || asks_.empty())
            {
                std::cout << "[MATCHORDERS] No more matching possible - " 
                          << (bids_.empty() ? "no bids" : "no asks") << "\n";
                break;
            }

            auto& [bidPrice, bids] = *bids_.begin();
            auto& [askPrice, asks] = *asks_.begin();

            // Check if prices can actually match
            if (bidPrice < askPrice) {
                std::cout << "[MATCHORDERS] No price overlap - Best bid: " << bidPrice 
                          << " < Best ask: " << askPrice << " - stopping matching" << "\n";
                break;
            }

            std::cout << "[MATCHORDERS] Matching level - Best bid: " << bidPrice 
                      << " (qty: " << bids.size() << " orders), Best ask: " << askPrice 
                      << " (qty: " << asks.size() << " orders)" << "\n";

            while (bids.size() && asks.size())
            {
                auto& bid = bids.front();
                auto& ask = asks.front();

                Quantity tradeQuantity = std::min(bid->getRemainingQuantity(), ask->getRemainingQuantity());

                std::cout << "[MATCHORDERS] Executing trade - Bid Order " << bid->getOrderId() 
                          << " (remaining: " << bid->getRemainingQuantity() << ") vs Ask Order " 
                          << ask->getOrderId() << " (remaining: " << ask->getRemainingQuantity() 
                          << ") - Trade qty: " << tradeQuantity << "\n";

                bid->fill(tradeQuantity);
                ask->fill(tradeQuantity);

                trades.push_back(
                    Trade{
                        TradeInfo{bid->getOrderId(), bid->getPrice(), tradeQuantity},
                        TradeInfo{ask->getOrderId(), ask->getPrice(), tradeQuantity}
                    } 
                ); 

                if(bid->isFilled())
                {
                    std::cout << "[MATCHORDERS] Bid Order " << bid->getOrderId() << " fully filled, removing from book" << "\n";
                    OrderId bidOrderId = bid->getOrderId(); 
                    bids.pop_front();
                    orders_.erase(bidOrderId); 
                }
                if(ask->isFilled())
                {
                    std::cout << "[MATCHORDERS] Ask Order " << ask->getOrderId() << " fully filled, removing from book" << "\n";
                    OrderId askOrderId = ask->getOrderId(); 
                    asks.pop_front();
                    orders_.erase(askOrderId); 
                }

                if (bids.empty())
                {
                    std::cout << "[MATCHORDERS] All bids at price " << bidPrice << " consumed, removing price level" << "\n";
                    bids_.erase(bidPrice);
                }
                if (asks.empty())
                {
                    std::cout << "[MATCHORDERS] All asks at price " << askPrice << " consumed, removing price level" << "\n";
                    asks_.erase(askPrice);
                }


            }
        }
        // Handle unfilled FOK orders - these should be cancelled if they couldn't be fully matched
        // We need to collect FOK orders to cancel to avoid modifying containers during iteration
        std::vector<OrderId> fokOrdersToCancel;
        
        if (!bids_.empty()) {
            auto& [_, bids] = *bids_.begin();
            auto& order = bids.front();
            if (order->getOrderType() == OrderType::FOK && !order->isFilled()){
                std::cout << "[MATCHORDERS] Found unfilled FOK bid order " << order->getOrderId() << " to cancel" << "\n";
                fokOrdersToCancel.push_back(order->getOrderId());
            }
        }

        if (!asks_.empty()) {
            auto& [_, asks] = *asks_.begin();
            auto& order = asks.front();
            if (order->getOrderType() == OrderType::FOK && !order->isFilled()){
                std::cout << "[MATCHORDERS] Found unfilled FOK ask order " << order->getOrderId() << " to cancel" << "\n";
                fokOrdersToCancel.push_back(order->getOrderId());
            }
        }
        
        // Cancel FOK orders after matching is complete to avoid recursion
        for (OrderId orderId : fokOrdersToCancel) {
            std::cout << "[MATCHORDERS] Cancelling unfilled FOK order " << orderId << "\n";
            cancelOrder(orderId);
        }
        std::cout << "[MATCHORDERS] Matching complete - generated " << trades.size() << " trade(s)" << "\n";
        return trades;
        }
        public:

    /**
     * Add new order to book and attempt immediate matching
     * @param order Shared pointer to order to add
     * @return Vector of trades generated from matching
     */
    Trades addOrder(OrderPointer order)
        {
            std::cout << "[ADDORDER] Adding new order - ID: " << order->getOrderId() 
                      << ", Side: " << (order->getOrderSide() == OrderSide::BUY ? "BUY" : "SELL")
                      << ", Type: " << (order->getOrderType() == OrderType::GTC ? "GTC" : "FOK")
                      << ", Price: " << order->getPrice() 
                      << ", Quantity: " << order->getRemainingQuantity() << "\n";
            
            if (orders_.contains(order->getOrderId())){ //contains added in C++ 20
                std::cout << "[ADDORDER] Order ID " << order->getOrderId() << " already exists - rejecting" << "\n";
                return {};
            }
            if (order->getOrderType() == OrderType::FOK && !canMatch(order->getOrderSide(), order->getPrice())){
                std::cout << "[ADDORDER] FOK order cannot be matched - rejecting Order ID " << order->getOrderId() << "\n";
                return {};
            }

            OrderPointers::iterator iterator;

            if (order->getOrderSide() == OrderSide::BUY)
            {
                auto& orders = bids_[order->getPrice()];
                orders.push_back(order);
                iterator = std::next(orders.begin(), orders.size()-1);
                std::cout << "[ADDORDER] Added BUY order to bid level " << order->getPrice() 
                          << " (now " << orders.size() << " orders at this level)" << "\n";
            }
            else
            {
                auto& orders = asks_[order->getPrice()];
                orders.push_back(order);
                iterator = std::next(orders.begin(), orders.size()-1);
                std::cout << "[ADDORDER] Added SELL order to ask level " << order->getPrice() 
                          << " (now " << orders.size() << " orders at this level)" << "\n";

            }
            orders_.insert({order->getOrderId(), OrderEntry{ order, iterator}});
            
            std::cout << "[ADDORDER] Order successfully added to book, initiating matching..." << "\n";
            return matchOrders();
        }

        /**
         * Remove order from book by ID
         * @param orderId Unique identifier of order to cancel
         */
        void cancelOrder(OrderId orderId){
            if(!orders_.contains(orderId)){
                return;
            }

            const auto& [order, iterator] = orders_.at(orderId);
            // Capture necessary data before erasing from orders_
            OrderSide orderSide = order->getOrderSide();
            Price orderPrice = order->getPrice();
            auto iteratorCopy = iterator;  // Copy the iterator before orders_.erase() invalidates it
            orders_.erase(orderId);

            if(orderSide == OrderSide::SELL){
                auto& orders = asks_.at(orderPrice);
                orders.erase(iteratorCopy);  // Use the copied iterator
                if(orders.empty()){
                    asks_.erase(orderPrice);
                }

            }
            else
            {
                auto& orders = bids_.at(orderPrice);
                orders.erase(iteratorCopy);  // Use the copied iterator
                if(orders.empty()){
                    bids_.erase(orderPrice);
                }
            }
        }

        /**
         * Modify existing order by canceling and re-adding with new parameters
         * @param order OrderModifier containing new order parameters
         * @return Vector of trades generated from re-matching
         */
        Trades matchOrder(OrderModifier order)
        {
            if(!orders_.contains(order.getOrderId())){
                return{};
            }

            const auto& [existingOrder, _] = orders_.at(order.getOrderId());
            // Capture the order type before cancelling the order
            OrderType existingOrderType = existingOrder->getOrderType();
            cancelOrder(order.getOrderId());
            return addOrder(order.toOrderPointer(existingOrderType));
        }

        /**
         * Get total number of active orders in book
         * @return Count of all orders (both bids and asks)
         */
        std::size_t getSize() const {return orders_.size();}

        /**
         * Generate aggregated order book snapshot for market data
         * @return OrderBookBAA containing bid/ask level summaries
         */
        OrderBookBAA getOrderBookLevelInfos() const {
            OrderBookLevels bidlevels;
            OrderBookLevels asklevels;
            bidlevels.reserve(orders_.size());
            asklevels.reserve(orders_.size());

            auto createLevelInfos = [](Price price, const OrderPointers& orders){
                return OrderBookLevel{price, std::accumulate(orders.begin(), orders.end(), (Quantity) 0,
                    [](Quantity runningSum, const OrderPointer& order)
                        {return runningSum + order->getRemainingQuantity(); }) };

                };

            for (const auto& [price, orders] : bids_){
                bidlevels.push_back(createLevelInfos(price, orders));
            } 
            
            for (const auto& [price, orders] : asks_){
                asklevels.push_back(createLevelInfos(price, orders));
            }
            

            return OrderBookBAA {bidlevels, asklevels}; 
            }
        
};

//=============================================================================
// TESTING FRAMEWORK - Interactive console interface for order book operations
//=============================================================================

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
    std::cout << "Order side (1 for BUY, 2 for SELL): ";
    std::cin >> choice;
    return (choice == 1) ? OrderSide::BUY : OrderSide::SELL;
}

OrderType testingGetOrderType() {
    int choice;
    std::cout << "Order type (1 for GTC, 2 for FOK): ";
    std::cin >> choice;
    return (choice == 1) ? OrderType::GTC : OrderType::FOK;
}

void testingCreateOrder(OrderBook& orderBook) {
    OrderId id;
    Price price;
    Quantity quantity;
    
    std::cout << "\n--- Create New Order ---\n";
    std::cout << "Order ID: ";
    std::cin >> id;
    
    OrderSide side = testingGetOrderSide();
    OrderType type = testingGetOrderType();
    
    std::cout << "Price: ";
    std::cin >> price;
    
    std::cout << "Quantity: ";
    std::cin >> quantity;
    
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
}

void testingModifyOrder(OrderBook& orderBook) {
    OrderId id;
    Price price;
    Quantity quantity;
    
    std::cout << "\n--- Modify Existing Order ---\n";
    std::cout << "Order ID to modify: ";
    std::cin >> id;
    
    OrderSide side = testingGetOrderSide();
    OrderType type = testingGetOrderType();
    
    std::cout << "New price: ";
    std::cin >> price;
    
    std::cout << "New quantity: ";
    std::cin >> quantity;
    
    OrderModifier modifier(id, side, type, price, quantity);
    auto trades = orderBook.matchOrder(modifier);
    
    std::cout << "Order modified successfully!\n";
    if (!trades.empty()) {
        std::cout << "Generated " << trades.size() << " trade(s) from modification.\n";
    }
}

void testingCancelOrder(OrderBook& orderBook) {
    OrderId id;
    
    std::cout << "\n--- Cancel Order ---\n";
    std::cout << "Order ID to cancel: ";
    std::cin >> id;
    
    orderBook.cancelOrder(id);
    std::cout << "Order cancellation processed.\n";
}

void testingDisplayOrderBook(const OrderBook& orderBook) {
    std::cout << "\n--- Order Book Status ---\n";
    std::cout << "Total orders in book: " << orderBook.getSize() << "\n";
    
    auto levelInfos = orderBook.getOrderBookLevelInfos();
    std::cout << "\nOrder book levels created successfully.\n";
}

int main() {
    OrderBook orderBook;
    int choice;
    
    std::cout << "Welcome to the Order Book Testing Framework!\n";
    
    while (true) {
        testingDisplayMenu();
        std::cin >> choice;
        
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