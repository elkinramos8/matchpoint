// pruebas_modelo.cpp
// PRUEBAS UNITARIAS del Modelo (sin Qt y sin abrir ventanas).
// Usan un reloj fijo para que los resultados no dependan del día en que se ejecuten.
#include "../src/modelo/ModeloMatchPoint.h"
#include "../src/persistencia/DatosDemo.h"
#include "../src/persistencia/Persistencia.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <thread>

static int pruebas = 0;
#define PRUEBA(nombre) std::cout << "[OK] " << nombre << std::endl; pruebas++

// Reloj fijo: lunes 7 de diciembre de 2026, 10:00. Se puede adelantar.
static FechaHora relojActual(2026, 12, 7, 10, 0);
static FechaHora reloj() { return relojActual; }

template <typename Excepcion, typename Funcion>
static bool lanza(Funcion f) {
    try { f(); } catch (const Excepcion &) { return true; }
    return false;
}

struct Escenario {
    ModeloMatchPoint m{reloj};
    Usuario *dueno, *ana, *beto, *caro;
    Cancha *cancha;
    // Lo que lleva gastado un jugador (pagos - devoluciones recibidas)
    static Dinero gastado(const Usuario *u) { return Dinero() - u->getBilletera().getSaldo(); }
    void aceptar(Reserva *r, Usuario *j) {
        m.solicitarUnirse(j->getId(), r->getId());
        m.responderSolicitud(r->getOrganizadorId(), r->getId(), j->getId(), true);
    }
    Escenario() {
        relojActual = FechaHora(2026, 12, 7, 10, 0);
        dueno = m.registrarUsuario("Dueño de cancha", "Rosa", "rosa@x.pe", "1234", "El Golazo");
        ana = m.registrarUsuario("Jugador", "Ana", "ana@x.pe", "1234", "volante", "1111");
        beto = m.registrarUsuario("Jugador", "Beto", "beto@x.pe", "1234", "arquero", "2222");
        caro = m.registrarUsuario("Jugador", "Caro", "caro@x.pe", "1234", "defensa", "3333");
        cancha = m.registrarCancha(dueno->getId(), "El Golazo", "Cayma", "Av. 1", Dinero(60.0), 8, 23);
    }
};

void pruebasDinero() {
    Dinero a(10.5), b(4.5);
    assert(a + b == Dinero(15.0));
    assert(a - b == Dinero(6.0));
    assert(Dinero(60.0) * 1.2 == Dinero(72.0));
    assert(Dinero(60.0) / 4 == Dinero(15.0));
    assert(Dinero(10.0) / 3 == Dinero(3.34));   // se redondea hacia arriba
    assert(b < a);
    std::ostringstream os;
    os << Dinero(12.5);
    assert(os.str() == "S/ 12.50");
    assert(lanza<ValidacionError>([] { Dinero(10.0) / 0; }));
    PRUEBA("Dinero: operadores + - * / < << y división entre 0");
}

void pruebasUsuarios() {
    Escenario e;
    assert(e.ana->getRol() == "Jugador" && e.ana->puedeReservar());
    assert(e.dueno->getRol() == "Dueño de cancha" && !e.dueno->puedeReservar());
    assert(e.m.iniciarSesion("ana@x.pe", "1234") == e.ana);
    assert(lanza<AutenticacionError>([&] { e.m.iniciarSesion("ana@x.pe", "malaclave"); }));
    assert(lanza<AutenticacionError>([&] { e.m.registrarUsuario("Jugador", "Otra", "ana@x.pe", "1234", "x", "1234"); }));
    assert(lanza<ValidacionError>([&] { e.m.registrarUsuario("Jugador", "Dani", "dani@x.pe", "1234", "x", "12a"); }));
    assert(e.ana->medioDePago() == "Tarjeta •••• 1111");
    assert(e.dueno->medioDePago() == "Abono en la cuenta de El Golazo");
    PRUEBA("Usuarios: Factory Method, polimorfismo (medioDePago), login y tarjeta");
}

