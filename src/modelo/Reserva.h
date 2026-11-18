// Reserva.h
// Una hora de cancha reservada por un organizador. Tres modos:
//  - Privada: "somos 10" (no busca a nadie).
//  - Buscar jugadores: "somos 6, buscamos 4"; cada jugador aceptado paga su parte.
//  - Buscar rival: el organizador reserva con su equipo y otro equipo paga la mitad.
// En los dos últimos, los interesados envían una solicitud que el organizador acepta o rechaza.
#pragma once
#include "FechaHora.h"
#include "ServicioReserva.h"
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

enum class EstadoReserva { ABIERTA, COMPLETA, JUGADA, NO_SE_PRESENTO, CANCELADA };

std::string textoEstado(EstadoReserva e);
EstadoReserva estadoDesdeTexto(const std::string &t);

class Reserva {
private:
    int id = 0;
    int canchaId, organizadorId;
    FechaHora inicio;
    int grupo;                                  // jugadores que trae el organizador (incluido él)
    int buscados;                               // jugadores que faltan y se buscan en la app
    std::unique_ptr<ServicioReserva> servicio;  // cancha + extras (Decorator)
    Dinero precioBase;                          // precio de la hora con tarifa aplicada
    std::vector<int> participantes;             // el organizador y los jugadores aceptados
    std::vector<int> solicitudes;               // jugadores (o capitanes) esperando ser aceptados
    int equipoId = -1;                          // modo "buscar rival": equipo del organizador
    int equipoRivalId = -1;                     // equipo rival aceptado
    std::map<int, int> equipoDeSolicitud;       // capitán que pidió jugar -> su equipo
    bool avisoConvocatoria = false;             // ya se avisó que el partido no se completa
    std::vector<int> calificaron;
    EstadoReserva estado;

public:
    Reserva(int canchaId, int organizadorId, FechaHora inicio, int grupo, int buscados,
            Dinero precioBase, const std::vector<std::string> &extras);

    int getId() const { return id; }
    void setId(int nuevo) { id = nuevo; }
    int getCanchaId() const { return canchaId; }
    int getOrganizadorId() const { return organizadorId; }
    const FechaHora &getInicio() const { return inicio; }
    int getGrupo() const { return grupo; }
    int getBuscados() const { return buscados; }
    int totalJugadores() const { return grupo + buscados; }
    int jugadoresConfirmados() const { return grupo + static_cast<int>(participantes.size()) - 1; }
    Dinero getPrecioBase() const { return precioBase; }
    EstadoReserva getEstado() const { return estado; }
    void setEstado(EstadoReserva e) { estado = e; }
    const std::vector<int> &getParticipantes() const { return participantes; }
    const std::vector<int> &getSolicitudes() const { return solicitudes; }
    const std::vector<int> &getCalificaron() const { return calificaron; }

    Dinero total() const { return servicio->precio(); }
    Dinero parte() const { return total() / totalJugadores(); }   // lo que paga cada jugador
    std::string descripcion() const { return servicio->descripcion(); }
    std::vector<std::string> extras() const { return servicio->extras(); }

    int lugaresLibres() const { return totalJugadores() - jugadoresConfirmados(); }
    bool buscaJugadores() const { return estado == EstadoReserva::ABIERTA; }
    bool esContraRival() const { return equipoId >= 0; }
    // Partido completo que vino de una convocatoria: las nuevas solicitudes quedan en LISTA DE ESPERA
    bool admiteListaDeEspera() const { return estado == EstadoReserva::COMPLETA && buscados > 0; }
    int puestoEnEspera(int jugadorId) const {   // 1, 2, 3... o 0 si no está
        for (size_t i = 0; i < solicitudes.size(); ++i) if (solicitudes[i] == jugadorId) return static_cast<int>(i) + 1;
        return 0;
    }
    // El organizador decide jugar con los que hay: los cupos vacíos pasan a ser suyos (ya los pagó)
    void cubrirCuposVacios() {
        int libres = lugaresLibres();
        grupo += libres;
        buscados -= libres;
        if (estado == EstadoReserva::ABIERTA) estado = EstadoReserva::COMPLETA;
    }
    bool getAvisoConvocatoria() const { return avisoConvocatoria; }
    void setAvisoConvocatoria(bool v) { avisoConvocatoria = v; }
    int getEquipoId() const { return equipoId; }
    int getEquipoRivalId() const { return equipoRivalId; }
    void setEquipo(int id) { equipoId = id; }
    void setEquipoRival(int id) { equipoRivalId = id; }
    int equipoQueSolicita(int capitanId) const {
        auto it = equipoDeSolicitud.find(capitanId);
        return it == equipoDeSolicitud.end() ? -1 : it->second;
    }
    bool estaActiva() const { return estado == EstadoReserva::ABIERTA || estado == EstadoReserva::COMPLETA; }
    bool tieneParticipante(int jugadorId) const;
    bool yaCalifico(int jugadorId) const;
    bool tieneSolicitud(int jugadorId) const;

    void agregarParticipante(int jugadorId);
    void quitarParticipante(int jugadorId);
    void agregarSolicitud(int jugadorId, int equipo = -1) {
        solicitudes.push_back(jugadorId);
        if (equipo >= 0) equipoDeSolicitud[jugadorId] = equipo;
    }
    void quitarSolicitud(int jugadorId);
    std::vector<int> retirarSolicitudes() {   // las devuelve y deja la lista vacía
        std::vector<int> r;
        r.swap(solicitudes);
        equipoDeSolicitud.clear();
        return r;
    }
    void registrarCalificacion(int jugadorId) { calificaron.push_back(jugadorId); }

    // Para cargar datos guardados
    void restaurar(std::vector<int> p, std::vector<int> c, EstadoReserva e, std::vector<int> s) {
        participantes = std::move(p); calificaron = std::move(c); estado = e; solicitudes = std::move(s);
    }
};
