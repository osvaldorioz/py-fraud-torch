#include "FraudDetector.hpp"
#include <algorithm>
#include <ctime>
#include <cmath>
#include <stdexcept>
#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <set>

FraudDetector::FraudDetector(const std::string& modelPath) {
    try {
        model = torch::jit::load(modelPath);
        model.eval();
    } catch (const std::exception& e) {
        throw std::runtime_error("Error al cargar el modelo: " + std::string(e.what()));
    }
}

float FraudDetector::haversine(float lat1, float lon1, float lat2, float lon2) {
    const float R = 6371000;
    float dlat = (lat2 - lat1) * M_PI / 180.0;
    float dlon = (lon2 - lon1) * M_PI / 180.0;
    lat1 = lat1 * M_PI / 180.0;
    lat2 = lat2 * M_PI / 180.0;
    float a = std::sin(dlat / 2) * std::sin(dlat / 2) +
              std::cos(lat1) * std::cos(lat2) * std::sin(dlon / 2) * std::sin(dlon / 2);
    float c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    return R * c;
}

std::map<int, std::pair<float, float>> FraudDetector::computePrimaryLocations(const std::vector<Transaction>& transactions) {
    std::map<int, std::vector<float>> lats, lons;
    for (const auto& t : transactions) {
        lats[t.idClient].push_back(t.latitude);
        lons[t.idClient].push_back(t.longitude);
    }

    std::map<int, std::pair<float, float>> primaryLocations;
    for (const auto& pair : lats) {
        auto clientId = pair.first;
        auto& lat = lats[clientId];
        auto& lon = lons[clientId];
        std::sort(lat.begin(), lat.end());
        std::sort(lon.begin(), lon.end());
        float medianLat = lat[lat.size() / 2];
        float medianLon = lon[lon.size() / 2];
        primaryLocations[clientId] = {medianLat, medianLon};
    }
    return primaryLocations;
}

std::map<int, std::string> FraudDetector::computePrimaryCity(const std::vector<Transaction>& transactions) {
    std::map<int, std::map<std::string, int>> cityCounts;
    for (const auto& t : transactions) {
        cityCounts[t.idClient][t.city]++;
    }

    std::map<int, std::string> primaryCities;
    for (const auto& pair : cityCounts) {
        auto clientId = pair.first;
        const auto& counts = pair.second;
        std::string primaryCity;
        int maxCount = 0;
        std::cout << "Client " << clientId << " city counts: ";
        for (const auto& cityPair : counts) {
            std::cout << cityPair.first << "=" << cityPair.second << ", ";
            if (cityPair.second > maxCount) {
                maxCount = cityPair.second;
                primaryCity = cityPair.first;
            }
        }
        std::cout << " -> Primary city: " << primaryCity << std::endl;
        primaryCities[clientId] = primaryCity;
    }
    return primaryCities;
}

std::map<int, FraudDetector::ClientStats> FraudDetector::computeClientStats(const std::vector<Transaction>& transactions,
                                                                           const std::map<int, std::pair<float, float>>& primaryLocations) {
    std::map<int, std::vector<float>> amounts, distances;
    for (const auto& t : transactions) {
        amounts[t.idClient].push_back(t.amount);
        float distance = haversine(t.latitude, t.longitude,
                                  primaryLocations.at(t.idClient).first,
                                  primaryLocations.at(t.idClient).second);
        distances[t.idClient].push_back(distance);
    }

    std::map<int, ClientStats> clientStats;
    for (const auto& pair : amounts) {
        auto clientId = pair.first;
        auto& amt = amounts[clientId];
        auto& dist = distances[clientId];

        float sum = 0.0f;
        for (float a : amt) sum += a;
        float mean_amount = sum / amt.size();

        float sum_sq = 0.0f;
        for (float a : amt) sum_sq += (a - mean_amount) * (a - mean_amount);
        float std_amount = std::sqrt(sum_sq / (amt.size() > 1 ? amt.size() - 1 : 1));

        sum = 0.0f;
        for (float d : dist) sum += d;
        float mean_distance = sum / dist.size();

        sum_sq = 0.0f;
        for (float d : dist) sum_sq += (d - mean_distance) * (d - mean_distance);
        float std_distance = std::sqrt(sum_sq / (dist.size() > 1 ? dist.size() - 1 : 1));

        clientStats[clientId] = {mean_amount, std_amount, mean_distance, std_distance};

        std::cout << "Client " << clientId << ": mean_amount=" << mean_amount
                  << ", std_amount=" << std_amount << ", mean_distance=" << mean_distance
                  << ", std_distance=" << std_distance << std::endl;
    }
    return clientStats;
}

