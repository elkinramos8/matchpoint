// Dinero.cpp
#include "Dinero.h"
#include "Excepciones.h"
#include <cmath>
#include <cstdio>

Dinero::Dinero(double soles) : centimos(std::llround(soles * 100.0)) {}

Dinero Dinero::desdeCentimos(long long c) {
    Dinero d;
    d.centimos = c;
    return d;
}

std::string Dinero::texto() const {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "S/ %.2f", enSoles());
    return buffer;
}

Dinero Dinero::operator+(const Dinero &otro) const { return desdeCentimos(centimos + otro.centimos); }
Dinero Dinero::operator-(const Dinero &otro) const { return desdeCentimos(centimos - otro.centimos); }

Dinero Dinero::operator*(double factor) const {
    return desdeCentimos(std::llround(centimos * factor));
}

Dinero Dinero::operator/(int partes) const {
    if (partes <= 0) throw ValidacionError("No se puede dividir un monto entre " + std::to_string(partes));
    // Se redondea hacia arriba para que la suma de las partes cubra el total
    return desdeCentimos((centimos + partes - 1) / partes);
}

Dinero &Dinero::operator+=(const Dinero &otro) { centimos += otro.centimos; return *this; }
Dinero &Dinero::operator-=(const Dinero &otro) { centimos -= otro.centimos; return *this; }

std::ostream &operator<<(std::ostream &os, const Dinero &d) { return os << d.texto(); }
