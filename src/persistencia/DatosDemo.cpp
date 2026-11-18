// DatosDemo.cpp
// Datos de ejemplo para la primera ejecución. Para que la app se vea "viva" se
// simulan los últimos 6 días usando las MISMAS reglas del modelo (reservas, pagos,
// solicitudes, partidos incompletos, calificaciones del dueño, cancelaciones...):
// se adelanta un reloj simulado día por día y al final se vuelve al reloj real.
#include "DatosDemo.h"
#include <algorithm>
#include <random>

namespace {

struct Simulador {
    explicit Simulador(ModeloMatchPoint &modelo) : m(modelo) {}
    ModeloMatchPoint &m;
    std::mt19937 gen{2026};                 // semilla fija: siempre se genera lo mismo
    std::vector<Usuario *> jugadores;
    std::vector<Usuario *> impuntuales;     // faltan, llegan tarde y cancelan tarde más seguido

    double azar() { return std::uniform_real_distribution<double>(0.0, 1.0)(gen); }
    int entre(int a, int b) { return std::uniform_int_distribution<int>(a, b)(gen); }
    Usuario *jugadorAlAzar(int desde = 0) { return jugadores[entre(desde, static_cast<int>(jugadores.size()) - 1)]; }
    bool esImpuntual(int id) const {
        for (Usuario *u : impuntuales) if (u->getId() == id) return true;
        return false;
    }

    std::vector<std::string> extrasAlAzar() {
        std::vector<std::string> e;
        if (azar() < 0.6) e.push_back("pelota");
        if (azar() < 0.4) e.push_back("chalecos");
        if (azar() < 0.15) e.push_back("arbitro");
        return e;
    }

    // Reserva una hora. Si es partido abierto ("somos 7, buscamos 3") acepta jugadores
    // (casi siempre se completa) y deja algunas solicitudes más (lista de espera o pendientes)
    Reserva *reservar(Usuario *org, Cancha *c, const FechaHora &inicio, int desde, bool abierto, int pendientes) {
        int total = 10 + 2 * entre(0, 2);
        int buscados = abierto ? entre(2, 5) : 0;
        Reserva *r;
        try {
            r = m.reservar(org->getId(), c->getId(), inicio, extrasAlAzar(), total - buscados, buscados);
        } catch (const MatchPointError &) {
            return nullptr;   // horario ocupado o el organizador ya juega a esa hora: se omite
        }
        if (!abierto) return r;
        int aAceptar = azar() < 0.8 ? buscados : entre(0, buscados - 1);
        for (int intento = 0; intento < 25 && r->getParticipantes().size() < static_cast<size_t>(aAceptar) + 1; ++intento) {
            Usuario *j = jugadorAlAzar(desde);
            try {
                m.solicitarUnirse(j->getId(), r->getId());
                m.responderSolicitud(org->getId(), r->getId(), j->getId(), true);
            } catch (const MatchPointError &) {}   // ya estaba, ya había pedido o tiene otro partido
        }
        for (int i = 0; i < pendientes; ++i)
            try { m.solicitarUnirse(jugadorAlAzar(desde)->getId(), r->getId()); } catch (const MatchPointError &) {}
        return r;
    }
};

// Calidad de cada cancha (promedio aproximado de estrellas que recibe)
double calidad(const std::string &cancha) {
    if (cancha == "El Golazo") return 4.6;
    if (cancha == "La Misti") return 4.1;
    if (cancha == "El Golazo 2") return 3.8;
    return 3.1;
}

// Probabilidad de que una hora esté reservada: la noche se llena primero
double demanda(int hora) { return hora >= 18 ? 0.75 : hora >= 15 ? 0.35 : 0.12; }

} // namespace

