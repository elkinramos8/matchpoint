// FechaHora.cpp
#include "FechaHora.h"
#include "Excepciones.h"
#include <cstdio>
#include <ctime>

FechaHora::FechaHora(int anio, int mes, int dia, int hora, int minuto)
    : anio(anio), mes(mes), dia(dia), hora(hora), minuto(minuto) {
    static const int diasDelMes[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool bisiesto = (anio % 4 == 0 && anio % 100 != 0) || anio % 400 == 0;
    int maximo = (mes == 2 && bisiesto) ? 29 : (mes >= 1 && mes <= 12 ? diasDelMes[mes - 1] : 0);
    if (mes < 1 || mes > 12 || dia < 1 || dia > maximo || hora < 0 || hora > 23 || minuto < 0 || minuto > 59)
        throw ValidacionError("Fecha u hora inválida");
}

static std::tm aTm(int anio, int mes, int dia, int hora, int minuto) {
    std::tm t{};
    t.tm_year = anio - 1900;
    t.tm_mon = mes - 1;
    t.tm_mday = dia;
    t.tm_hour = hora;
    t.tm_min = minuto;
    t.tm_isdst = -1;
    return t;
}

FechaHora FechaHora::ahora() {
    std::time_t t = std::time(nullptr);
    std::tm *l = std::localtime(&t);
    return FechaHora(l->tm_year + 1900, l->tm_mon + 1, l->tm_mday, l->tm_hour, l->tm_min);
}

FechaHora FechaHora::desdeTexto(const std::string &t) {
    int a, m, d, h, mi;
    if (std::sscanf(t.c_str(), "%d-%d-%d %d:%d", &a, &m, &d, &h, &mi) != 5)
        throw ValidacionError("Fecha con formato inválido: " + t);
    return FechaHora(a, m, d, h, mi);
}

std::string FechaHora::texto() const {
    char b[32];
    std::snprintf(b, sizeof(b), "%04d-%02d-%02d %02d:%02d", anio, mes, dia, hora, minuto);
    return b;
}

std::string FechaHora::textoFecha() const {
    char b[16];
    std::snprintf(b, sizeof(b), "%04d-%02d-%02d", anio, mes, dia);
    return b;
}

int FechaHora::diaSemana() const {
    std::tm t = aTm(anio, mes, dia, hora, minuto);
    std::mktime(&t);
    return t.tm_wday;
}

std::string FechaHora::nombreDia() const {
    static const char *dias[] = {"Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"};
    return dias[diaSemana()];
}

long long FechaHora::enMinutos() const {
    std::tm t = aTm(anio, mes, dia, hora, minuto);
    return static_cast<long long>(std::mktime(&t)) / 60;
}

FechaHora FechaHora::masMinutos(long long minutos) const {
    std::time_t s = static_cast<std::time_t>((enMinutos() + minutos) * 60);
    std::tm *l = std::localtime(&s);
    return FechaHora(l->tm_year + 1900, l->tm_mon + 1, l->tm_mday, l->tm_hour, l->tm_min);
}

std::ostream &operator<<(std::ostream &os, const FechaHora &f) { return os << f.texto(); }
