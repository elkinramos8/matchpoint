// Persistencia.cpp
// Formato: TIPO|campo|campo|...   (los textos se escapan para admitir '|')
#include "Persistencia.h"
#include <map>
#include <sstream>

namespace {

std::string escapar(const std::string &s) {
    std::string r;
    for (char c : s) {
        if (c == '\\') r += "\\\\";
        else if (c == '|') r += "\\p";
        else if (c == '\n') r += "\\n";
        else r += c;
    }
    return r;
}

std::vector<std::string> separar(const std::string &linea) {
    std::vector<std::string> campos(1);
    for (size_t i = 0; i < linea.size(); ++i) {
        char c = linea[i];
        if (c == '|') { campos.emplace_back(); continue; }
        if (c == '\\' && i + 1 < linea.size()) {
            char s = linea[++i];
            campos.back() += (s == 'p' ? '|' : s == 'n' ? '\n' : s);
            continue;
        }
        campos.back() += c;
    }
    return campos;
}

template <typename T>
std::string unir(const std::vector<T> &v) {
    std::ostringstream os;
    for (size_t i = 0; i < v.size(); ++i) os << (i ? "," : "") << v[i];
    return os.str();
}

std::vector<int> enteros(const std::string &s) {
    std::vector<int> r;
    std::stringstream ss(s);
    std::string x;
    while (std::getline(ss, x, ',')) if (!x.empty()) r.push_back(std::stoi(x));
    return r;
}

std::vector<std::string> textos(const std::string &s) {
    std::vector<std::string> r;
    std::stringstream ss(s);
    std::string x;
    while (std::getline(ss, x, ',')) if (!x.empty()) r.push_back(x);
    return r;
}

Dinero cent(const std::string &s) { return Dinero::desdeCentimos(std::stoll(s)); }

std::string solicitudesATexto(const Reserva &r) {
    std::ostringstream os;
    for (size_t i = 0; i < r.getSolicitudes().size(); ++i) {
        int j = r.getSolicitudes()[i];
        os << (i ? "," : "") << j;
        if (r.equipoQueSolicita(j) >= 0) os << ':' << r.equipoQueSolicita(j);
    }
    return os.str();
}

} // namespace

std::string Persistencia::serializar(ModeloMatchPoint &m) {
    std::ostringstream os;
    for (Usuario *u : m.repoUsuarios().todos()) {
        os << "U|" << u->getId() << '|' << escapar(u->getRol()) << '|' << escapar(u->getNombre()) << '|'
           << escapar(u->getCorreo()) << '|' << u->getClaveHash() << '|' << escapar(u->getDetalle()) << '|'
           << unir(u->getCalificaciones()) << '|' << u->getBilletera().getSaldo().getCentimos() << '|'
           << u->getTarjeta() << '\n';
        for (const auto &mv : u->getBilletera().getHistorial())
            os << "M|" << u->getId() << '|' << mv.fecha.texto() << '|' << mv.monto.getCentimos() << '|'
               << escapar(mv.concepto) << '|' << mv.comprobante << '\n';
    }
    for (Cancha *c : m.repoCanchas().todos())
        os << "C|" << c->getId() << '|' << c->getDuenoId() << '|' << escapar(c->getNombre()) << '|'
           << escapar(c->getDistrito()) << '|' << escapar(c->getDireccion()) << '|' << c->getPrecioHora().getCentimos() << '|' << c->getHoraApertura() << '|'
           << c->getHoraCierre() << '|' << unir(c->getCalificaciones()) << '\n';
    for (Reserva *r : m.repoReservas().todos())
        os << "R|" << r->getId() << '|' << r->getCanchaId() << '|' << r->getOrganizadorId() << '|'
           << r->getInicio().texto() << '|' << r->getGrupo() << '|' << r->getBuscados() << '|'
           << r->getPrecioBase().getCentimos() << '|'
           << unir(r->extras()) << '|' << escapar(textoEstado(r->getEstado())) << '|'
           << unir(r->getParticipantes()) << '|' << unir(r->getCalificaron()) << '|'
           << solicitudesATexto(*r) << '|' << r->getEquipoId() << '|' << r->getEquipoRivalId() << '|'
           << (r->getAvisoConvocatoria() ? 1 : 0) << '\n';
    for (Equipo *e : m.repoEquipos().todos())
        os << "E|" << e->getId() << '|' << e->getCapitanId() << '|' << escapar(e->getNombre()) << '|'
           << unir(e->getTitulares().idsJugadores()) << '|' << unir(e->getSuplentes().idsJugadores()) << '\n';
    for (Torneo *t : m.repoTorneos().todos()) {
        os << "T|" << t->getId() << '|' << t->getDuenoId() << '|' << t->getCanchaId() << '|'
           << escapar(t->getNombre()) << '|' << t->getCostoInscripcion().getCentimos() << '|'
           << t->getMaxEquipos() << '|' << escapar(textoEstadoTorneo(t->getEstado())) << '|'
           << unir(t->getEquipos()) << '\n';
        for (const auto &p : t->getPartidos())
            os << "P|" << t->getId() << '|' << p.localId << '|' << p.visitanteId << '|' << p.golesLocal << '|'
               << p.golesVisitante << '|' << (p.jugado ? 1 : 0) << '\n';
    }
    os << "K|" << m.getUltimoComprobante() << '\n';   // último número de comprobante
    for (const auto &n : m.todasLasNotificaciones())
        os << "N|" << n.usuarioId << '|' << n.fecha.texto() << '|' << escapar(n.mensaje) << '|' << n.tipo << '\n';
    return os.str();
}

