#include "adapters/CsvTransactionLoader.hpp"
#include "adapters/JsonAlertOutput.hpp"
#include "domain/FraudDetector.hpp"
#include <iostream>

int main() {
    try {
        CsvTransactionLoader loader;
        auto transactions = loader.loadTransactions("data/clients_transactions_v5.csv");
        
        FraudDetector detector("model/autoencoder.pt");
        auto alerts = detector.detectFraud(transactions);
        
        JsonAlertOutput output;
        output.outputAlerts(alerts, "data/alerts.json");
        
        std::cout << "Se generaron " << alerts.size() << " alertas de fraude." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}