void cargarDatosDemo(ModeloMatchPoint &m) {
    const std::string clave = "1234";
    const FechaHora hoy = m.ahora();
    FechaHora simulado = hoy.inicioDelDia().masDias(-8).masMinutos(9 * 60);
    auto relojReal = m.cambiarReloj([&simulado] { return simulado; });
    Simulador sim(m);

    // ---------- Usuarios y canchas (registrados hace 8 días) ----------
    Usuario *rosa = m.registrarUsuario("Dueño de cancha", "Rosa Huamán", "rosa@matchpoint.pe", clave, "Complejo El Golazo");
    Usuario *luis = m.registrarUsuario("Dueño de cancha", "Luis Paredes", "luis@matchpoint.pe", clave, "Sintéticos La Misti");
    Cancha *golazo = m.registrarCancha(rosa->getId(), "El Golazo", "Cerro Colorado", "Av. Aviación 512", Dinero(60.0), 8, 23);
    m.registrarCancha(rosa->getId(), "El Golazo 2", "Cerro Colorado", "Av. Aviación 520", Dinero(50.0), 8, 23);
    Cancha *misti = m.registrarCancha(luis->getId(), "La Misti", "Cayma", "Calle Bolognesi 230", Dinero(70.0), 9, 23);
    m.registrarCancha(luis->getId(), "Chachani Sport", "Yanahuara", "Av. Ejército 1020", Dinero(55.0), 10, 22);

    // Jugadores: nombre, correo, posición y los 4 últimos dígitos de su tarjeta (simulada)
    const char *datos[][4] = {
        {"Elkin Ramos", "elkin@matchpoint.pe", "volante", "4821"},
        {"Diego Schreiber", "diego@matchpoint.pe", "delantero", "1337"},
        {"Jerry Huaynacho", "jerry@matchpoint.pe", "arquero", "7702"},
        {"José Quispe", "jose@matchpoint.pe", "defensa", "5510"},
        {"Fernando Solsol", "fernando@matchpoint.pe", "volante", "9034"},
        {"Carlos Mamani", "carlos@matchpoint.pe", "defensa", "2468"},
        {"Andrea Flores", "andrea@matchpoint.pe", "delantero", "3141"},
        {"Miguel Torres", "miguel@matchpoint.pe", "arquero", "8080"},
        {"Lucía Vargas", "lucia@matchpoint.pe", "volante", "6123"},
        {"Pedro Chávez", "pedro@matchpoint.pe", "defensa", "4407"},
        {"Kevin Apaza", "kevin@matchpoint.pe", "delantero", "1290"},
        {"Luis Condori", "lcondori@matchpoint.pe", "defensa", "3375"},
        {"Marco Ccama", "marco@matchpoint.pe", "volante", "8642"},
        {"Renzo Pinto", "renzo@matchpoint.pe", "arquero", "5091"},
        {"Brayan Huamaní", "brayan@matchpoint.pe", "delantero", "7310"},
        {"Jhon Mamani", "jhon@matchpoint.pe", "defensa", "2207"},
        {"Daniel Quispe", "daniel@matchpoint.pe", "volante", "6648"},
        {"Sebastián Torres", "sebastian@matchpoint.pe", "delantero", "9921"},
        {"Gabriel Salas", "gabriel@matchpoint.pe", "defensa", "4015"},
        {"Rodrigo Flores", "rodrigo@matchpoint.pe", "volante", "3388"},
        {"Valeria Ccori", "valeria@matchpoint.pe", "delantero", "7754"},
        {"Alonso Zegarra", "alonso@matchpoint.pe", "arquero", "1862"},
        {"Piero Begazo", "piero@matchpoint.pe", "volante", "5530"},
        {"Mateo Delgado", "mateo@matchpoint.pe", "defensa", "2984"},
    };
    for (auto &j : datos) sim.jugadores.push_back(m.registrarUsuario("Jugador", j[0], j[1], clave, j[2], j[3]));
    auto &js = sim.jugadores;
    sim.impuntuales = {js[9], js[14], js[17], js[21]};   // Pedro, Brayan, Sebastián, Alonso
    std::vector<Cancha *> canchas = m.listarCanchas();

    // ---------- Historial: los últimos 6 días ----------
    for (int d = 6; d >= 1; --d) {
        FechaHora dia = hoy.inicioDelDia().masDias(-d);
        simulado = dia.masDias(-1).masMinutos(7 * 60);   // se reserva el día anterior
        std::vector<Reserva *> delDia;
        for (Cancha *c : canchas)
            for (int h = c->getHoraApertura(); h < c->getHoraCierre(); ++h) {
                if (sim.azar() >= demanda(h)) continue;
                Reserva *r = sim.reservar(sim.jugadorAlAzar(), c, dia.masMinutos(h * 60LL), 0, sim.azar() < 0.5, 0);
                if (r) delDia.push_back(r);
            }
        // 24 h antes de cada partido vence el plazo: los que no se completaron se cancelan solos
        simulado = dia.masDias(-1).masMinutos(23 * 60 + 59);
        m.revisarConvocatorias();

        // Cancelaciones y retiros a última hora (3 h antes del partido)
        for (Reserva *r : delDia) {
            if (!r->estaActiva() || r->getInicio().getHora() < 10) continue;
            simulado = r->getInicio().masMinutos(-3 * 60);
            bool impuntual = sim.esImpuntual(r->getOrganizadorId());
            try {
                if (sim.azar() < (impuntual ? 0.25 : 0.04)) m.cancelar(r->getOrganizadorId(), r->getId());
                else if (r->getParticipantes().size() > 1 && sim.azar() < 0.12)
                    m.retirarse(r->getParticipantes().back(), r->getId());
            } catch (const MatchPointError &) {}
        }

        // Al cerrar el día, el dueño califica la asistencia y los jugadores califican la cancha
        simulado = dia.masMinutos(23 * 60 + 50);
        for (Reserva *r : delDia) {
            if (!r->estaActiva()) continue;
            int dueno = m.buscarCancha(r->getCanchaId())->getDuenoId();
            bool impuntual = sim.esImpuntual(r->getOrganizadorId());
            double suerte = sim.azar();
            if (suerte < (impuntual ? 0.35 : 0.05)) {
                m.calificarPartido(dueno, r->getId(), Asistencia::NO_SE_PRESENTARON);
                continue;
            }
            m.calificarPartido(dueno, r->getId(), suerte < (impuntual ? 0.65 : 0.17) ? Asistencia::LLEGARON_TARDE
                                                                                     : Asistencia::TODO_BIEN);
            double base = calidad(m.buscarCancha(r->getCanchaId())->getNombre());
            for (int p : std::vector<int>(r->getParticipantes()))
                if (sim.azar() < 0.5) {
                    int estrellas = static_cast<int>(base + sim.azar() * 2.0 - 1.0 + 0.5);
                    m.calificarCancha(p, r->getId(), std::max(1, std::min(5, estrellas)));
                }
        }
    }

    // ---------- Torneo ya terminado (para ver una tabla final) ----------
    simulado = hoy.inicioDelDia().masDias(-5).masMinutos(10 * 60);
    Equipo *pumas = m.crearEquipo(js[0]->getId(), "Los Pumas",
                                  {"Diego Schreiber", "Jerry Huaynacho", "José Quispe", "Fernando Solsol"}, {"Kevin Apaza"});
    Equipo *halcones = m.crearEquipo(js[6]->getId(), "Halcones FC",
                                     {"Carlos Mamani", "Miguel Torres", "Lucía Vargas", "Pedro Chávez"}, {});
    Equipo *condores = m.crearEquipo(js[12]->getId(), "Los Cóndores",
                                     {"Renzo Pinto", "Brayan Huamaní", "Jhon Mamani", "Daniel Quispe"}, {"Mateo Delgado"});
    Equipo *chachani = m.crearEquipo(js[17]->getId(), "Real Chachani",
                                     {"Gabriel Salas", "Rodrigo Flores", "Valeria Ccori", "Alonso Zegarra"}, {"Piero Begazo"});
    Torneo *relampago = m.crearTorneo(luis->getId(), misti->getId(), "Copa Misti Relámpago", Dinero(30.0), 4);
    for (Equipo *e : {pumas, halcones, condores, chachani})
        m.inscribirEquipo(e->getCapitanId(), relampago->getId(), e->getId());
    m.iniciarTorneo(luis->getId(), relampago->getId());
    const int goles[][2] = {{2, 1}, {3, 3}, {1, 0}, {4, 2}, {0, 2}, {2, 2}};
    for (size_t i = 0; i < relampago->getPartidos().size(); ++i)
        m.registrarResultado(luis->getId(), relampago->getId(), static_cast<int>(i), goles[i % 6][0], goles[i % 6][1]);

    // ---------- Hace unas horas: Elkin abrió un partido que todavía no se completa ----------
    // (al abrir la app recibe el aviso de que se acerca el límite y puede pulsar "Jugar así")
    simulado = hoy.masMinutos(-6 * 60);
    for (int horas : {26, 25})   // entre 24 y 27 h desde ahora: ya toca el aviso
        try {
            FechaHora pronto = hoy.inicioDelDia().masMinutos((hoy.getHora() + horas) * 60LL);
            Reserva *apurado = m.reservar(js[0]->getId(), canchas[1]->getId(), pronto, {"pelota"}, 8, 4);
            m.solicitarUnirse(js[10]->getId(), apurado->getId());
            m.responderSolicitud(js[0]->getId(), apurado->getId(), js[10]->getId(), true);
            break;
        } catch (const MatchPointError &) {}   // si a esa hora la cancha está cerrada, se prueba otra

    // ---------- Hoy: partidos que ya empezaron y el dueño todavía no califica ----------
    simulado = hoy.inicioDelDia().masMinutos(6 * 60);
    for (int h = 8; h + 1 <= hoy.getHora() && h < 12; ++h)
        sim.reservar(sim.jugadorAlAzar(1), canchas[h % 2], hoy.inicioDelDia().masMinutos(h * 60LL), 1, false, 0);

    // ---------- Vuelta al presente ----------
    m.cambiarReloj(relojReal);

    // Próximos 7 días: los más cercanos están más llenos. Solo se busca gente con más de 24 h.
    const double cercania[] = {1.0, 0.95, 0.85, 0.65, 0.5, 0.4, 0.3};
    for (int d = 0; d < 7; ++d) {
        FechaHora dia = hoy.inicioDelDia().masDias(d);
        for (Cancha *c : canchas)
            for (int h = c->getHoraApertura(); h < c->getHoraCierre(); ++h) {
                FechaHora inicio = dia.masMinutos(h * 60LL);
                if (inicio <= hoy || sim.azar() >= demanda(h) * cercania[d]) continue;
                bool abiertoAlAzar = inicio - hoy > 25 * 60 && sim.azar() < 0.55;
                // Elkin (índice 0) no entra al azar para que su cuenta quede ordenada en la demo
                sim.reservar(sim.jugadorAlAzar(1), c, inicio, 1, abiertoAlAzar, abiertoAlAzar ? sim.entre(0, 2) : 0);
            }
    }

    // Primer horario libre de la cancha (desde el día y la hora dados) en que todos los involucrados
    // están libres: así los escenarios de la demo siempre se pueden aceptar en vivo
    auto horarioPara = [&m](Cancha *c, int desdeDia, int desdeHora, std::vector<int> gente) {
        auto semana = m.disponibilidad(c->getId());
        for (size_t d = desdeDia; d < semana.size(); ++d)
            for (const Horario &h : semana[d]) {
                if (h.estado != EstadoHorario::LIBRE || h.inicio.getHora() < desdeHora) continue;
                bool libres = std::all_of(gente.begin(), gente.end(), [&](int id) {
                    auto suyas = m.reservasDe(id);
                    return std::none_of(suyas.begin(), suyas.end(),
                                        [&](Reserva *r) { return r->estaActiva() && r->getInicio() == h.inicio; });
                });
                if (libres) return h.inicio;
            }
        throw ReservaError("No hay horario para los datos de ejemplo");
    };
    auto jugadoresDe = [](Equipo *e) { std::vector<int> ids; e->listarJugadores(ids); return ids; };
    auto unir = [](std::vector<int> a, const std::vector<int> &b) { a.insert(a.end(), b.begin(), b.end()); return a; };

    // Diego: "somos 6, buscamos 4"; aceptó a Jerry y José; Fernando espera respuesta
    Reserva *abierto = m.reservar(js[1]->getId(), golazo->getId(),
                                  horarioPara(golazo, 2, 19, {js[1]->getId(), js[2]->getId(), js[3]->getId(), js[4]->getId()}),
                                  {"pelota", "chalecos"}, 6, 4);
    for (int i : {2, 3}) {
        m.solicitarUnirse(js[i]->getId(), abierto->getId());
        m.responderSolicitud(js[1]->getId(), abierto->getId(), js[i]->getId(), true);
    }
    m.solicitarUnirse(js[4]->getId(), abierto->getId());
    // Elkin: "somos 7, buscamos 3" con dos solicitudes por responder (Lucía y Pedro)
    Reserva *deElkin = m.reservar(js[0]->getId(), golazo->getId(),
                                  horarioPara(golazo, 2, 18, {js[0]->getId(), js[8]->getId(), js[9]->getId()}), {"pelota"}, 7, 3);
    m.solicitarUnirse(js[8]->getId(), deElkin->getId());
    m.solicitarUnirse(js[9]->getId(), deElkin->getId());
    m.reservar(js[5]->getId(), misti->getId(), horarioPara(misti, 2, 18, {js[5]->getId()}), {}, 10);   // privada

    // Equipo contra equipo: Los Pumas (Elkin) buscan rival y Halcones FC ya los retó;
    // Los Cóndores también buscan rival (Elkin puede retarlos con Los Pumas)
    Reserva *pumasVs = m.reservar(js[0]->getId(), misti->getId(),
                                  horarioPara(misti, 3, 20, unir(jugadoresDe(pumas), jugadoresDe(halcones))),
                                  {"pelota", "arbitro"}, 1, 1, pumas->getId());
    m.solicitarUnirse(js[6]->getId(), pumasVs->getId(), halcones->getId());
    m.reservar(js[12]->getId(), canchas[1]->getId(),
               horarioPara(canchas[1], 2, 19, unir(jugadoresDe(condores), jugadoresDe(pumas))), {"pelota"}, 1, 1,
               condores->getId());

    // Elkin ya pidió unirse a un partido de otro jugador
    for (Reserva *r : m.partidosAbiertos(js[0]->getId()))
        if (!r->esContraRival() && r->getOrganizadorId() != js[0]->getId() && r->getInicio() - hoy > 30 * 60) {
            try {
                m.solicitarUnirse(js[0]->getId(), r->getId());
                break;
            } catch (const MatchPointError &) {}
        }

    // Torneo con inscripciones abiertas
    Torneo *copa = m.crearTorneo(rosa->getId(), golazo->getId(), "Copa El Golazo 2026", Dinero(40.0), 4);
    m.inscribirEquipo(js[0]->getId(), copa->getId(), pumas->getId());
    m.inscribirEquipo(js[6]->getId(), copa->getId(), halcones->getId());
}
