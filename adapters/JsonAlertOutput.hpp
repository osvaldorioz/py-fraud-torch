#pragma once
#include "../ports/AlertOutputPort.hpp"
#include <nlohmann/json.hpp>

class JsonAlertOutput : public AlertOutputPort {
public:
    void outputAlerts(const std::vector<Alert>& alerts, const std::string& outputPath) override;
};