void pruebasReservaYPagos() {
    Escenario e;
    FechaHora noche(2026, 12, 9, 20, 0);   // hora punta: 60 * 1.2 = 72
    Reserva *r = e.m.reservar(e.ana->getId(), e.cancha->getId(), noche, {"pelota"}, 1, 3);   // Ana sola, busca 3: el total se divide entre 4
    assert(r->total() == Dinero(77.0));     // 72 + pelota 5 (Decorator)
    assert(r->descripcion() == "Cancha 1 h + pelota");
    assert(Escenario::gastado(e.ana) == Dinero(77.0));
    assert(e.dueno->getBilletera().getSaldo() == Dinero(77.0));
    const Movimiento &pago = e.ana->getBilletera().getHistorial().back();
    assert(pago.comprobante == "MP-000001" && pago.comprobante == e.dueno->getBilletera().getHistorial().back().comprobante);
    assert(r->getEstado() == EstadoReserva::ABIERTA);
    PRUEBA("Reserva: tarifa de hora punta (lambda) + extras (Decorator) + pago con comprobante");

    assert(lanza<ReservaError>([&] { e.m.reservar(e.beto->getId(), e.cancha->getId(), noche, {}, 1); }));
    assert(lanza<ReservaError>([&] { e.m.reservar(e.dueno->getId(), e.cancha->getId(), FechaHora(2026, 12, 9, 15, 0), {}, 1); }));
    assert(lanza<ReservaError>([&] { e.m.reservar(e.ana->getId(), e.cancha->getId(), FechaHora(2026, 12, 20, 15, 0), {}, 1); }));
    PRUEBA("Reserva: horario ocupado, dueño no reserva, máximo 7 días");

    e.m.solicitarUnirse(e.beto->getId(), r->getId());
    assert(!r->tieneParticipante(e.beto->getId()) && Escenario::gastado(e.beto) == Dinero());   // aún no paga
    assert(lanza<ReservaError>([&] { e.m.solicitarUnirse(e.beto->getId(), r->getId()); }));     // no repetir
    assert(lanza<ReservaError>([&] { e.m.responderSolicitud(e.caro->getId(), r->getId(), e.beto->getId(), true); }));
    assert(e.m.solicitudesPendientes(e.ana->getId()).size() == 1);
    e.m.responderSolicitud(e.ana->getId(), r->getId(), e.beto->getId(), true);   // paga 77/4 = 19.25 a Ana
    assert(Escenario::gastado(e.beto) == Dinero(19.25));
    assert(Escenario::gastado(e.ana) == Dinero(57.75));
    assert(r->lugaresLibres() == 2 && e.m.solicitudesPendientes(e.ana->getId()).empty());
    PRUEBA("Partido abierto: solicitud, el organizador acepta y se divide el pago");

    e.m.solicitarUnirse(e.caro->getId(), r->getId());
    e.m.responderSolicitud(e.ana->getId(), r->getId(), e.caro->getId(), false);
    assert(!r->tieneParticipante(e.caro->getId()) && Escenario::gastado(e.caro) == Dinero());
    PRUEBA("Partido abierto: solicitud rechazada (no se cobra nada)");

    e.m.retirarse(e.beto->getId(), r->getId()); // faltan más de 24 h: se devuelve todo
    assert(Escenario::gastado(e.beto) == Dinero());
    Usuario *dani = e.m.registrarUsuario("Jugador", "Dani", "dani@x.pe", "1234", "delantero", "0000");  // el banco la rechaza
    e.m.solicitarUnirse(dani->getId(), r->getId());
    assert(lanza<PagoError>([&] { e.m.responderSolicitud(e.ana->getId(), r->getId(), dani->getId(), true); }));
    assert(!r->tieneParticipante(dani->getId()) && r->tieneSolicitud(dani->getId()));   // no quedó a medias
    PRUEBA("Retiro a tiempo con devolución y pago rechazado por el banco (PagoError)");
}

