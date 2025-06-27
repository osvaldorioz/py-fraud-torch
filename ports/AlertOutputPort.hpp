#pragma once
#include <vector>
#include "FraudDetectionPort.hpp"

class AlertOutputPort {
public:
    virtual ~AlertOutputPort() = default;
    virtual void outputAlerts(const std::vector<Alert>& alerts, const std::string& outputPath) = 0;
};