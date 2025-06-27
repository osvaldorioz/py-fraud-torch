#pragma once
#include <vector>
#include "Types.hpp"

class FraudDetectionPort {
public:
    virtual std::vector<Alert> detectFraud(const std::vector<Transaction>& transactions) = 0;
    virtual ~FraudDetectionPort() = default;
};