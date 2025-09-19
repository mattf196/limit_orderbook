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
    
    // Check all bid price levels for unfilled FOK orders
    for (const auto& [price, orders] : bids_) {
        for (const auto& order : orders) {
            if (order->getOrderType() == OrderType::FOK && !order->isFilled()) {
                std::cout << "[MATCHORDERS] Found unfilled FOK bid order " << order->getOrderId() << " to cancel" << "\n";
                fokOrdersToCancel.push_back(order->getOrderId());
            }
        }
    }

    // Check all ask price levels for unfilled FOK orders
    for (const auto& [price, orders] : asks_) {
        for (const auto& order : orders) {
            if (order->getOrderType() == OrderType::FOK && !order->isFilled()) {
                std::cout << "[MATCHORDERS] Found unfilled FOK ask order " << order->getOrderId() << " to cancel" << "\n";
                fokOrdersToCancel.push_back(order->getOrderId());
            }
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