void pruebasCancelacion() {
    Escenario e;
    FechaHora partido(2026, 12, 8, 18, 0);
    Reserva *r = e.m.reservar(e.ana->getId(), e.cancha->getId(), partido, {}, 1, 1);   // faltan 32 h: puede buscar
    e.aceptar(r, e.beto);
    relojActual = FechaHora(2026, 12, 8, 10, 0);                                       // faltan 8 h: tardía
    e.m.cancelar(e.ana->getId(), r->getId());
    assert(r->getEstado() == EstadoReserva::CANCELADA);
    assert(Escenario::gastado(e.beto) == Dinero());              // a Beto se le devuelve todo
    assert(Escenario::gastado(e.ana) == Dinero(30.0));           // Ana pierde el 50 % (multa de 30)
    assert(e.dueno->getBilletera().getSaldo() == Dinero(30.0));  // el dueño se queda con la multa
    assert(e.ana->getCalificaciones().back() == 1);              // el sistema le pone 1 ★
    PRUEBA("Cancelación tardía: multa del 50 %, devolución a los demás y 1 ★ al organizador");
}

void pruebasAsistencia() {
    Escenario e;
    Reserva *r = e.m.reservar(e.ana->getId(), e.cancha->getId(), FechaHora(2026, 12, 7, 15, 0), {}, 1);
    relojActual = FechaHora(2026, 12, 7, 15, 10);
    assert(lanza<ReservaError>([&] { e.m.calificarPartido(e.dueno->getId(), r->getId(), Asistencia::NO_SE_PRESENTARON); }));
    relojActual = FechaHora(2026, 12, 7, 15, 25);
    e.m.calificarPartido(e.dueno->getId(), r->getId(), Asistencia::NO_SE_PRESENTARON);
    assert(r->getEstado() == EstadoReserva::NO_SE_PRESENTO && e.ana->getReputacion() == 0.0);
    PRUEBA("Tolerancia de 25 minutos antes de marcar 'No se presentaron' (0 ★)");

    Reserva *r2 = e.m.reservar(e.beto->getId(), e.cancha->getId(), FechaHora(2026, 12, 7, 17, 0), {}, 1);
    relojActual = FechaHora(2026, 12, 7, 17, 5);
    e.m.calificarPartido(e.dueno->getId(), r2->getId(), Asistencia::LLEGARON_TARDE);
    assert(e.beto->getReputacion() == 3.0 && r2->getEstado() == EstadoReserva::JUGADA);
    e.m.calificarCancha(e.beto->getId(), r2->getId(), 4);
    assert(e.cancha->promedioCalificacion() == 4.0);
    assert(lanza<ReservaError>([&] { e.m.calificarCancha(e.beto->getId(), r2->getId(), 5); }));
    PRUEBA("Reputación bidireccional: el dueño califica (tarde = 3 ★) y el jugador califica la cancha");
}

void pruebasTorneo() {
    Escenario e;
    for (int i = 1; i <= 8; ++i)
        e.m.registrarUsuario("Jugador", "J" + std::to_string(i), "j" + std::to_string(i) + "@x.pe", "1234", "volante",
                             "4" + std::to_string(100 + i));
    Equipo *a = e.m.crearEquipo(e.ana->getId(), "Rojos", {"J1", "J2", "J3"}, {"J4"});
    Equipo *b = e.m.crearEquipo(e.beto->getId(), "Azules", {"J5", "J6", "J7"}, {"J8"});
    Equipo *c = e.m.crearEquipo(e.caro->getId(), "Verdes", {"J1"}, {});
    assert(a->cantidadJugadores() == 5 && a->getSuplentes().cantidadJugadores() == 1);   // Composite
    PRUEBA("Equipos con Composite: titulares + suplentes");

    Torneo *t = e.m.crearTorneo(e.dueno->getId(), e.cancha->getId(), "Copa", Dinero(20.0), 4);
    e.m.inscribirEquipo(e.ana->getId(), t->getId(), a->getId());
    e.m.inscribirEquipo(e.beto->getId(), t->getId(), b->getId());
    assert(lanza<TorneoError>([&] { e.m.inscribirEquipo(e.caro->getId(), t->getId(), c->getId()); }));
    assert(Escenario::gastado(e.ana) == Dinero(20.0) && e.dueno->getBilletera().getSaldo() == Dinero(40.0));
    e.m.iniciarTorneo(e.dueno->getId(), t->getId());
    assert(t->getPartidos().size() == 1);
    e.m.registrarResultado(e.dueno->getId(), t->getId(), 0, 3, 1);
    assert(t->getEstado() == EstadoTorneo::FINALIZADO);
    assert(t->tablaPosiciones().front().equipoId == a->getId() && t->tablaPosiciones().front().puntos() == 3);
    PRUEBA("Torneo: inscripción pagada, fixture, resultado y tabla de posiciones");
}

