#pragma once
#include <vector>
#include <string>
#include <map>
#include <torch/script.h>
#include "FraudDetectionPort.hpp"
#include "Types.hpp"

class FraudDetector : public FraudDetectionPort {
public:
    FraudDetector(const std::string& modelPath);
    std::vector<Alert> detectFraud(const std::vector<Transaction>& transactions) override;

private:
    torch::jit::Module model;
    struct ClientStats {
        float mean_amount, std_amount;
        float mean_distance, std_distance;
    };
    std::map<int, std::pair<float, float>> computePrimaryLocations(const std::vector<Transaction>& transactions);
    std::map<int, std::string> computePrimaryCity(const std::vector<Transaction>& transactions);
    std::map<int, ClientStats> computeClientStats(const std::vector<Transaction>& transactions,
                                                 const std::map<int, std::pair<float, float>>& primaryLocations);
    float haversine(float lat1, float lon1, float lat2, float lon2);
    torch::Tensor preprocess(const std::vector<Transaction>& transactions,
                            const std::map<int, std::pair<float, float>>& primaryLocations,
                            const std::map<int, std::string>& primaryCities,
                            const std::map<int, ClientStats>& clientStats);
};