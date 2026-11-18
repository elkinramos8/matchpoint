// DuenoCancha.h
#pragma once
#include "Usuario.h"

// Dueño de cancha: publica sus canchas, recibe pagos y organiza torneos
class DuenoCancha : public Usuario {
private:
    std::string negocio;   // nombre comercial del local

public:
    DuenoCancha(std::string nombre, std::string correo, std::string claveHash, std::string negocio)
        : Usuario(std::move(nombre), std::move(correo), std::move(claveHash)),
          negocio(std::move(negocio)) {}

    std::string getRol() const override { return "Dueño de cancha"; }
    bool puedeReservar() const override { return false; }
    bool puedeAdministrarCanchas() const override { return true; }
    std::string getDetalle() const override { return negocio; }
    std::string medioDePago() const override { return "Abono en la cuenta de " + negocio; }
};