void pruebasTipoDeReserva() {
    Escenario e;
    assert(lanza<ValidacionError>([&] { e.m.registrarUsuario("Jugador", "Dani", "dani@x.pe", "1234", "portero", "1234"); }));
    FechaHora noche(2026, 12, 9, 18, 0);
    Reserva *privada = e.m.reservar(e.caro->getId(), e.cancha->getId(), FechaHora(2026, 12, 9, 16, 0), {}, 10);
    assert(privada->getEstado() == EstadoReserva::COMPLETA && privada->parte() == Dinero(6.0));   // 60 / 10
    assert(lanza<ReservaError>([&] { e.m.solicitarUnirse(e.beto->getId(), privada->getId()); }));   // no busca a nadie

    Reserva *r = e.m.reservar(e.ana->getId(), e.cancha->getId(), noche, {}, 3, 1);   // "somos 3, buscamos 1"
    assert(r->buscaJugadores() && r->totalJugadores() == 4 && r->parte() == Dinero(15.0) && r->lugaresLibres() == 1);
    e.m.solicitarUnirse(e.caro->getId(), r->getId());
    e.aceptar(r, e.beto);                                   // con Beto se completa: 4 / 4
    assert(r->getEstado() == EstadoReserva::COMPLETA && r->jugadoresConfirmados() == 4);
    assert(r->puestoEnEspera(e.caro->getId()) == 1);        // Caro queda en lista de espera
    e.m.retirarse(e.beto->getId(), r->getId());
    assert(r->buscaJugadores() && r->lugaresLibres() == 1); // al liberarse el cupo vuelve a buscar
    assert(lanza<ValidacionError>([&] { e.m.reservar(e.ana->getId(), e.cancha->getId(), FechaHora(2026, 12, 10, 18, 0), {}, 10, 5); }));
    PRUEBA("Reserva privada y partido abierto: \"somos X, buscamos Y\"");

    // Un jugador no puede estar en dos partidos a la misma hora (ni reservando ni uniéndose)
    assert(lanza<ReservaError>([&] {
        e.m.reservar(e.ana->getId(), e.m.registrarCancha(e.dueno->getId(), "Otra", "Cayma", "Av. 2",
                                                         Dinero(40.0), 8, 23)->getId(), noche, {}, 1); }));
    Reserva *otro = e.m.reservar(e.caro->getId(), e.m.buscarCancha(2)->getId(), FechaHora(2026, 12, 9, 18, 0), {}, 1, 5);
    assert(lanza<ReservaError>([&] { e.m.solicitarUnirse(e.ana->getId(), otro->getId()); }));
    PRUEBA("Un jugador no puede tener dos partidos a la misma hora");
}

