/**
 * Naive Order Book Implementation
 * A price-time priority matching engine with comprehensive order lifecycle management
 */

#pragma once

#include <iostream>     // Console I/O for debug output
#include <list>         // FIFO order queues at each price level
#include <map>          // Sorted price levels (bids descending, asks ascending)
#include <unordered_map> // Fast O(1) order ID lookups
#include <vector>       // Trade collections and order book snapshots
#include <numeric>      // Quantity aggregation for level summaries
#include <memory>       // Smart pointers
#include "types.h"      // Strong type definitions for Price, Quantity, OrderId

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

// Strong types are now defined in types.h
// Price, Quantity, and OrderId are imported from there

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
    bool isFilled() const { return remainingQuantity_.get() == 0; }

    /**
     * Execute partial or complete fill against this order
     * @param quantity Amount to fill (must not exceed remaining quantity)
     * @throws std::invalid_argument if quantity exceeds remaining
     */
    void fill(Quantity quantity);

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
    OrderPointer toOrderPointer(OrderType type) const;

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
    bool canMatch(OrderSide side, Price price) const;

    /**
     * Execute all possible trades using price-time priority matching
     * Continues until no more matches possible or FOK orders need cancellation
     * @return Vector of executed trades
     */
    Trades matchOrders();

    public:

    /**
     * Add new order to book and attempt immediate matching
     * @param order Shared pointer to order to add
     * @return Vector of trades generated from matching
     */
    Trades addOrder(OrderPointer order);

    /**
     * Remove order from book by ID
     * @param orderId Unique identifier of order to cancel
     */
    void cancelOrder(OrderId orderId);

    /**
     * Modify existing order by canceling and re-adding with new parameters
     * @param order OrderModifier containing new order parameters
     * @return Vector of trades generated from re-matching
     */
    Trades matchOrder(OrderModifier order);

    /**
     * Get total number of active orders in book
     * @return Count of all orders (both bids and asks)
     */
    std::size_t getSize() const {return orders_.size();}

    /**
     * Check if an order with given ID exists in the book
     * @param orderId Unique identifier of order to check
     * @return true if order exists, false otherwise
     */
    bool orderExists(OrderId orderId) const { return orders_.find(orderId) != orders_.end(); }

    /**
     * Generate aggregated order book snapshot for market data
     * @return OrderBookBAA containing bid/ask level summaries
     */
    OrderBookBAA getOrderBookLevelInfos() const;
};
