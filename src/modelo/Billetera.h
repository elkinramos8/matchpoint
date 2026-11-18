// Billetera.h
// Historial de pagos simulados de un usuario. Cada movimiento tiene su comprobante.
// Para el dueño, el saldo son sus ingresos; para el jugador, lo que lleva gastado.
#pragma once
#include "Dinero.h"
#include "FechaHora.h"
#include <string>
#include <vector>

struct Movimiento {
    FechaHora fecha;
    std::string concepto;
    Dinero monto;              // positivo = ingreso, negativo = pago
    std::string comprobante;   // "MP-000012"
};

class Billetera {
private:
    Dinero saldo;   // ingresos - pagos
    std::vector<Movimiento> historial;

public:
    Dinero getSaldo() const { return saldo; }
    const std::vector<Movimiento> &getHistorial() const { return historial; }

    void registrarIngreso(const Dinero &monto, const std::string &concepto, const FechaHora &fecha,
                          const std::string &comprobante);
    void registrarPago(const Dinero &monto, const std::string &concepto, const FechaHora &fecha,
                       const std::string &comprobante);

    // Para cargar datos guardados
    void restaurar(const Dinero &s, std::vector<Movimiento> h) { saldo = s; historial = std::move(h); }
};