void pruebaRival() {
    Escenario e;
    for (int i = 1; i <= 8; ++i)
        e.m.registrarUsuario("Jugador", "J" + std::to_string(i), "j" + std::to_string(i) + "@x.pe", "1234", "volante",
                             "5" + std::to_string(100 + i));
    Equipo *rojos = e.m.crearEquipo(e.ana->getId(), "Rojos", {"J1", "J2", "J3", "J4"}, {});
    Equipo *azules = e.m.crearEquipo(e.beto->getId(), "Azules", {"J5", "J6", "J7", "J8"}, {});
    Equipo *chico = e.m.crearEquipo(e.caro->getId(), "Chico", {"J1"}, {});
    FechaHora noche(2026, 12, 10, 20, 0);
    assert(lanza<ReservaError>([&] { e.m.reservar(e.caro->getId(), e.cancha->getId(), noche, {}, 1, 1, chico->getId()); }));
    Reserva *r = e.m.reservar(e.ana->getId(), e.cancha->getId(), noche, {}, 1, 1, rojos->getId());   // S/ 72 (hora punta)
    assert(r->esContraRival() && r->parte() == Dinero(36.0));                                         // mitad y mitad
    assert(lanza<ReservaError>([&] { e.m.solicitarUnirse(e.caro->getId(), r->getId()); }));         // busca equipos
    e.m.solicitarUnirse(e.beto->getId(), r->getId(), azules->getId());
    assert(e.m.solicitudesPendientes(e.ana->getId()).front().equipoId == azules->getId());
    e.m.responderSolicitud(e.ana->getId(), r->getId(), e.beto->getId(), true);
    assert(r->getEquipoRivalId() == azules->getId() && r->getEstado() == EstadoReserva::COMPLETA);
    assert(Escenario::gastado(e.beto) == Dinero(36.0) && Escenario::gastado(e.ana) == Dinero(36.0));
    relojActual = FechaHora(2026, 12, 10, 20, 5);
    e.m.calificarPartido(e.dueno->getId(), r->getId(), Asistencia::TODO_BIEN);   // califica a todos en conjunto
    assert(e.m.buscarJugadorPorNombre("J7")->getReputacion() == 5.0 &&
           e.m.buscarJugadorPorNombre("J7")->getCalificaciones().size() == 1);    // también a los del equipo rival
    relojActual = FechaHora(2026, 12, 7, 10, 0);
    std::string texto = Persistencia::serializar(e.m);
    ModeloMatchPoint copia(reloj);
    Persistencia::cargar(copia, texto);
    assert(copia.buscarReserva(r->getId())->getEquipoRivalId() == azules->getId());
    PRUEBA("Equipo contra equipo: el rival aceptado paga la mitad (Composite: equipos de 5+)");
}

void pruebasConvocatoria() {
    Escenario e;
    // Solo se puede buscar gente con más de 24 h de anticipación
    assert(lanza<ReservaError>([&] { e.m.reservar(e.ana->getId(), e.cancha->getId(), FechaHora(2026, 12, 8, 9, 0), {}, 6, 4); }));

    Reserva *r = e.m.reservar(e.ana->getId(), e.cancha->getId(), FechaHora(2026, 12, 9, 20, 0), {}, 6, 4);   // S/ 72
    e.aceptar(r, e.beto);
    relojActual = FechaHora(2026, 12, 8, 18, 0);   // faltan 26 h: se avisa al organizador
    e.m.revisarConvocatorias();
    assert(r->getAvisoConvocatoria() && r->buscaJugadores());
    relojActual = FechaHora(2026, 12, 8, 20, 0);   // faltan 24 h y sigue incompleto: se cancela solo
    e.m.revisarConvocatorias();
    assert(r->getEstado() == EstadoReserva::CANCELADA);
    assert(Escenario::gastado(e.ana) == Dinero() && Escenario::gastado(e.beto) == Dinero() &&
           e.dueno->getBilletera().getSaldo() == Dinero());   // se devolvió todo, sin multa
    PRUEBA("Partido incompleto: aviso antes del límite y cancelación con devolución total a las 24 h");

    relojActual = FechaHora(2026, 12, 7, 10, 0);
    Reserva *r2 = e.m.reservar(e.ana->getId(), e.cancha->getId(), FechaHora(2026, 12, 10, 20, 0), {}, 6, 4);
    e.m.solicitarUnirse(e.caro->getId(), r2->getId());
    e.m.jugarConLosQueHay(e.ana->getId(), r2->getId());   // "Jugar así": cubre los 4 cupos (ya los pagó)
    assert(r2->getEstado() == EstadoReserva::COMPLETA && r2->getSolicitudes().empty() && r2->getGrupo() == 10);
    relojActual = FechaHora(2026, 12, 9, 21, 0);
    e.m.revisarConvocatorias();
    assert(r2->estaActiva());                           // ya no se cancela
    PRUEBA("\"Jugar así\": el organizador cubre los cupos y el partido no se cancela");
}

