// FechaHora.h
// Fecha y hora de un partido. Usa operadores para comparar y restar fechas.
#pragma once
#include <ostream>
#include <string>

class FechaHora {
private:
    int anio, mes, dia, hora, minuto;

public:
    FechaHora() : anio(2026), mes(1), dia(1), hora(0), minuto(0) {}
    FechaHora(int anio, int mes, int dia, int hora, int minuto = 0);

    static FechaHora ahora();                          // reloj del sistema
    static FechaHora desdeTexto(const std::string &t); // "2026-11-20 19:00"

    std::string texto() const;          // "2026-11-20 19:00"
    std::string textoFecha() const;     // "2026-11-20"
    std::string nombreDia() const;      // "Viernes"
    int getHora() const { return hora; }
    int diaSemana() const;              // 0 = domingo ... 6 = sábado

    long long enMinutos() const;        // minutos desde 1970 (hora local)
    FechaHora masMinutos(long long minutos) const;
    FechaHora masDias(int dias) const { return masMinutos(dias * 24LL * 60); }
    FechaHora inicioDelDia() const { return FechaHora(anio, mes, dia, 0, 0); }

    // Sobrecarga de operadores
    long long operator-(const FechaHora &otra) const { return enMinutos() - otra.enMinutos(); }
    bool operator<(const FechaHora &otra) const { return enMinutos() < otra.enMinutos(); }
    bool operator<=(const FechaHora &otra) const { return enMinutos() <= otra.enMinutos(); }
    bool operator==(const FechaHora &otra) const { return enMinutos() == otra.enMinutos(); }
};

std::ostream &operator<<(std::ostream &os, const FechaHora &f);
