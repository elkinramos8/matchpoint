// Usuario.h
// Clase base abstracta de todos los usuarios (HERENCIA + POLIMORFISMO).
#pragma once
#include "Billetera.h"
#include <numeric>
#include <string>
#include <vector>

class Usuario {
protected:
    int id = 0;
    std::string nombre;
    std::string correo;
    std::string claveHash;
    std::vector<int> calificaciones;   // estrellas recibidas (0 a 5): REPUTACIÓN BIDIRECCIONAL
    std::string tarjeta;    // 4 últimos dígitos de la tarjeta guardada (pago simulado)
    Billetera billetera;    // historial de pagos con sus comprobantes

public:
    Usuario(std::string nombre, std::string correo, std::string claveHash)
        : nombre(std::move(nombre)), correo(std::move(correo)), claveHash(std::move(claveHash)) {}
    virtual ~Usuario() = default;

    // Métodos que cada tipo de usuario redefine
    virtual std::string getRol() const = 0;
    virtual bool puedeReservar() const = 0;
    virtual bool puedeAdministrarCanchas() const = 0;
    virtual std::string getDetalle() const = 0;   // posición o nombre del negocio
    virtual std::string medioDePago() const = 0;  // con qué paga o dónde recibe el dinero

    int getId() const { return id; }
    void setId(int nuevo) { id = nuevo; }
    const std::string &getNombre() const { return nombre; }
    const std::string &getCorreo() const { return correo; }
    const std::string &getClaveHash() const { return claveHash; }
    // Reputación = promedio de estrellas recibidas (como en Airbnb): el dueño califica la asistencia
    // y el sistema califica solo las cancelaciones y retiros tardíos
    void recibirCalificacion(int estrellas) { calificaciones.push_back(estrellas); }
    const std::vector<int> &getCalificaciones() const { return calificaciones; }
    void restaurarCalificaciones(std::vector<int> c) { calificaciones = std::move(c); }
    bool esNuevo() const { return calificaciones.empty(); }
    double getReputacion() const {   // un usuario nuevo empieza con 5
        if (calificaciones.empty()) return 5.0;
        return std::accumulate(calificaciones.begin(), calificaciones.end(), 0.0) / calificaciones.size();
    }
    const std::string &getTarjeta() const { return tarjeta; }
    void setTarjeta(const std::string &t) { tarjeta = t; }
    Billetera &getBilletera() { return billetera; }
    const Billetera &getBilletera() const { return billetera; }

    bool verificarClave(const std::string &clave) const { return claveHash == hashClave(clave); }

    // Hash simple de la contraseña (FNV-1a de 64 bits): nunca se guarda la clave
    // en texto plano y da el mismo resultado en Windows, Linux y macOS.
    static std::string hashClave(const std::string &clave) {
        unsigned long long h = 1469598103934665603ULL;
        for (unsigned char c : "matchpoint#" + clave) {
            h ^= c;
            h *= 1099511628211ULL;
        }
        return std::to_string(h);
    }
};
