#pragma once
#include <vector>
#include "Types.hpp"

class TransactionLoaderPort {
public:
    virtual std::vector<Transaction> loadTransactions(const std::string& filePath) = 0;
    virtual ~TransactionLoaderPort() = default;
};