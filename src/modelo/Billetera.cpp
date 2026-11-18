// Billetera.cpp
#include "Billetera.h"

void Billetera::registrarIngreso(const Dinero &monto, const std::string &concepto, const FechaHora &fecha,
                                 const std::string &comprobante) {
    saldo += monto;
    historial.push_back({fecha, concepto, monto, comprobante});
}

void Billetera::registrarPago(const Dinero &monto, const std::string &concepto, const FechaHora &fecha,
                              const std::string &comprobante) {
    saldo -= monto;
    historial.push_back({fecha, concepto, Dinero() - monto, comprobante});
}
