/**
 * CSV Processing Module
 * Handles reading and processing CSV files containing order operations
 */

#pragma once

#include "orderbook.h"
#include <string>

/**
 * Safely convert string to numeric type with range checking
 * @param str String to convert
 * @return Converted value
 * @throws std::out_of_range if value exceeds type limits
 */
template<typename T>
T safeStringToNumber(const std::string& str);

/**
 * Process CSV file containing order operations
 * @param filename Path to CSV file
 * @param orderBook Order book instance to process orders against
 */
void processCsvFile(const std::string& filename, OrderBook& orderBook);
