// ServicioReserva.h
// PATRÓN DECORATOR (estructural): los extras (pelota, chalecos, árbitro)
// "envuelven" al alquiler de la cancha y le suman precio y descripción,
// sin crear una subclase por cada combinación posible.
#pragma once
#include "Configuracion.h"
#include "Dinero.h"
#include <memory>
#include <string>
#include <vector>

// Componente: lo que se cobra en una reserva
class ServicioReserva {
public:
    virtual ~ServicioReserva() = default;
    virtual Dinero precio() const = 0;
    virtual std::string descripcion() const = 0;
    virtual std::vector<std::string> extras() const = 0;
};

// Componente concreto: solo el alquiler de la cancha por una hora
class AlquilerCancha : public ServicioReserva {
private:
    Dinero precioHora;

public:
    explicit AlquilerCancha(Dinero precioHora) : precioHora(precioHora) {}
    Dinero precio() const override { return precioHora; }
    std::string descripcion() const override { return "Cancha 1 h"; }
    std::vector<std::string> extras() const override { return {}; }
};

// Decorador base: guarda el servicio que envuelve
class DecoradorExtra : public ServicioReserva {
protected:
    std::unique_ptr<ServicioReserva> envuelto;

public:
    explicit DecoradorExtra(std::unique_ptr<ServicioReserva> s) : envuelto(std::move(s)) {}
};

class ConPelota : public DecoradorExtra {
public:
    using DecoradorExtra::DecoradorExtra;
    Dinero precio() const override { return envuelto->precio() + Configuracion::obtenerInstancia().precioPelota(); }
    std::string descripcion() const override { return envuelto->descripcion() + " + pelota"; }
    std::vector<std::string> extras() const override { auto e = envuelto->extras(); e.push_back("pelota"); return e; }
};

class ConChalecos : public DecoradorExtra {
public:
    using DecoradorExtra::DecoradorExtra;
    Dinero precio() const override { return envuelto->precio() + Configuracion::obtenerInstancia().precioChalecos(); }
    std::string descripcion() const override { return envuelto->descripcion() + " + chalecos"; }
    std::vector<std::string> extras() const override { auto e = envuelto->extras(); e.push_back("chalecos"); return e; }
};

class ConArbitro : public DecoradorExtra {
public:
    using DecoradorExtra::DecoradorExtra;
    Dinero precio() const override { return envuelto->precio() + Configuracion::obtenerInstancia().precioArbitro(); }
    std::string descripcion() const override { return envuelto->descripcion() + " + árbitro"; }
    std::vector<std::string> extras() const override { auto e = envuelto->extras(); e.push_back("arbitro"); return e; }
};

// Arma la cadena de decoradores a partir de los extras elegidos
std::unique_ptr<ServicioReserva> construirServicio(Dinero precioBase, const std::vector<std::string> &extras);
