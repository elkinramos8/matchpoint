// Persistencia.h
// Convierte el Modelo en una "foto" (una línea por registro: TIPO|campo|campo...) y lo reconstruye.
// No usa Qt: se puede probar sin ventanas. La foto se guarda en SQLite (BaseDatos).
#pragma once
#include "../modelo/ModeloMatchPoint.h"
#include <string>
#include <vector>

class Persistencia {
public:
    using Registro = std::vector<std::string>;   // {"U", "1", "Jugador", "Elkin Ramos", ...}

    static std::string serializar(ModeloMatchPoint &modelo);
    static void cargar(ModeloMatchPoint &modelo, const std::string &texto);

    static std::vector<Registro> aRegistros(const std::string &texto);
    static void cargarRegistros(ModeloMatchPoint &modelo, const std::vector<Registro> &registros);
};
