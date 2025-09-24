/**
 * OrderBook Implementation
 * Contains the implementation of all OrderBook-related classes and methods
 */

#include "orderbook.h"

// Order class implementation
void Order::fill(Quantity quantity)
{
    if (quantity > remainingQuantity_)
    {
        throw std::invalid_argument("Quantity to fill is greater than remaining quantity");
    }
    remainingQuantity_ -= quantity;
}

// OrderModifier class implementation
OrderPointer OrderModifier::toOrderPointer(OrderType type) const
{
    return std::make_shared<Order>(getOrderId(), getOrderSide(), type, getPrice(), getQuantity());
}

// OrderBook class implementation
bool OrderBook::canMatch(OrderSide side, Price price) const
{
    std::cout << "[CANMATCH] Checking if order can match - Side: " 
              << (side == OrderSide::BUY ? "BUY" : "SELL") 
              << ", Price: " << price << "\n";
    
    // Lambda to handle both bid and ask matching logic
    auto checkMatch = [&](const auto& sideMap, const std::string& sideName, auto comparator) {
        if (sideMap.empty()) {
            std::cout << "[CANMATCH] No " << sideName << "s available - cannot match " 
                      << (side == OrderSide::BUY ? "BUY" : "SELL") << " order" << "\n";
            return false;
        }
        bool canMatch = comparator(price, sideMap.begin()->first);
        std::cout << "[CANMATCH] " << (side == OrderSide::BUY ? "BUY" : "SELL") 
                  << " order @ " << price << " vs best " << sideName << " @ " 
                  << sideMap.begin()->first << " - " 
                  << (canMatch ? "CAN MATCH" : "CANNOT MATCH") << "\n";
        return canMatch;
    };
    
    return (side == OrderSide::BUY) 
        ? checkMatch(asks_, "ask", [](Price p1, Price p2) { return p1 >= p2; })
        : checkMatch(bids_, "bid", [](Price p1, Price p2) { return p1 <= p2; });
}

Trades OrderBook::matchOrders()
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
    // Check ALL orders at all price levels, not just the first order at each level
    std::vector<OrderId> fokOrdersToCancel;
    
    // Lambda to collect unfilled FOK orders from either bid or ask side
    auto collectFokOrders = [&](const auto& sideMap, const std::string& sideName) {
        for (const auto& [price, orders] : sideMap) {
            for (const auto& order : orders) {
                if (order->getOrderType() == OrderType::FOK && !order->isFilled()) {
                    std::cout << "[MATCHORDERS] Found unfilled FOK " << sideName << " order " 
                              << order->getOrderId() << " to cancel" << "\n";
                    fokOrdersToCancel.push_back(order->getOrderId());
                }
            }
        }
    };

    collectFokOrders(bids_, "bid");
    collectFokOrders(asks_, "ask");
    
    // Cancel FOK orders after matching is complete to avoid recursion
    for (OrderId orderId : fokOrdersToCancel) {
        std::cout << "[MATCHORDERS] Cancelling unfilled FOK order " << orderId << "\n";
        cancelOrder(orderId);
    }
    std::cout << "[MATCHORDERS] Matching complete - generated " << trades.size() << " trade(s)" << "\n";
    return trades;
}

Trades OrderBook::addOrder(OrderPointer order)
{
    std::cout << "[ADDORDER] Adding new order - ID: " << order->getOrderId() 
              << ", Side: " << (order->getOrderSide() == OrderSide::BUY ? "BUY" : "SELL")
              << ", Type: " << (order->getOrderType() == OrderType::GTC ? "GTC" : "FOK")
              << ", Price: " << order->getPrice() 
              << ", Quantity: " << order->getRemainingQuantity() << "\n";
    
    if (orders_.find(order->getOrderId()) != orders_.end()){ // C++17 compatible
        std::cout << "[ADDORDER] Order ID " << order->getOrderId() << " already exists - rejecting" << "\n";
        return {};
    }
    if (order->getOrderType() == OrderType::FOK && !canMatch(order->getOrderSide(), order->getPrice())){
        std::cout << "[ADDORDER] FOK order cannot be matched - rejecting Order ID " << order->getOrderId() << "\n";
        return {};
    }

    // Lambda to handle adding orders to either bid or ask side
    auto addToSide = [&](auto& sideMap, const std::string& sideName) -> OrderPointers::iterator {
        auto& orders = sideMap[order->getPrice()];
        orders.push_back(order);
        auto iterator = std::next(orders.begin(), orders.size()-1);
        std::cout << "[ADDORDER] Added " << sideName << " order to " << sideName << " level " 
                  << order->getPrice() << " (now " << orders.size() << " orders at this level)" << "\n";
        return iterator;
    };

    OrderPointers::iterator iterator = (order->getOrderSide() == OrderSide::BUY)
        ? addToSide(bids_, "BUY")
        : addToSide(asks_, "SELL");
    orders_.insert({order->getOrderId(), OrderEntry{ order, iterator}});
    
    std::cout << "[ADDORDER] Order successfully added to book, initiating matching..." << "\n";
    return matchOrders();
}

void OrderBook::cancelOrder(OrderId orderId){
    if(orders_.find(orderId) == orders_.end()){
        return;
    }

    const auto& [order, iterator] = orders_.at(orderId);
    // Capture necessary data before erasing from orders_
    OrderSide orderSide = order->getOrderSide();
    Price orderPrice = order->getPrice();
    auto iteratorCopy = iterator;  // Copy the iterator before orders_.erase() invalidates it
    orders_.erase(orderId);

    // Lambda to handle removing orders from either bid or ask side
    auto removeFromSide = [&](auto& sideMap) {
        auto& orders = sideMap.at(orderPrice);
        orders.erase(iteratorCopy);  // Use the copied iterator
        if(orders.empty()){
            sideMap.erase(orderPrice);
        }
    };

    (orderSide == OrderSide::SELL) ? removeFromSide(asks_) : removeFromSide(bids_);
}

Trades OrderBook::matchOrder(OrderModifier order)
{
    if(orders_.find(order.getOrderId()) == orders_.end()){
        return{};
    }

    const auto& [existingOrder, _] = orders_.at(order.getOrderId());
    // Capture the order type before cancelling the order
    OrderType existingOrderType = existingOrder->getOrderType();
    cancelOrder(order.getOrderId());
    return addOrder(order.toOrderPointer(existingOrderType));
}

OrderBookBAA OrderBook::getOrderBookLevelInfos() const {
    OrderBookLevels bidlevels;
    OrderBookLevels asklevels;
    bidlevels.reserve(orders_.size());
    asklevels.reserve(orders_.size());

        auto createLevelInfos = [](Price price, const OrderPointers& orders){
        std::uint32_t totalQty = std::accumulate(orders.begin(), orders.end(), 0u,
            [](std::uint32_t runningSum, const OrderPointer& order)
                {return runningSum + order->getRemainingQuantity().get(); });
        // Handle case where totalQty is 0 (no orders at this level)
        if (totalQty == 0) {
            return OrderBookLevel{price, Quantity(1)}; // Use minimum valid quantity
        }
        return OrderBookLevel{price, Quantity(totalQty)};
            };

    for (const auto& [price, orders] : bids_){
        bidlevels.push_back(createLevelInfos(price, orders));
    } 
    
    for (const auto& [price, orders] : asks_){
        asklevels.push_back(createLevelInfos(price, orders));
    }
    

    return OrderBookBAA {bidlevels, asklevels}; 
}
