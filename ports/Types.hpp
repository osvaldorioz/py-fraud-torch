#pragma once
#include <string>

#ifndef TYPES_HPP
#define TYPES_HPP

struct Transaction {
    int idClient;
    std::string datetime;
    float amount;
    float latitude;
    float longitude;
    std::string city;
};

struct Alert {
    int idClient;
    std::string datetime;
    float amount;
    std::string city;
    std::string reason;
    float mse;          
    float mse_threshold; 
};

#endif // TYPES_HPP