// Cancha.cpp
#include "Cancha.h"
#include "Excepciones.h"
#include <numeric>

static void validar(const std::string &nombre, const std::string &distrito, Dinero precio,
                    int apertura, int cierre) {
    if (nombre.empty() || distrito.empty()) throw ValidacionError("La cancha necesita nombre y distrito");
    if (precio <= Dinero(0.0)) throw ValidacionError("El precio por hora debe ser mayor que cero");
    if (apertura < 0 || cierre > 24 || apertura >= cierre)
        throw ValidacionError("El horario de atención no es válido");
}

Cancha::Cancha(int duenoId, std::string nombre, std::string distrito, std::string direccion,
               Dinero precioHora, int horaApertura, int horaCierre)
    : duenoId(duenoId), nombre(std::move(nombre)), distrito(std::move(distrito)),
      direccion(std::move(direccion)),
      precioHora(precioHora), horaApertura(horaApertura), horaCierre(horaCierre) {
    validar(this->nombre, this->distrito, precioHora, horaApertura, horaCierre);
}

void Cancha::actualizar(const std::string &nuevoNombre, const std::string &nuevoDistrito,
                        const std::string &nuevaDireccion, Dinero nuevoPrecio, int apertura, int cierre) {
    validar(nuevoNombre, nuevoDistrito, nuevoPrecio, apertura, cierre);
    nombre = nuevoNombre;
    distrito = nuevoDistrito;
    direccion = nuevaDireccion;
    precioHora = nuevoPrecio;
    horaApertura = apertura;
    horaCierre = cierre;
}

void Cancha::agregarCalificacion(int estrellas) {
    if (estrellas < 1 || estrellas > 5) throw ValidacionError("La calificación debe estar entre 1 y 5");
    calificaciones.push_back(estrellas);
}

double Cancha::promedioCalificacion() const {
    if (calificaciones.empty()) return 0.0;
    return std::accumulate(calificaciones.begin(), calificaciones.end(), 0.0) / calificaciones.size();
}

bool Cancha::operator<(const Cancha &otra) const {
    if (promedioCalificacion() != otra.promedioCalificacion())
        return promedioCalificacion() < otra.promedioCalificacion();
    return calificaciones.size() < otra.calificaciones.size();
}
