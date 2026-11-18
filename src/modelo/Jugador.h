// Jugador.h
#pragma once
#include "Usuario.h"

// Jugador: reserva canchas, se une a partidos y forma equipos
class Jugador : public Usuario {
private:
    std::string posicion;   // arquero, defensa, volante, delantero

public:
    Jugador(std::string nombre, std::string correo, std::string claveHash, std::string posicion)
        : Usuario(std::move(nombre), std::move(correo), std::move(claveHash)),
          posicion(std::move(posicion)) {}

    std::string getRol() const override { return "Jugador"; }
    bool puedeReservar() const override { return true; }
    bool puedeAdministrarCanchas() const override { return false; }
    std::string getDetalle() const override { return posicion; }
    std::string medioDePago() const override {
        return tarjeta.empty() ? "Sin tarjeta registrada" : "Tarjeta •••• " + tarjeta;
    }
};
