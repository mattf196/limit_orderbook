/**
 * Testing Framework Module
 * Interactive console interface for order book operations
 */

#pragma once

#include "orderbook.h"

/**
 * Display the main testing menu
 */
void testingDisplayMenu();

/**
 * Get order side from user input
 * @return OrderSide enum value
 */
OrderSide testingGetOrderSide();

/**
 * Get order type from user input
 * @return OrderType enum value
 */
OrderType testingGetOrderType();

/**
 * Create a new order through interactive interface
 * @param orderBook Order book instance to add order to
 */
void testingCreateOrder(OrderBook& orderBook);

/**
 * Modify an existing order through interactive interface
 * @param orderBook Order book instance to modify order in
 */
void testingModifyOrder(OrderBook& orderBook);

/**
 * Cancel an order through interactive interface
 * @param orderBook Order book instance to cancel order from
 */
void testingCancelOrder(OrderBook& orderBook);

/**
 * Display order book status through interactive interface
 * @param orderBook Order book instance to display
 */
void testingDisplayOrderBook(const OrderBook& orderBook);