void pruebaListaDeEspera() {
    Escenario e;
    Reserva *r = e.m.reservar(e.ana->getId(), e.cancha->getId(), FechaHora(2026, 12, 9, 18, 0), {}, 9, 1);   // busca 1
    e.aceptar(r, e.beto);                                                      // completo
    e.m.solicitarUnirse(e.caro->getId(), r->getId());                         // queda en lista de espera
    assert(r->admiteListaDeEspera() && r->puestoEnEspera(e.caro->getId()) == 1);
    assert(lanza<ReservaError>([&] { e.m.responderSolicitud(e.ana->getId(), r->getId(), e.caro->getId(), true); }));
    e.m.retirarse(e.beto->getId(), r->getId());                               // a tiempo: se libera el cupo
    e.m.responderSolicitud(e.ana->getId(), r->getId(), e.caro->getId(), true);
    assert(r->tieneParticipante(e.caro->getId()) && r->getEstado() == EstadoReserva::COMPLETA);
    PRUEBA("Lista de espera: si alguien se retira, entra el primero de la lista");
}

void pruebasRevision() {
    // 1) Partido incompleto con la app cerrada hasta después de la hora: igual se devuelve todo
    {
        Escenario e;
        Reserva *r = e.m.reservar(e.ana->getId(), e.cancha->getId(), FechaHora(2026, 12, 9, 20, 0), {}, 6, 4);
        relojActual = FechaHora(2026, 12, 9, 21, 0);
        e.m.revisarConvocatorias();
        assert(r->getEstado() == EstadoReserva::CANCELADA && Escenario::gastado(e.ana) == Dinero());
    }
    // 4) Retirarse justo a 24 h ya es tarde (misma frontera que el plazo para completar)
    {
        Escenario e;
        Reserva *r = e.m.reservar(e.ana->getId(), e.cancha->getId(), FechaHora(2026, 12, 9, 20, 0), {}, 9, 1);
        e.aceptar(r, e.beto);
        relojActual = FechaHora(2026, 12, 8, 20, 0);   // exactamente 24 h antes
        e.m.retirarse(e.beto->getId(), r->getId());
        e.m.revisarConvocatorias();
        assert(r->estaActiva() && !r->buscaJugadores());   // no se reabre ni se cancela para todos
        assert(e.beto->getCalificaciones().back() == 2);
    }
    // 2 y 3) Equipo contra equipo: nadie en los dos equipos y sin choque de horario para los miembros
    {
        Escenario e;
        for (int i = 1; i <= 9; ++i)
            e.m.registrarUsuario("Jugador", "J" + std::to_string(i), "j" + std::to_string(i) + "@x.pe", "1234",
                                 "volante", "6" + std::to_string(100 + i));
        Equipo *a = e.m.crearEquipo(e.ana->getId(), "A", {"J1", "J2", "J3", "J4"}, {});
        Equipo *b = e.m.crearEquipo(e.beto->getId(), "B", {"J1", "J5", "J6", "J7"}, {});   // J1 en ambos
        FechaHora noche(2026, 12, 10, 20, 0);
        Reserva *r = e.m.reservar(e.ana->getId(), e.cancha->getId(), noche, {}, 1, 1, a->getId());
        assert(lanza<ReservaError>([&] { e.m.solicitarUnirse(e.beto->getId(), r->getId(), b->getId()); }));
        Usuario *j2 = e.m.buscarJugadorPorNombre("J2");   // J2 juega con el equipo A a esa hora
        Cancha *otra = e.m.registrarCancha(e.dueno->getId(), "Otra", "Cayma", "Av. 2", Dinero(40.0), 8, 23);
        assert(lanza<ReservaError>([&] { e.m.reservar(j2->getId(), otra->getId(), noche, {}, 10); }));
        assert(e.m.reservasDe(j2->getId()).size() == 1);   // y lo ve en "Mis reservas"
        assert(lanza<ReservaError>([&] { e.m.jugarConLosQueHay(e.ana->getId(), r->getId()); }));   // necesita rival
    }
    // Canchas: no repetir nombre ni dejar reservas fuera del horario al editar
    {
        Escenario e;
        e.m.registrarCancha(e.dueno->getId(), "Otra", "Cayma", "Av. 2", Dinero(40.0), 8, 23);
        assert(lanza<ValidacionError>([&] { e.m.actualizarCancha(e.dueno->getId(), e.cancha->getId(), "Otra", "Cayma",
                                                                 "Av. 1", Dinero(60.0), 8, 23); }));
        e.m.reservar(e.ana->getId(), e.cancha->getId(), FechaHora(2026, 12, 8, 22, 0), {}, 10);
        assert(lanza<ValidacionError>([&] { e.m.actualizarCancha(e.dueno->getId(), e.cancha->getId(), "El Golazo",
                                                                 "Cayma", "Av. 1", Dinero(60.0), 8, 21); }));
        assert(lanza<ValidacionError>([] { FechaHora(2026, 2, 31, 10, 0); }));
    }
    PRUEBA("Revisión: partido vencido con la app cerrada, frontera de 24 h, equipos repetidos y canchas");
}