std::vector<Persistencia::Registro> Persistencia::aRegistros(const std::string &texto) {
    std::vector<Registro> registros;
    std::istringstream is(texto);
    std::string linea;
    while (std::getline(is, linea))
        if (!linea.empty()) registros.push_back(separar(linea));
    return registros;
}

void Persistencia::cargar(ModeloMatchPoint &m, const std::string &texto) { cargarRegistros(m, aRegistros(texto)); }

void Persistencia::cargarRegistros(ModeloMatchPoint &m, const std::vector<Registro> &registros) {
    std::map<int, std::vector<Movimiento>> movimientos;
    std::map<int, Dinero> saldos;
    std::map<int, std::vector<PartidoTorneo>> partidos;
    std::map<int, std::pair<EstadoTorneo, std::vector<int>>> datosTorneo;

    for (const Registro &f : registros) {
        if (f.empty()) continue;
        const std::string &tipo = f[0];
        if (tipo == "U" && f.size() >= 9) {
            // FACTORY METHOD también al cargar desde archivo
            auto u = UsuarioFactory::crear(UsuarioFactory::tipoDesdeTexto(f[2]), f[3], f[4], f[5], f[6]);
            u->setId(std::stoi(f[1]));
            u->restaurarCalificaciones(enteros(f[7]));
            saldos[u->getId()] = cent(f[8]);
            if (f.size() >= 10) u->setTarjeta(f[9]);
            m.repoUsuarios().restaurar(std::move(u));
        } else if (tipo == "M" && f.size() >= 5) {
            movimientos[std::stoi(f[1])].push_back({FechaHora::desdeTexto(f[2]), f[4], cent(f[3]),
                                                    f.size() >= 6 ? f[5] : ""});
        } else if (tipo == "C" && f.size() >= 10) {
            auto c = std::make_unique<Cancha>(std::stoi(f[2]), f[3], f[4], f[5], cent(f[6]), std::stoi(f[7]),
                                              std::stoi(f[8]));
            c->setId(std::stoi(f[1]));
            for (int x : enteros(f[9])) c->agregarCalificacion(x);
            m.repoCanchas().restaurar(std::move(c));
        } else if (tipo == "R" && f.size() >= 13) {
            auto r = std::make_unique<Reserva>(std::stoi(f[2]), std::stoi(f[3]), FechaHora::desdeTexto(f[4]),
                                               std::stoi(f[5]), std::stoi(f[6]), cent(f[7]), textos(f[8]));
            r->setId(std::stoi(f[1]));
            r->restaurar(enteros(f[10]), enteros(f[11]), estadoDesdeTexto(f[9]), {});
            for (const std::string &sol : textos(f[12])) {   // "capitán:equipo" o solo "jugador"
                size_t dos = sol.find(':');
                r->agregarSolicitud(std::stoi(sol.substr(0, dos)), dos == std::string::npos ? -1 : std::stoi(sol.substr(dos + 1)));
            }
            if (f.size() >= 15) {
                r->setEquipo(std::stoi(f[13]));
                r->setEquipoRival(std::stoi(f[14]));
            }
            if (f.size() >= 16) r->setAvisoConvocatoria(f[15] == "1");
            m.repoReservas().restaurar(std::move(r));
        } else if (tipo == "E" && f.size() >= 6) {
            auto e = std::make_unique<Equipo>(f[3], std::stoi(f[2]));
            e->setId(std::stoi(f[1]));
            for (int id : enteros(f[4]))
                if (Usuario *u = m.buscarUsuario(id))
                    e->getTitulares().agregar(std::make_unique<MiembroEquipo>(id, u->getNombre()));
            for (int id : enteros(f[5]))
                if (Usuario *u = m.buscarUsuario(id))
                    e->getSuplentes().agregar(std::make_unique<MiembroEquipo>(id, u->getNombre()));
            m.repoEquipos().restaurar(std::move(e));
        } else if (tipo == "T" && f.size() >= 9) {
            auto t = std::make_unique<Torneo>(std::stoi(f[2]), std::stoi(f[3]), f[4], cent(f[5]), std::stoi(f[6]));
            t->setId(std::stoi(f[1]));
            datosTorneo[t->getId()] = {estadoTorneoDesdeTexto(f[7]), enteros(f[8])};
            m.repoTorneos().restaurar(std::move(t));
        } else if (tipo == "P" && f.size() >= 7) {
            partidos[std::stoi(f[1])].push_back({std::stoi(f[2]), std::stoi(f[3]), std::stoi(f[4]),
                                                 std::stoi(f[5]), f[6] == "1"});
        } else if (tipo == "K" && f.size() >= 2) {
            m.setUltimoComprobante(std::stoi(f[1]));
        } else if (tipo == "N" && f.size() >= 4) {
            m.restaurarNotificacion({std::stoi(f[1]), FechaHora::desdeTexto(f[2]), f[3], f.size() >= 5 ? f[4] : ""});
        }
    }
    for (auto &par : saldos)
        if (Usuario *u = m.buscarUsuario(par.first)) u->getBilletera().restaurar(par.second, movimientos[par.first]);
    for (auto &par : datosTorneo)
        if (Torneo *t = m.buscarTorneo(par.first))
            t->restaurar(par.second.first, par.second.second, partidos[par.first]);
}
