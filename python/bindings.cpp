#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "adapters/CsvTransactionLoader.hpp"
#include "domain/FraudDetector.hpp"

namespace py = pybind11;

PYBIND11_MODULE(fraud_detector, m) {
    py::class_<Transaction>(m, "Transaction")
        .def(py::init<>())
        .def_readwrite("idClient", &Transaction::idClient)
        .def_readwrite("datetime", &Transaction::datetime)
        .def_readwrite("amount", &Transaction::amount)
        .def_readwrite("latitude", &Transaction::latitude)
        .def_readwrite("longitude", &Transaction::longitude)
        .def_readwrite("city", &Transaction::city);

    py::class_<Alert>(m, "Alert")
        .def(py::init<>())
        .def_readwrite("idClient", &Alert::idClient)
        .def_readwrite("datetime", &Alert::datetime)
        .def_readwrite("amount", &Alert::amount)
        .def_readwrite("city", &Alert::city)
        .def_readwrite("reason", &Alert::reason)
        .def_readwrite("mse", &Alert::mse)          // Añadido
        .def_readwrite("mse_threshold", &Alert::mse_threshold); // Añadido

    py::class_<CsvTransactionLoader>(m, "CsvTransactionLoader")
        .def(py::init<>())
        .def("load_transactions", &CsvTransactionLoader::loadTransactions);

    py::class_<FraudDetector>(m, "FraudDetector")
        .def(py::init<const std::string&>())
        .def("detect_fraud", &FraudDetector::detectFraud);
}