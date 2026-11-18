// Dinero.h
// Monto en soles guardado en céntimos (entero) para evitar errores de redondeo.
// Usa sobrecarga de operadores para que los cálculos se lean de forma natural.
#pragma once
#include <ostream>
#include <string>

class Dinero {
private:
    long long centimos;

public:
    Dinero() : centimos(0) {}
    explicit Dinero(double soles);
    static Dinero desdeCentimos(long long c);

    long long getCentimos() const { return centimos; }
    double enSoles() const { return centimos / 100.0; }
    std::string texto() const;   // "S/ 12.50"

    // Sobrecarga de operadores
    Dinero operator+(const Dinero &otro) const;
    Dinero operator-(const Dinero &otro) const;
    Dinero operator*(double factor) const;   // aplicar recargos o multas
    Dinero operator/(int partes) const;      // dividir el pago entre jugadores
    Dinero &operator+=(const Dinero &otro);
    Dinero &operator-=(const Dinero &otro);
    bool operator==(const Dinero &otro) const { return centimos == otro.centimos; }
    bool operator!=(const Dinero &otro) const { return centimos != otro.centimos; }
    bool operator<(const Dinero &otro) const { return centimos < otro.centimos; }
    bool operator<=(const Dinero &otro) const { return centimos <= otro.centimos; }
    bool operator>(const Dinero &otro) const { return centimos > otro.centimos; }
};

std::ostream &operator<<(std::ostream &os, const Dinero &d);
