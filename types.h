/**
 * Strong Type Definitions
 * Provides type-safe wrappers for Price and Quantity to prevent mixing of different concepts
 */

#pragma once

#include <iostream>
#include <stdexcept>
#include <cstdint>

/**
 * Strong type wrapper for price values
 * Prevents accidental mixing with quantity or other numeric types
 */
class Price {
private:
    std::int32_t value_;
    
public:
    explicit Price(std::int32_t value) : value_(value) {
        if (value <= 0) {
            throw std::invalid_argument("Price must be positive");
        }
    }
    
    // Explicit conversion to underlying type
    std::int32_t get() const { return value_; }
    
    // Comparison operators
    bool operator==(const Price& other) const { return value_ == other.value_; }
    bool operator!=(const Price& other) const { return value_ != other.value_; }
    bool operator<(const Price& other) const { return value_ < other.value_; }
    bool operator<=(const Price& other) const { return value_ <= other.value_; }
    bool operator>(const Price& other) const { return value_ > other.value_; }
    bool operator>=(const Price& other) const { return value_ >= other.value_; }
    
    // Arithmetic operators
    Price operator+(const Price& other) const { return Price(value_ + other.value_); }
    Price operator-(const Price& other) const { return Price(value_ - other.value_); }
    
    // Stream operators
    friend std::ostream& operator<<(std::ostream& os, const Price& price) {
        return os << price.value_;
    }
    friend std::istream& operator>>(std::istream& is, Price& price) {
        std::int32_t val;
        is >> val;
        price = Price(val);
        return is;
    }
};

/**
 * Strong type wrapper for quantity values
 * Prevents accidental mixing with price or other numeric types
 */
class Quantity {
private:
    std::uint32_t value_;
    
public:
    explicit Quantity(std::uint32_t value) : value_(value) {
        if (value == 0) {
            throw std::invalid_argument("Quantity must be positive");
        }
    }
    
    // Explicit conversion to underlying type
    std::uint32_t get() const { return value_; }
    
    // Comparison operators
    bool operator==(const Quantity& other) const { return value_ == other.value_; }
    bool operator!=(const Quantity& other) const { return value_ != other.value_; }
    bool operator<(const Quantity& other) const { return value_ < other.value_; }
    bool operator<=(const Quantity& other) const { return value_ <= other.value_; }
    bool operator>(const Quantity& other) const { return value_ > other.value_; }
    bool operator>=(const Quantity& other) const { return value_ >= other.value_; }
    
    // Arithmetic operators
    Quantity operator+(const Quantity& other) const { return Quantity(value_ + other.value_); }
    Quantity operator-(const Quantity& other) const { return Quantity(value_ - other.value_); }
    Quantity& operator+=(const Quantity& other) { 
        value_ += other.value_; 
        return *this; 
    }
    Quantity& operator-=(const Quantity& other) { 
        value_ -= other.value_; 
        return *this; 
    }
    
    // Stream operators
    friend std::ostream& operator<<(std::ostream& os, const Quantity& qty) {
        return os << qty.value_;
    }
    friend std::istream& operator>>(std::istream& is, Quantity& qty) {
        std::uint32_t val;
        is >> val;
        qty = Quantity(val);
        return is;
    }
};

// Keep OrderId as alias for now - it's less likely to be confused with other types
using OrderId = std::uint64_t;
