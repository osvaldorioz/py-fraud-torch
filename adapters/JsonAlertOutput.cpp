#include "JsonAlertOutput.hpp"
#include <fstream>

void JsonAlertOutput::outputAlerts(const std::vector<Alert>& alerts, const std::string& outputPath) {
    nlohmann::json jsonAlerts = nlohmann::json::array();
    for (const auto& alert : alerts) {
        nlohmann::json j;
        j["idClient"] = alert.idClient;
        j["datetime"] = alert.datetime;
        j["amount"] = alert.amount;
        j["city"] = alert.city;
        jsonAlerts.push_back(j);
    }

    std::ofstream file(outputPath);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo de salida: " + outputPath);
    }
    file << jsonAlerts.dump(4);
    file.close();
}