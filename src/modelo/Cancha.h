// Cancha.h
#pragma once
#include "Dinero.h"
#include <string>
#include <vector>

class Cancha {
private:
    int id = 0;
    int duenoId;
    std::string nombre, distrito, direccion;
    Dinero precioHora;
    int horaApertura, horaCierre;  // por ejemplo 8 y 23: último turno 22:00
    std::vector<int> calificaciones;

public:
    Cancha(int duenoId, std::string nombre, std::string distrito, std::string direccion,
           Dinero precioHora, int horaApertura, int horaCierre);

    int getId() const { return id; }
    void setId(int nuevo) { id = nuevo; }
    int getDuenoId() const { return duenoId; }
    const std::string &getNombre() const { return nombre; }
    const std::string &getDistrito() const { return distrito; }
    const std::string &getDireccion() const { return direccion; }
    Dinero getPrecioHora() const { return precioHora; }
    int getHoraApertura() const { return horaApertura; }
    int getHoraCierre() const { return horaCierre; }
    const std::vector<int> &getCalificaciones() const { return calificaciones; }

    void actualizar(const std::string &nuevoNombre, const std::string &nuevoDistrito,
                    const std::string &nuevaDireccion, Dinero nuevoPrecio, int apertura, int cierre);
    void agregarCalificacion(int estrellas);
    double promedioCalificacion() const;
    bool atiendeALas(int hora) const { return hora >= horaApertura && hora < horaCierre; }

    // Sobrecarga: una cancha es "menor" si está peor calificada (para el ranking)
    bool operator<(const Cancha &otra) const;
};
