// Reserva.cpp
#include "Reserva.h"
#include "Excepciones.h"

std::string textoEstado(EstadoReserva e) {
    switch (e) {
        case EstadoReserva::ABIERTA: return "Buscando jugadores";
        case EstadoReserva::COMPLETA: return "Confirmada";
        case EstadoReserva::JUGADA: return "Jugada";
        case EstadoReserva::NO_SE_PRESENTO: return "No se presentó";
        case EstadoReserva::CANCELADA: return "Cancelada";
    }
    return "?";
}

EstadoReserva estadoDesdeTexto(const std::string &t) {
    for (auto e : {EstadoReserva::ABIERTA, EstadoReserva::COMPLETA, EstadoReserva::JUGADA,
                   EstadoReserva::NO_SE_PRESENTO, EstadoReserva::CANCELADA})
        if (textoEstado(e) == t) return e;
    throw ValidacionError("Estado de reserva desconocido: " + t);
}

Reserva::Reserva(int canchaId, int organizadorId, FechaHora inicio, int grupo, int buscados,
                 Dinero precioBase, const std::vector<std::string> &extras)
    : canchaId(canchaId), organizadorId(organizadorId), inicio(inicio), grupo(grupo), buscados(buscados),
      servicio(construirServicio(precioBase, extras)), precioBase(precioBase),
      participantes{organizadorId},
      estado(buscados == 0 ? EstadoReserva::COMPLETA : EstadoReserva::ABIERTA) {
    int maximo = Configuracion::obtenerInstancia().maxJugadoresPorPartido();
    if (grupo < 1 || buscados < 0 || grupo + buscados > maximo)
        throw ValidacionError("Entre tu grupo y los que buscas deben ser de 1 a " + std::to_string(maximo) + " jugadores");
}

bool Reserva::tieneParticipante(int jugadorId) const {
    return std::find(participantes.begin(), participantes.end(), jugadorId) != participantes.end();
}

bool Reserva::yaCalifico(int jugadorId) const {
    return std::find(calificaron.begin(), calificaron.end(), jugadorId) != calificaron.end();
}

void Reserva::agregarParticipante(int jugadorId) {
    if (lugaresLibres() <= 0) throw ReservaError("El partido ya está completo");
    participantes.push_back(jugadorId);
    if (lugaresLibres() == 0) estado = EstadoReserva::COMPLETA;
}

void Reserva::quitarParticipante(int jugadorId) {
    if (esContraRival() && jugadorId != organizadorId) equipoRivalId = -1;   // se retiró el rival
    participantes.erase(std::remove(participantes.begin(), participantes.end(), jugadorId), participantes.end());
    // Si se libera un cupo, el partido vuelve a buscar jugadores
    if (estado == EstadoReserva::COMPLETA && lugaresLibres() > 0) estado = EstadoReserva::ABIERTA;
}

bool Reserva::tieneSolicitud(int jugadorId) const {
    return std::find(solicitudes.begin(), solicitudes.end(), jugadorId) != solicitudes.end();
}

void Reserva::quitarSolicitud(int jugadorId) {
    equipoDeSolicitud.erase(jugadorId);
    solicitudes.erase(std::remove(solicitudes.begin(), solicitudes.end(), jugadorId), solicitudes.end());
}
