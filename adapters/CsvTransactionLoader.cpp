#include "CsvTransactionLoader.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

std::vector<Transaction> CsvTransactionLoader::loadTransactions(const std::string& filePath) {
    std::vector<Transaction> transactions;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo: " + filePath);
    }

    std::string line;
    std::getline(file, line); // Saltar cabecera
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        Transaction t;

        std::getline(ss, token, ','); t.idClient = std::stoi(token);
        std::getline(ss, t.datetime, ',');
        std::getline(ss, token, ','); t.amount = std::stof(token);
        std::getline(ss, token, ','); t.latitude = std::stof(token);
        std::getline(ss, token, ','); t.longitude = std::stof(token);
        std::getline(ss, t.city);

        transactions.push_back(t);
    }
    file.close();
    return transactions;
}