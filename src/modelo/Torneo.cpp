// Torneo.cpp
#include "Torneo.h"
#include "Excepciones.h"
#include <algorithm>
#include <map>

std::string textoEstadoTorneo(EstadoTorneo e) {
    switch (e) {
        case EstadoTorneo::INSCRIPCIONES: return "Inscripciones abiertas";
        case EstadoTorneo::EN_JUEGO: return "En juego";
        case EstadoTorneo::FINALIZADO: return "Finalizado";
    }
    return "?";
}

EstadoTorneo estadoTorneoDesdeTexto(const std::string &t) {
    for (auto e : {EstadoTorneo::INSCRIPCIONES, EstadoTorneo::EN_JUEGO, EstadoTorneo::FINALIZADO})
        if (textoEstadoTorneo(e) == t) return e;
    throw ValidacionError("Estado de torneo desconocido: " + t);
}

Torneo::Torneo(int duenoId, int canchaId, std::string nombre, Dinero costoInscripcion, int maxEquipos)
    : duenoId(duenoId), canchaId(canchaId), nombre(std::move(nombre)),
      costoInscripcion(costoInscripcion), maxEquipos(maxEquipos) {
    if (this->nombre.empty()) throw ValidacionError("El torneo necesita un nombre");
    if (costoInscripcion < Dinero(0.0)) throw ValidacionError("El costo de inscripción no puede ser negativo");
    if (maxEquipos < 2 || maxEquipos > 8) throw ValidacionError("El torneo admite entre 2 y 8 equipos");
}

bool Torneo::tieneEquipo(int equipoId) const {
    return std::find(equipos.begin(), equipos.end(), equipoId) != equipos.end();
}

void Torneo::inscribir(int equipoId) {
    if (estado != EstadoTorneo::INSCRIPCIONES) throw TorneoError("Las inscripciones ya están cerradas");
    if (tieneEquipo(equipoId)) throw TorneoError("El equipo ya está inscrito");
    if (static_cast<int>(equipos.size()) >= maxEquipos) throw TorneoError("El torneo ya no tiene cupos");
    equipos.push_back(equipoId);
}

void Torneo::iniciar() {
    if (estado != EstadoTorneo::INSCRIPCIONES) throw TorneoError("El torneo ya comenzó");
    if (equipos.size() < 2) throw TorneoError("Se necesitan al menos 2 equipos inscritos");
    partidos.clear();
    for (size_t i = 0; i < equipos.size(); ++i)
        for (size_t j = i + 1; j < equipos.size(); ++j)
            partidos.push_back({equipos[i], equipos[j]});
    estado = EstadoTorneo::EN_JUEGO;
}

void Torneo::registrarResultado(int indicePartido, int golesLocal, int golesVisitante) {
    if (estado != EstadoTorneo::EN_JUEGO) throw TorneoError("El torneo no está en juego");
    if (indicePartido < 0 || indicePartido >= static_cast<int>(partidos.size()))
        throw TorneoError("El partido no existe");
    if (golesLocal < 0 || golesVisitante < 0) throw ValidacionError("Los goles no pueden ser negativos");
    PartidoTorneo &p = partidos[indicePartido];
    if (p.jugado) throw TorneoError("Ese partido ya tiene resultado");
    p.golesLocal = golesLocal;
    p.golesVisitante = golesVisitante;
    p.jugado = true;
    bool todos = std::all_of(partidos.begin(), partidos.end(), [](const PartidoTorneo &x) { return x.jugado; });
    if (todos) estado = EstadoTorneo::FINALIZADO;
}

std::vector<FilaTabla> Torneo::tablaPosiciones() const {
    std::map<int, FilaTabla> filas;
    for (int e : equipos) filas[e].equipoId = e;
    for (const auto &p : partidos) {
        if (!p.jugado) continue;
        FilaTabla &l = filas[p.localId], &v = filas[p.visitanteId];
        l.pj++; v.pj++;
        l.gf += p.golesLocal; l.gc += p.golesVisitante;
        v.gf += p.golesVisitante; v.gc += p.golesLocal;
        if (p.golesLocal > p.golesVisitante) { l.pg++; v.pp++; }
        else if (p.golesLocal < p.golesVisitante) { v.pg++; l.pp++; }
        else { l.pe++; v.pe++; }
    }
    std::vector<FilaTabla> tabla;
    for (const auto &par : filas) tabla.push_back(par.second);
    std::sort(tabla.begin(), tabla.end(), [](const FilaTabla &a, const FilaTabla &b) { return a > b; });
    return tabla;
}
