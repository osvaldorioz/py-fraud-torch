#pragma once
#include <vector>
#include <string>
#include "TransactionLoaderPort.hpp"
#include "Types.hpp"

class CsvTransactionLoader : public TransactionLoaderPort {
public:
    std::vector<Transaction> loadTransactions(const std::string& filePath) override;
};