void pruebaDisponibilidad() {
    Escenario e;   // hoy lunes 10:00; la cancha atiende de 8 a 23 (15 horas)
    Reserva *abierto = e.m.reservar(e.ana->getId(), e.cancha->getId(), FechaHora(2026, 12, 8, 19, 0), {}, 1, 5);
    Reserva *lleno = e.m.reservar(e.beto->getId(), e.cancha->getId(), FechaHora(2026, 12, 8, 20, 0), {}, 1);
    auto grilla = e.m.disponibilidad(e.cancha->getId(), e.ana->getId());
    assert(grilla.size() == 7 && grilla[0].size() == 15);
    assert(grilla[0][0].estado == EstadoHorario::PASADO && grilla[0][3].estado == EstadoHorario::LIBRE);  // 8:00 y 11:00
    assert(grilla[1][11].estado == EstadoHorario::MI_PARTIDO && grilla[1][11].reservaId == abierto->getId());
    assert(grilla[1][12].estado == EstadoHorario::OCUPADO && grilla[1][12].reservaId == lleno->getId());
    assert(grilla[1][11].horaPunta && !grilla[1][10].horaPunta && grilla[1][11].precio == Dinero(72.0));
    auto paraCaro = e.m.disponibilidad(e.cancha->getId(), e.caro->getId());
    assert(paraCaro[1][11].estado == EstadoHorario::PARTIDO_ABIERTO);
    PRUEBA("Grilla semanal: libre, ocupado, partido abierto, mi partido y hora punta");
}

void pruebaConcurrencia() {
    // Dos jugadores intentan reservar el MISMO horario al mismo tiempo desde dos hilos.
    // Gracias al mutex del Modelo, solo uno lo consigue.
    for (int intento = 0; intento < 50; ++intento) {
        Escenario e;
        FechaHora hora(2026, 12, 8, 16, 0);
        int exitos = 0, rechazos = 0;
        std::mutex mtxConteo;
        auto intentar = [&](int jugadorId) {
            try {
                e.m.reservar(jugadorId, e.cancha->getId(), hora, {}, 1);
                std::lock_guard<std::mutex> l(mtxConteo); exitos++;
            } catch (const ReservaError &) {
                std::lock_guard<std::mutex> l(mtxConteo); rechazos++;
            }
        };
        std::thread h1(intentar, e.ana->getId());
        std::thread h2(intentar, e.beto->getId());
        h1.join();
        h2.join();
        assert(exitos == 1 && rechazos == 1);
    }
    PRUEBA("Multithreading: 2 hilos reservan a la vez y solo uno lo logra (50 repeticiones)");
}