torch::Tensor FraudDetector::preprocess(const std::vector<Transaction>& transactions,
                                       const std::map<int, std::pair<float, float>>& primaryLocations,
                                       const std::map<int, std::string>& primaryCities,
                                       const std::map<int, FraudDetector::ClientStats>& clientStats) {
    std::vector<float> data;
    std::set<std::string> foreignCities = {"New York", "London", "Paris", "Sydney"};
    for (const auto& t : transactions) {
        float amount = t.amount;
        float distance = haversine(t.latitude, t.longitude,
                                  primaryLocations.at(t.idClient).first,
                                  primaryLocations.at(t.idClient).second);
        
        std::tm tm = {};
        std::stringstream ss(t.datetime);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        float hour = tm.tm_hour;
        float dayOfWeek = tm.tm_wday;
        bool isForeign = (t.city != primaryCities.at(t.idClient) || foreignCities.count(t.city) > 0);
        float isForeignFloat = isForeign ? 1.0f : 0.0f;

        const auto& stats = clientStats.at(t.idClient);
        float norm_amount = (amount - stats.mean_amount) / (stats.std_amount + 1e-6);
        float norm_distance = (distance - stats.mean_distance) / (stats.std_distance + 1e-6);

        std::cout << "Transaction client=" << t.idClient << ", amount=" << amount
                  << ", norm_amount=" << norm_amount << ", distance=" << distance
                  << ", norm_distance=" << norm_distance << ", isForeign=" << isForeign
                  << ", city=" << t.city << ", primaryCity=" << primaryCities.at(t.idClient) << std::endl;

        data.insert(data.end(), {norm_amount, norm_distance, hour, dayOfWeek, isForeignFloat});
    }

    torch::Tensor tensor = torch::from_blob(data.data(), {static_cast<long>(transactions.size()), 5}).clone();
    return tensor;
}

std::vector<Alert> FraudDetector::detectFraud(const std::vector<Transaction>& transactions) {
    auto primaryLocations = computePrimaryLocations(transactions);
    auto primaryCities = computePrimaryCity(transactions);
    auto clientStats = computeClientStats(transactions, primaryLocations);
    torch::Tensor input = preprocess(transactions, primaryLocations, primaryCities, clientStats);
    
    torch::NoGradGuard no_grad;
    std::vector<torch::jit::IValue> inputs = {input};
    torch::Tensor output = model.forward(inputs).toTensor();
    torch::Tensor mse = torch::mean(torch::pow(input - output, 2), 1);
    
    auto mseSortedTuple = mse.sort();
    torch::Tensor mseSorted = std::get<0>(mseSortedTuple);
    float threshold = mseSorted[int(mseSorted.size(0) * 0.80)].item<float>(); // Percentil 80

    std::vector<Alert> alerts;
    std::set<std::string> foreignCities = {"New York", "London", "Paris", "Sydney"};
    for (size_t i = 0; i < transactions.size(); ++i) {
        const auto& stats = clientStats.at(transactions[i].idClient);
        float distance = haversine(transactions[i].latitude, transactions[i].longitude,
                                  primaryLocations.at(transactions[i].idClient).first,
                                  primaryLocations.at(transactions[i].idClient).second);
        bool isAnomaly = mse[i].item<float>() > threshold;
        bool isAmountExtreme = transactions[i].amount > stats.mean_amount + 1.5 * stats.std_amount;
        bool isDistanceExtreme = distance > stats.mean_distance + 1.5 * stats.std_distance;
        bool isForeign = (transactions[i].city != primaryCities.at(transactions[i].idClient) || foreignCities.count(transactions[i].city) > 0);

        std::cout << "Transaction " << i << ": client=" << transactions[i].idClient
                  << ", amount=" << transactions[i].amount << ", threshold_amount=" << (stats.mean_amount + 1.5 * stats.std_amount)
                  << ", distance=" << distance << ", threshold_distance=" << (stats.mean_distance + 1.5 * stats.std_distance)
                  << ", isForeign=" << isForeign << ", mse=" << mse[i].item<float>()
                  << ", mse_threshold=" << threshold << ", isAnomaly=" << isAnomaly
                  << ", isAmountExtreme=" << isAmountExtreme << ", isDistanceExtreme=" << isDistanceExtreme
                  << ", isExtreme=" << (isAmountExtreme || isDistanceExtreme || isForeign) << std::endl;

        Alert alert;
        alert.idClient = transactions[i].idClient;
        alert.datetime = transactions[i].datetime;
        alert.amount = transactions[i].amount;
        alert.city = transactions[i].city;
        alert.mse = mse[i].item<float>(); // Añadido: Guardar MSE
        alert.mse_threshold = threshold;  // Añadido: Guardar umbral

        if (isForeign) {
            alert.reason = "Foreign city detected";
            alerts.push_back(alert);
        } else if (isAnomaly) {
            alert.reason = "High reconstruction error";
            alerts.push_back(alert);
        } else if (isAmountExtreme || isDistanceExtreme) {
            alert.reason = "Extreme amount or distance";
            alerts.push_back(alert);
        }
    }
    return alerts;
}