void pruebaObserverYPersistencia() {
    Escenario e;
    int avisos = 0;
    e.m.suscribir([&avisos](const Evento &) { avisos++; });
    e.m.reservar(e.ana->getId(), e.cancha->getId(), FechaHora(2026, 12, 8, 16, 0), {"arbitro"}, 1, 1);
    assert(avisos == 2);   // aviso al dueño y al jugador
    PRUEBA("Observer: el modelo notifica a sus suscriptores");

    std::string texto = Persistencia::serializar(e.m);
    ModeloMatchPoint copia(reloj);
    Persistencia::cargar(copia, texto);
    assert(Persistencia::serializar(copia) == texto);
    assert(copia.iniciarSesion("ana@x.pe", "1234")->getBilletera().getSaldo() == e.ana->getBilletera().getSaldo());
    assert(copia.listarCanchas().size() == 1 && copia.reservasDe(e.ana->getId()).front()->total() == Dinero(80.0));
    Reserva *r = copia.reservasDe(e.ana->getId()).front();
    copia.solicitarUnirse(e.beto->getId(), r->getId());
    std::string conSolicitud = Persistencia::serializar(copia);
    ModeloMatchPoint otra(reloj);
    Persistencia::cargar(otra, conSolicitud);
    assert(otra.solicitudesPendientes(e.ana->getId()).size() == 1);
    assert(otra.buscarUsuario(e.ana->getId())->getTarjeta() == "1111" && otra.getUltimoComprobante() == 1);
    PRUEBA("Persistencia: guardar y volver a cargar deja los mismos datos");
}

void pruebaDatosDemo() {
    ModeloMatchPoint m(reloj);
    relojActual = FechaHora(2026, 12, 7, 10, 0);
    cargarDatosDemo(m);   // simula 6 días de historial con las reglas del modelo
    int bajos = 0, altos = 0;
    for (Usuario *u : m.rankingJugadores()) { if (u->getReputacion() < 4.0) bajos++; if (u->getReputacion() >= 4.5) altos++; }
    assert(bajos >= 2 && altos >= 3);   // hay jugadores confiables y otros que faltan o llegan tarde
    for (Cancha *c : m.listarCanchas()) assert(!c->getCalificaciones().empty());
    int ocupadas = 0;
    auto semana = m.disponibilidad(m.listarCanchas().front()->getId());
    for (const auto &h : semana[1]) if (h.reservaId >= 0) ocupadas++;
    assert(ocupadas >= 3);              // mañana ya hay varias horas tomadas
    assert(m.reservasDe(m.iniciarSesion("elkin@matchpoint.pe", "1234")->getId()).size() >= 1);
    std::cout << "     (" << m.repoReservas().cantidad() << " reservas, " << bajos << " jugadores con reputación baja)\n";
    PRUEBA("Datos de ejemplo: historial simulado con reputaciones y calificaciones variadas");
}

int main() {
    pruebasDinero();
    pruebasUsuarios();
    pruebasReservaYPagos();
    pruebasCancelacion();
    pruebasAsistencia();
    pruebasTorneo();
    pruebasTipoDeReserva();
    pruebasConvocatoria();
    pruebaListaDeEspera();
    pruebaRival();
    pruebasRevision();
    pruebaDisponibilidad();
    pruebaConcurrencia();
    pruebaObserverYPersistencia();
    pruebaDatosDemo();
    std::cout << pruebas << " pruebas pasaron" << std::endl;
    return 0;
}
