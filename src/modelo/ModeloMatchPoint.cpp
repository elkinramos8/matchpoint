// ModeloMatchPoint.cpp
#include "ModeloMatchPoint.h"
#include "Configuracion.h"
#include <algorithm>
#include <cctype>
#include <cstdio>

// Cada operación que modifica datos sigue el mismo esquema:
//   { lock_guard (exclusión mutua) -> validar -> modificar -> encolar evento }
//   despacharEventos()  (los observadores se llaman sin el mutex tomado)

ModeloMatchPoint::ModeloMatchPoint(Reloj reloj) : reloj(std::move(reloj)) {
    // Tarifa como lambda: desde la hora punta se aplica un recargo
    tarifa = [](const Dinero &precioHora, const FechaHora &inicio) {
        const auto &cfg = Configuracion::obtenerInstancia();
        return inicio.getHora() >= cfg.horaPuntaInicio() ? precioHora * (1.0 + cfg.recargoHoraPunta()) : precioHora;
    };
}

// ===================== Auxiliares =====================

Usuario &ModeloMatchPoint::usuarioValido(int id) const {
    Usuario *u = usuarios.buscar(id);
    if (!u) throw ValidacionError("El usuario no existe");
    return *u;
}

Reserva &ModeloMatchPoint::reservaValida(int id) const {
    Reserva *r = reservas.buscar(id);
    if (!r) throw ReservaError("La reserva no existe");
    return *r;
}

Cancha &ModeloMatchPoint::canchaDelDueno(int duenoId, int canchaId) const {
    Cancha *c = canchas.buscar(canchaId);
    if (!c) throw ValidacionError("La cancha no existe");
    if (c->getDuenoId() != duenoId) throw ReservaError("Esa cancha no te pertenece");
    return *c;
}

std::string ModeloMatchPoint::nombreCancha(int canchaId) const {
    Cancha *c = canchas.buscar(canchaId);
    return c ? c->getNombre() : "?";
}

static bool sonCuatroDigitos(const std::string &t) {
    return t.size() == 4 && std::all_of(t.begin(), t.end(), [](unsigned char c) { return std::isdigit(c); });
}

bool ModeloMatchPoint::horarioOcupado(int canchaId, const FechaHora &inicio) const {
    return reservas.buscarSi([&](const Reserva &r) {
        return r.getCanchaId() == canchaId && r.estaActiva() && r.getInicio() == inicio;
    }) != nullptr;
}

void ModeloMatchPoint::validarJugadorLibre(int jugadorId, const FechaHora &inicio) const {
    if (reservas.buscarSi([&](const Reserva &r) {
            return r.estaActiva() && r.getInicio() == inicio && juegaEn(r, jugadorId); })) {
        Usuario *u = usuarios.buscar(jugadorId);
        throw ReservaError((u ? u->getNombre() : std::string("Un jugador")) + " ya tiene otro partido a esa misma hora");
    }
}

bool ModeloMatchPoint::juegaEn(const Reserva &r, int jugadorId) const {
    auto ids = jugadoresDe(r);
    return std::find(ids.begin(), ids.end(), jugadorId) != ids.end();
}

void ModeloMatchPoint::validarEquiposSinRepetidos(int equipoA, int equipoB) const {
    Equipo *a = equipos.buscar(equipoA), *b = equipos.buscar(equipoB);
    if (!a || !b) return;
    auto idsA = a->idsJugadores();
    for (int j : b->idsJugadores())
        if (std::find(idsA.begin(), idsA.end(), j) != idsA.end())
            throw ReservaError(usuarioValido(j).getNombre() + " está en los dos equipos: no pueden jugar entre sí");
}

// PASARELA DE PAGO SIMULADA: no hay banco real. El jugador paga con su tarjeta
// guardada, el dinero llega al destino y ambos ven el mismo comprobante.
// Las devoluciones no pasan por el banco (no pueden ser rechazadas).
std::string ModeloMatchPoint::pagar(Usuario &origen, Usuario &destino, const Dinero &monto,
                                      const std::string &concepto, bool esDevolucion) {
    if (monto <= Dinero()) return "";
    if (!esDevolucion && origen.puedeReservar()) {   // solo los jugadores pagan con tarjeta
        if (origen.getTarjeta().empty()) throw PagoError("No tienes una tarjeta registrada");
        if (origen.getTarjeta() == Configuracion::obtenerInstancia().tarjetaRechazada())
            throw PagoError("El banco rechazó el pago con la tarjeta •••• " + origen.getTarjeta());
    }
    char numero[16];
    std::snprintf(numero, sizeof(numero), "MP-%06d", ++ultimoComprobante);
    origen.getBilletera().registrarPago(monto, concepto, reloj(), numero);
    destino.getBilletera().registrarIngreso(monto, concepto, reloj(), numero);
    return numero;
}

void ModeloMatchPoint::encolarEvento(const std::string &tipo, const std::string &mensaje,
                                       const std::vector<int> &destinos) {
    pendientes.push_back({tipo, mensaje, destinos});
    for (int d : destinos) notificaciones.push_back({d, reloj(), mensaje, tipo});
}

void ModeloMatchPoint::despacharEventos() {
    std::vector<Evento> lista;
    std::map<int, Observador> copia;
    {
        std::lock_guard<std::mutex> candado(mtx);
        lista.swap(pendientes);
        copia = observadores;   // se copia con el mutex tomado; se avisa sin él
    }
    for (const auto &e : lista)
        for (const auto &par : copia) par.second(e);   // OBSERVER: avisa a todos
}

// ===================== Usuarios =====================

Usuario *ModeloMatchPoint::registrarUsuario(const std::string &rol, const std::string &nombre,
                                              const std::string &correo, const std::string &clave,
                                              const std::string &detalle, const std::string &tarjeta) {
    Usuario *nuevo;
    {
        std::lock_guard<std::mutex> candado(mtx);
        if (nombre.empty() || correo.empty() || detalle.empty())
            throw ValidacionError("Completa todos los campos");
        if (correo.find('@') == std::string::npos) throw ValidacionError("El correo no es válido");
        if (clave.size() < 4) throw ValidacionError("La contraseña debe tener al menos 4 caracteres");
        if (usuarios.buscarSi([&](const Usuario &u) { return u.getCorreo() == correo; }))
            throw AutenticacionError("Ya existe una cuenta con ese correo");
        if (usuarios.buscarSi([&](const Usuario &u) { return u.getNombre() == nombre; }))
            throw AutenticacionError("Ya existe un usuario con ese nombre");

        // FACTORY METHOD: el modelo no hace "new Jugador" directamente
        auto u = UsuarioFactory::crear(UsuarioFactory::tipoDesdeTexto(rol), nombre, correo,
                                       Usuario::hashClave(clave), detalle);
        if (u->puedeReservar()) {
            const std::vector<std::string> posiciones{"arquero", "defensa", "volante", "delantero"};
            if (std::find(posiciones.begin(), posiciones.end(), detalle) == posiciones.end())
                throw ValidacionError("Elige una posición: arquero, defensa, volante o delantero");
            if (!sonCuatroDigitos(tarjeta)) throw ValidacionError("Ingresa los 4 últimos dígitos de tu tarjeta");
            u->setTarjeta(tarjeta);
        }
        nuevo = usuarios.agregar(std::move(u));
        encolarEvento("usuario", "¡Bienvenido a MatchPoint, " + nombre + "!", {nuevo->getId()});
    }
    despacharEventos();
    return nuevo;
}

Usuario *ModeloMatchPoint::iniciarSesion(const std::string &correo, const std::string &clave) const {
    Usuario *u = usuarios.buscarSi([&](const Usuario &x) { return x.getCorreo() == correo; });
    if (!u || !u->verificarClave(clave)) throw AutenticacionError("Correo o contraseña incorrectos");
    return u;
}

Usuario *ModeloMatchPoint::buscarJugadorPorNombre(const std::string &nombre) const {
    return usuarios.buscarSi([&](const Usuario &u) { return u.getNombre() == nombre && u.puedeReservar(); });
}

// ===================== Canchas =====================

Cancha *ModeloMatchPoint::registrarCancha(int duenoId, const std::string &nombre, const std::string &distrito,
                                            const std::string &direccion, const Dinero &precio,
                                            int apertura, int cierre) {
    Cancha *c;
    {
        std::lock_guard<std::mutex> candado(mtx);
        if (!usuarioValido(duenoId).puedeAdministrarCanchas())
            throw ReservaError("Solo los dueños pueden registrar canchas");
        if (canchas.buscarSi([&](const Cancha &x) { return x.getNombre() == nombre; }))
            throw ValidacionError("Ya existe una cancha con ese nombre");
        c = canchas.agregar(std::make_unique<Cancha>(duenoId, nombre, distrito, direccion, precio, apertura, cierre));
        encolarEvento("cancha", "Tu cancha " + nombre + " ya aparece en MatchPoint", {duenoId});
    }
    despacharEventos();
    return c;
}

void ModeloMatchPoint::actualizarCancha(int duenoId, int canchaId, const std::string &nombre,
                                          const std::string &distrito, const std::string &direccion,
                                          const Dinero &precio, int apertura, int cierre) {
    {
        std::lock_guard<std::mutex> candado(mtx);
        Cancha &c = canchaDelDueno(duenoId, canchaId);
        if (canchas.buscarSi([&](const Cancha &x) { return x.getNombre() == nombre && x.getId() != canchaId; }))
            throw ValidacionError("Ya existe otra cancha con ese nombre");
        FechaHora ahoraMismo = reloj();
        if (reservas.buscarSi([&](const Reserva &r) {
                int h = r.getInicio().getHora();
                return r.getCanchaId() == canchaId && r.estaActiva() && ahoraMismo < r.getInicio() &&
                       (h < apertura || h >= cierre); }))
            throw ValidacionError("Hay reservas futuras fuera del nuevo horario: cancélalas o elige otro horario");
        c.actualizar(nombre, distrito, direccion, precio, apertura, cierre);
        encolarEvento("cancha", "Se actualizaron los datos de " + nombre, {duenoId});
    }
    despacharEventos();
}

std::vector<Cancha *> ModeloMatchPoint::canchasDe(int duenoId) const {
    return canchas.filtrar([duenoId](const Cancha &c) { return c.getDuenoId() == duenoId; });
}

std::vector<FechaHora> ModeloMatchPoint::diasReservables() const {
    std::vector<FechaHora> dias;
    FechaHora hoy = reloj().inicioDelDia();
    for (int i = 0; i < Configuracion::obtenerInstancia().diasReservables(); ++i) dias.push_back(hoy.masDias(i));
    return dias;
}

std::vector<std::vector<Horario>> ModeloMatchPoint::disponibilidad(int canchaId, int paraUsuarioId) const {
    std::vector<std::vector<Horario>> semana;
    Cancha *c = canchas.buscar(canchaId);
    if (!c) return semana;
    int horaPunta = Configuracion::obtenerInstancia().horaPuntaInicio();
    FechaHora ahoraMismo = reloj();
    for (const FechaHora &dia : diasReservables()) {
        std::vector<Horario> horas;
        for (int h = c->getHoraApertura(); h < c->getHoraCierre(); ++h) {
            FechaHora inicio = dia.masMinutos(h * 60LL);
            Reserva *r = reservas.buscarSi([&](const Reserva &x) {
                return x.getCanchaId() == canchaId && x.estaActiva() && x.getInicio() == inicio;
            });
            EstadoHorario estado;
            if (r && juegaEn(*r, paraUsuarioId)) estado = EstadoHorario::MI_PARTIDO;
            else if (inicio <= ahoraMismo) estado = EstadoHorario::PASADO;
            else if (!r) estado = EstadoHorario::LIBRE;
            else if (r->buscaJugadores() && r->esContraRival()) estado = EstadoHorario::BUSCA_RIVAL;
            else if (r->buscaJugadores()) estado = EstadoHorario::PARTIDO_ABIERTO;
            else estado = EstadoHorario::OCUPADO;
            horas.push_back({inicio, estado, r ? r->getId() : -1, h >= horaPunta, tarifa(c->getPrecioHora(), inicio)});
        }
        semana.push_back(horas);
    }
    return semana;
}

Dinero ModeloMatchPoint::cotizar(int canchaId, const FechaHora &inicio, const std::vector<std::string> &extras) const {
    Cancha *c = canchas.buscar(canchaId);
    if (!c) throw ValidacionError("La cancha no existe");
    return construirServicio(tarifa(c->getPrecioHora(), inicio), extras)->precio();
}

// ===================== Reservas y pagos =====================

Reserva *ModeloMatchPoint::reservar(int jugadorId, int canchaId, const FechaHora &inicio,
                                      const std::vector<std::string> &extras, int grupo, int buscados, int equipoId) {
    Reserva *r;
    {
        std::lock_guard<std::mutex> candado(mtx);
        Usuario &jugador = usuarioValido(jugadorId);
        if (!jugador.puedeReservar()) throw ReservaError("Solo los jugadores pueden reservar");
        Cancha *c = canchas.buscar(canchaId);
        if (!c) throw ValidacionError("La cancha no existe");
        FechaHora limite = reloj().inicioDelDia().masDias(Configuracion::obtenerInstancia().diasReservables());
        if (inicio <= reloj()) throw ReservaError("Ese horario ya pasó");
        if (!(inicio < limite)) throw ReservaError("Solo se puede reservar para los próximos 7 días");
        if (!c->atiendeALas(inicio.getHora())) throw ReservaError("La cancha no atiende a esa hora");
        if (horarioOcupado(canchaId, inicio)) throw ReservaError("Ese horario ya fue reservado");
        validarJugadorLibre(jugadorId, inicio);
        if ((buscados > 0 || equipoId >= 0) && inicio - reloj() <= minutosLimite())
            throw ReservaError("Para buscar jugadores o rival reserva con más de 24 h de anticipación "
                               "(es el plazo para completar el partido)");
        if (equipoId >= 0) {   // buscar rival: se juega equipo contra equipo y se paga mitad y mitad
            validarEquipoDelCapitan(jugadorId, equipoId);
            for (int j : equipos.buscar(equipoId)->idsJugadores()) validarJugadorLibre(j, inicio);
            grupo = 1;
            buscados = 1;
        }

        auto nueva = std::make_unique<Reserva>(canchaId, jugadorId, inicio, grupo, buscados,
                                               tarifa(c->getPrecioHora(), inicio), extras);
        nueva->setEquipo(equipoId);
        // El organizador paga el total al dueño; los que acepte le pagan su parte a él
        Usuario &dueno = usuarioValido(c->getDuenoId());
        pagar(jugador, dueno, nueva->total(), "Reserva " + c->getNombre() + " " + inicio.texto());
        r = reservas.agregar(std::move(nueva));

        encolarEvento("reserva", "Nueva reserva en " + c->getNombre() + " para el " + inicio.texto() +
                      " (" + r->total().texto() + ")", {c->getDuenoId()});
        std::string msj = "Reservaste " + c->getNombre() + " el " + inicio.texto();
        if (equipoId >= 0) msj += ". Buscas un equipo rival para " + equipos.buscar(equipoId)->getNombre();
        else if (buscados > 0) msj += ". Partido abierto: buscas " + std::to_string(buscados) + " jugadores";
        encolarEvento("reserva", msj, {jugadorId});
    }
    despacharEventos();
    return r;
}

static std::string reputacionTexto(const Usuario &u) {
    if (u.esNuevo()) return "jugador nuevo";
    char texto[32];
    std::snprintf(texto, sizeof(texto), "%.1f ★", u.getReputacion());
    return texto;
}

long long ModeloMatchPoint::minutosLimite() const {
    return Configuracion::obtenerInstancia().horasCancelacionGratis() * 60LL;
}

std::vector<int> ModeloMatchPoint::jugadoresDe(const Reserva &r) const {
    std::vector<int> ids = r.getParticipantes();
    if (r.esContraRival())   // COMPOSITE: se recorre cada equipo completo
        for (int eq : {r.getEquipoId(), r.getEquipoRivalId()})
            if (Equipo *e = equipos.buscar(eq))
                for (int j : e->idsJugadores())
                    if (std::find(ids.begin(), ids.end(), j) == ids.end()) ids.push_back(j);
    return ids;
}

void ModeloMatchPoint::validarEquipoDelCapitan(int capitanId, int equipoId) const {
    Equipo *e = equipos.buscar(equipoId);
    if (!e) throw ReservaError("El equipo no existe");
    if (e->getCapitanId() != capitanId) throw ReservaError("Solo el capitán puede jugar con el equipo");
    if (e->cantidadJugadores() < 5) throw ReservaError("El equipo necesita al menos 5 jugadores");
}

void ModeloMatchPoint::solicitarUnirse(int jugadorId, int reservaId, int equipoId) {
    {
        std::lock_guard<std::mutex> candado(mtx);
        Usuario &jugador = usuarioValido(jugadorId);
        Reserva &r = reservaValida(reservaId);
        if (!jugador.puedeReservar()) throw ReservaError("Solo los jugadores pueden unirse a partidos");
        if (!r.buscaJugadores() && !r.admiteListaDeEspera()) throw ReservaError("Ese partido no recibe jugadores");
        if (r.getInicio() - reloj() <= minutosLimite())
            throw ReservaError("La convocatoria de ese partido ya cerró (falta menos de 24 h)");
        if (juegaEn(r, jugadorId)) throw ReservaError("Ya estás en ese partido");
        if (r.tieneSolicitud(jugadorId)) throw ReservaError("Ya enviaste una solicitud a ese partido");
        if (r.getInicio() <= reloj()) throw ReservaError("El partido ya empezó");
        validarJugadorLibre(jugadorId, r.getInicio());
        if (r.esContraRival()) {
            if (equipoId < 0) throw ReservaError("Ese partido busca un equipo rival: pide jugar como capitán de tu equipo");
            validarEquipoDelCapitan(jugadorId, equipoId);
            if (equipoId == r.getEquipoId()) throw ReservaError("No puedes jugar contra tu propio equipo");
            validarEquiposSinRepetidos(r.getEquipoId(), equipoId);
            for (int j : equipos.buscar(equipoId)->idsJugadores()) validarJugadorLibre(j, r.getInicio());
            encolarEvento("solicitud", "El equipo " + equipos.buscar(equipoId)->getNombre() + " (capitán " +
                          jugador.getNombre() + ") quiere jugar contra " + equipos.buscar(r.getEquipoId())->getNombre() +
                          " el " + r.getInicio().texto(), {r.getOrganizadorId()});
        } else {
            if (equipoId >= 0) throw ReservaError("Ese partido busca jugadores, no equipos");
            encolarEvento("solicitud", jugador.getNombre() + " (" + jugador.getDetalle() + ", reputación " +
                          reputacionTexto(jugador) + ") quiere unirse a tu partido del " +
                          r.getInicio().texto(), {r.getOrganizadorId()});
        }
        r.agregarSolicitud(jugadorId, equipoId);
        if (r.admiteListaDeEspera())
            encolarEvento("solicitud", "El partido del " + r.getInicio().texto() + " está completo: quedaste en la lista "
                          "de espera (puesto " + std::to_string(r.puestoEnEspera(jugadorId)) + ")", {jugadorId});
        encolarEvento("solicitud", "Enviaste tu solicitud para el partido del " + r.getInicio().texto() +
                      " en " + nombreCancha(r.getCanchaId()) + ". Pagas solo si te aceptan", {jugadorId});
    }
    despacharEventos();
}

void ModeloMatchPoint::responderSolicitud(int organizadorId, int reservaId, int jugadorId, bool aceptar) {
    {
        std::lock_guard<std::mutex> candado(mtx);
        Reserva &r = reservaValida(reservaId);
        if (r.getOrganizadorId() != organizadorId) throw ReservaError("Solo el organizador responde las solicitudes");
        if (!r.tieneSolicitud(jugadorId)) throw ReservaError("Esa solicitud ya no existe");
        Usuario &jugador = usuarioValido(jugadorId);
        std::string partido = nombreCancha(r.getCanchaId()) + " del " + r.getInicio().texto();

        if (!aceptar) {
            r.quitarSolicitud(jugadorId);
            encolarEvento("solicitud", "El organizador no aceptó tu solicitud para " + partido, {jugadorId});
        } else {
            if (!r.buscaJugadores()) throw ReservaError("El partido está completo: esa solicitud sigue en lista de espera");
            if (r.getInicio() <= reloj()) throw ReservaError("El partido ya empezó");
            if (r.getInicio() - reloj() <= minutosLimite()) throw ReservaError("Ya venció el plazo para completar ese partido");
            validarJugadorLibre(jugadorId, r.getInicio());   // pudo reservar otro partido mientras esperaba
            int equipoQuePide = r.equipoQueSolicita(jugadorId);
            if (r.esContraRival() && equipoQuePide >= 0) {   // los equipos pudieron cambiar mientras esperaba
                validarEquiposSinRepetidos(r.getEquipoId(), equipoQuePide);
                for (int j : equipos.buscar(equipoQuePide)->idsJugadores()) validarJugadorLibre(j, r.getInicio());
            }
            // Dividir el pago: el aceptado le paga su parte al organizador
            Usuario &organizador = usuarioValido(organizadorId);
            std::string comprobante = pagar(jugador, organizador, r.parte(), "Parte del partido " + partido);
            int equipoRival = r.equipoQueSolicita(jugadorId);
            r.quitarSolicitud(jugadorId);
            r.agregarParticipante(jugadorId);
            if (r.esContraRival()) r.setEquipoRival(equipoRival);
            encolarEvento("solicitud", "¡Te aceptaron en el partido " + partido + "! Pagaste " + r.parte().texto() +
                          " (comprobante " + comprobante + ")", {jugadorId});
            encolarEvento("reserva", jugador.getNombre() + " ya está en tu partido (" +
                          std::to_string(r.lugaresLibres()) + " cupos libres)", {organizadorId});
            if (r.getEstado() == EstadoReserva::COMPLETA) {
                encolarEvento("reserva", "¡Partido completo! " + partido, r.getParticipantes());
                // LISTA DE ESPERA: las solicitudes que quedaban esperan por si alguien se retira
                for (int espera : r.getSolicitudes())
                    encolarEvento("solicitud", "El partido " + partido + " se completó: sigues en lista de espera (puesto " +
                                  std::to_string(r.puestoEnEspera(espera)) + ")", {espera});
            }
        }
    }
    despacharEventos();
}

std::vector<Reserva *> ModeloMatchPoint::solicitudesEnviadas(int jugadorId) const {
    auto lista = reservas.filtrar([jugadorId](const Reserva &r) { return r.estaActiva() && r.tieneSolicitud(jugadorId); });
    std::sort(lista.begin(), lista.end(), [](Reserva *a, Reserva *b) { return a->getInicio() < b->getInicio(); });
    return lista;
}

std::vector<Solicitud> ModeloMatchPoint::solicitudesPendientes(int organizadorId) const {
    std::vector<Solicitud> lista;
    for (Reserva *r : reservasDe(organizadorId))
        if (r->getOrganizadorId() == organizadorId && r->estaActiva())
            for (int j : r->getSolicitudes()) lista.push_back({r->getId(), j, r->equipoQueSolicita(j)});
    return lista;
}

void ModeloMatchPoint::retirarse(int jugadorId, int reservaId) {
    {
        std::lock_guard<std::mutex> candado(mtx);
        Usuario &jugador = usuarioValido(jugadorId);
        Reserva &r = reservaValida(reservaId);
        if (!r.tieneParticipante(jugadorId)) throw ReservaError("No estás en ese partido");
        if (r.getOrganizadorId() == jugadorId)
            throw ReservaError("El organizador no se retira: debe cancelar la reserva");
        if (!r.estaActiva()) throw ReservaError("La reserva ya no está activa");
        if (r.getInicio() <= reloj()) throw ReservaError("El partido ya empezó");

        long long minutos = r.getInicio() - reloj();
        bool aTiempo = minutos > minutosLimite();   // más de 24 h: igual que el plazo para completar
        int equipoQueSeVa = r.esContraRival() ? r.getEquipoRivalId() : -1;
        if (aTiempo) {
            Usuario &organizador = usuarioValido(r.getOrganizadorId());
            pagar(organizador, jugador, r.parte(), "Devolución por retiro del partido", true);
        } else {
            // Si se retira el capitán rival, se va todo su equipo y todos reciben la calificación
            std::vector<int> afectados{jugadorId};
            if (Equipo *e = equipos.buscar(equipoQueSeVa)) afectados = e->idsJugadores();
            for (int j : afectados) usuarioValido(j).recibirCalificacion(Configuracion::obtenerInstancia().estrellasRetiroTardio());
        }
        r.quitarParticipante(jugadorId);   // si se retira a tiempo, el partido vuelve a buscar
        if (aTiempo) r.setAvisoConvocatoria(false);   // vuelve a buscar: corresponde un nuevo aviso
        if (!aTiempo) r.cubrirCuposVacios();   // tarde: ya no hay tiempo de buscar; el cupo lo cubre su pago
        if (r.buscaJugadores() && !r.getSolicitudes().empty()) {
            int primero = r.getSolicitudes().front();
            encolarEvento("solicitud", "Se liberó un cupo en el partido del " + r.getInicio().texto() +
                          ": eres el primero de la lista de espera", {primero});
            encolarEvento("solicitud", "Se liberó un cupo y tienes " + std::to_string(r.getSolicitudes().size()) +
                          " en lista de espera para el " + r.getInicio().texto(), {r.getOrganizadorId()});
        }

        encolarEvento("reserva", jugador.getNombre() + " se retiró de tu partido del " + r.getInicio().texto(),
                      {r.getOrganizadorId()});
        encolarEvento("reserva", aTiempo ? "Te retiraste a tiempo: se te devolvió " + r.parte().texto()
                                         : "Te retiraste tarde: no hay devolución y bajó tu reputación",
                      {jugadorId});
    }
    despacharEventos();
}

void ModeloMatchPoint::cancelar(int jugadorId, int reservaId) {
    {
        std::lock_guard<std::mutex> candado(mtx);
        Reserva &r = reservaValida(reservaId);
        if (r.getOrganizadorId() != jugadorId) throw ReservaError("Solo el organizador puede cancelar la reserva");
        if (!r.estaActiva()) throw ReservaError("La reserva ya no está activa");
        if (r.getInicio() <= reloj()) throw ReservaError("El partido ya empezó");

        const auto &cfg = Configuracion::obtenerInstancia();
        Usuario &organizador = usuarioValido(jugadorId);
        Usuario &dueno = usuarioValido(buscarCancha(r.getCanchaId())->getDuenoId());
        bool aTiempo = (r.getInicio() - reloj()) > minutosLimite();
        Dinero devolucion = aTiempo ? r.total() : r.total() * (1.0 - cfg.porcentajeMulta());

        // El dueño devuelve (todo o la mitad) y el organizador devuelve su parte a cada jugador
        pagar(dueno, organizador, devolucion,
              aTiempo ? "Devolución por cancelación" : "Devolución por cancelación tardía (multa del 50 %)", true);
        for (int p : r.getParticipantes())
            if (p != jugadorId)
                pagar(organizador, usuarioValido(p), r.parte(), "Devolución: partido cancelado", true);
        if (!aTiempo) organizador.recibirCalificacion(Configuracion::obtenerInstancia().estrellasCancelacionTardia());
        r.setEstado(EstadoReserva::CANCELADA);
        std::vector<int> pendientesDeRespuesta = r.retirarSolicitudes();
        if (!pendientesDeRespuesta.empty())
            encolarEvento("solicitud", "El partido del " + r.getInicio().texto() + " al que pediste unirte fue cancelado",
                          pendientesDeRespuesta);

        std::vector<int> otros;
        for (int p : r.getParticipantes()) if (p != jugadorId) otros.push_back(p);
        encolarEvento("reserva", "Se canceló la reserva de " + nombreCancha(r.getCanchaId()) + " del " +
                      r.getInicio().texto() + (aTiempo ? "" : " (con multa)"), {dueno.getId()});
        if (!otros.empty())
            encolarEvento("reserva", "El partido del " + r.getInicio().texto() +
                          " fue cancelado; se te devolvió tu parte", otros);
        encolarEvento("reserva", aTiempo ? "Cancelaste a tiempo: se te devolvió " + devolucion.texto()
                                         : "Cancelaste tarde: multa de " + (r.total() - devolucion).texto(),
                      {jugadorId});
    }
    despacharEventos();
}

void ModeloMatchPoint::calificarPartido(int duenoId, int reservaId, Asistencia asistencia) {
    {
        std::lock_guard<std::mutex> candado(mtx);
        Reserva &r = reservaValida(reservaId);
        canchaDelDueno(duenoId, r.getCanchaId());
        if (!r.estaActiva()) throw ReservaError("Ese partido ya fue calificado o cancelado");
        if (reloj() < r.getInicio()) throw ReservaError("El partido todavía no empieza");
        if (r.buscaJugadores()) throw ReservaError("Ese partido no se completó: se cancela solo con devolución total");
        const auto &cfg = Configuracion::obtenerInstancia();
        if (asistencia == Asistencia::NO_SE_PRESENTARON && reloj() - r.getInicio() < cfg.toleranciaMinutos())
            throw ReservaError("Aún no pasan los " + std::to_string(cfg.toleranciaMinutos()) + " minutos de tolerancia");

        int estrellas = asistencia == Asistencia::TODO_BIEN ? cfg.estrellasTodoBien()
                      : asistencia == Asistencia::LLEGARON_TARDE ? cfg.estrellasLlegoTarde()
                      : cfg.estrellasNoSePresento();
        std::vector<int> jugadores = jugadoresDe(r);   // a todos en conjunto (y a los equipos completos)
        for (int j : jugadores) usuarioValido(j).recibirCalificacion(estrellas);
        r.setEstado(asistencia == Asistencia::NO_SE_PRESENTARON ? EstadoReserva::NO_SE_PRESENTO : EstadoReserva::JUGADA);
        r.retirarSolicitudes();   // la lista de espera ya no sirve

        std::string cancha = nombreCancha(r.getCanchaId());
        if (asistencia == Asistencia::NO_SE_PRESENTARON) {
            encolarEvento("reserva", "No se presentaron al partido del " + r.getInicio().texto() + " en " + cancha +
                          ": no hay devolución y recibieron 0 ★", jugadores);
        } else {
            encolarEvento("reserva", "El dueño de " + cancha + " les dio " + std::to_string(estrellas) + " ★" +
                          (asistencia == Asistencia::LLEGARON_TARDE ? " (llegaron tarde)" : " (todo bien)") +
                          ". ¿Qué tal estuvo la cancha? Ya puedes calificarla", jugadores);
        }
    }
    despacharEventos();
}

void ModeloMatchPoint::jugarConLosQueHay(int organizadorId, int reservaId) {
    {
        std::lock_guard<std::mutex> candado(mtx);
        Reserva &r = reservaValida(reservaId);
        if (r.getOrganizadorId() != organizadorId) throw ReservaError("Solo el organizador decide eso");
        if (!r.buscaJugadores()) throw ReservaError("Ese partido no está buscando jugadores");
        if (r.esContraRival())
            throw ReservaError("Un partido equipo contra equipo necesita rival: si no aparece, se cancela solo "
                               "con devolución total (o puedes cancelarlo tú)");
        r.cubrirCuposVacios();
        std::vector<int> sinRespuesta = r.retirarSolicitudes();
        if (!sinRespuesta.empty())
            encolarEvento("solicitud", "El partido del " + r.getInicio().texto() + " ya no busca jugadores: "
                          "tu solicitud se cerró", sinRespuesta);
        encolarEvento("reserva", "Dejaste de buscar: el partido del " + r.getInicio().texto() +
                      " se juega con los que ya están (los cupos vacíos ya los pagaste)", {organizadorId});
    }
    despacharEventos();
}

void ModeloMatchPoint::revisarConvocatorias() {
    {
        std::lock_guard<std::mutex> candado(mtx);
        long long aviso = minutosLimite() + Configuracion::obtenerInstancia().horasAvisoConvocatoria() * 60LL;
        for (Reserva *r : reservas.filtrar([](const Reserva &x) { return x.buscaJugadores(); })) {
            long long faltan = r->getInicio() - reloj();
            // (si la app estuvo cerrada y el partido ya pasó, también se cancela y se devuelve todo)
            std::string partido = nombreCancha(r->getCanchaId()) + " del " + r->getInicio().texto();
            if (faltan <= minutosLimite()) {
                // Llegó el límite sin completarse: se cancela y se devuelve TODO (no es culpa de nadie)
                Usuario &organizador = usuarioValido(r->getOrganizadorId());
                Usuario &dueno = usuarioValido(buscarCancha(r->getCanchaId())->getDuenoId());
                pagar(dueno, organizador, r->total(), "Devolución: partido incompleto", true);
                for (int p : r->getParticipantes())
                    if (p != organizador.getId())
                        pagar(organizador, usuarioValido(p), r->parte(), "Devolución: partido incompleto", true);
                r->setEstado(EstadoReserva::CANCELADA);
                std::vector<int> avisar = r->getParticipantes();
                for (int s : r->retirarSolicitudes()) avisar.push_back(s);
                encolarEvento("reserva", "El partido " + partido + " no se completó a tiempo: se canceló y se "
                              "devolvió todo el dinero", avisar);
                encolarEvento("reserva", "Se canceló la reserva " + partido + " porque no se completó (sin multa)",
                              {dueno.getId()});
            } else if (faltan <= aviso && !r->getAvisoConvocatoria()) {
                r->setAvisoConvocatoria(true);
                encolarEvento("reserva", "Tu partido " + partido + " aún busca " + std::to_string(r->lugaresLibres()) +
                              " jugadores. Si no se completa 24 h antes, se cancelará con devolución total. "
                              "Puedes pulsar \"Jugar así\" para jugar con los que ya están", {r->getOrganizadorId()});
            }
        }
    }
    despacharEventos();
}

void ModeloMatchPoint::calificarCancha(int jugadorId, int reservaId, int estrellas) {
    {
        std::lock_guard<std::mutex> candado(mtx);
        Reserva &r = reservaValida(reservaId);
        if (!juegaEn(r, jugadorId)) throw ReservaError("No participaste en ese partido");
        if (r.getEstado() != EstadoReserva::JUGADA) throw ReservaError("Solo se califican partidos jugados");
        if (r.yaCalifico(jugadorId)) throw ReservaError("Ya calificaste este partido");
        Cancha *c = buscarCancha(r.getCanchaId());
        c->agregarCalificacion(estrellas);
        r.registrarCalificacion(jugadorId);
        encolarEvento("cancha", "Tu cancha " + c->getNombre() + " recibió " + std::to_string(estrellas) +
                      " estrellas", {c->getDuenoId()});
    }
    despacharEventos();
}

std::vector<Reserva *> ModeloMatchPoint::reservasDe(int jugadorId) const {
    auto lista = reservas.filtrar([this, jugadorId](const Reserva &r) { return juegaEn(r, jugadorId); });
    std::sort(lista.begin(), lista.end(), [](Reserva *a, Reserva *b) { return b->getInicio() < a->getInicio(); });
    return lista;
}

std::vector<Reserva *> ModeloMatchPoint::partidosAbiertos(int paraJugadorId) const {
    FechaHora ahoraMismo = reloj();
    auto lista = reservas.filtrar([&](const Reserva &r) {   // incluye los que ya tienen mi solicitud
        return r.getEstado() == EstadoReserva::ABIERTA && ahoraMismo < r.getInicio() &&
               !juegaEn(r, paraJugadorId);
    });
    std::sort(lista.begin(), lista.end(), [](Reserva *a, Reserva *b) { return a->getInicio() < b->getInicio(); });
    return lista;
}

std::vector<Reserva *> ModeloMatchPoint::reservasRecibidas(int duenoId) const {
    auto lista = reservas.filtrar([&](const Reserva &r) {
        Cancha *c = canchas.buscar(r.getCanchaId());
        return c && c->getDuenoId() == duenoId;
    });
    std::sort(lista.begin(), lista.end(), [](Reserva *a, Reserva *b) { return b->getInicio() < a->getInicio(); });
    return lista;
}

// ===================== Equipos y torneos =====================

Equipo *ModeloMatchPoint::crearEquipo(int capitanId, const std::string &nombre,
                                        const std::vector<std::string> &titulares,
                                        const std::vector<std::string> &suplentes) {
    Equipo *e;
    {
        std::lock_guard<std::mutex> candado(mtx);
        Usuario &capitan = usuarioValido(capitanId);
        if (!capitan.puedeReservar()) throw TorneoError("Solo los jugadores pueden crear equipos");
        if (nombre.empty()) throw ValidacionError("El equipo necesita un nombre");
        if (equipos.buscarSi([&](const Equipo &x) { return x.getNombre() == nombre; }))
            throw TorneoError("Ya existe un equipo con ese nombre");

        auto nuevo = std::make_unique<Equipo>(nombre, capitanId);
        std::vector<int> usados{capitanId};
        // COMPOSITE: se agregan hojas (jugadores) dentro de los grupos
        nuevo->getTitulares().agregar(std::make_unique<MiembroEquipo>(capitanId, capitan.getNombre()));
        auto agregarA = [&](GrupoEquipo &grupo, const std::vector<std::string> &nombres) {
            for (const auto &n : nombres) {
                Usuario *j = buscarJugadorPorNombre(n);
                if (!j) throw TorneoError("No existe el jugador \"" + n + "\"");
                if (std::find(usados.begin(), usados.end(), j->getId()) != usados.end()) continue;
                usados.push_back(j->getId());
                grupo.agregar(std::make_unique<MiembroEquipo>(j->getId(), j->getNombre()));
            }
        };
        agregarA(nuevo->getTitulares(), titulares);
        agregarA(nuevo->getSuplentes(), suplentes);
        e = equipos.agregar(std::move(nuevo));
        encolarEvento("torneo", "Fuiste agregado al equipo " + nombre, e->idsJugadores());
    }
    despacharEventos();
    return e;
}

std::vector<Equipo *> ModeloMatchPoint::equiposDe(int jugadorId) const {
    return equipos.filtrar([jugadorId](const Equipo &e) {
        auto ids = e.idsJugadores();
        return std::find(ids.begin(), ids.end(), jugadorId) != ids.end();
    });
}

Torneo *ModeloMatchPoint::crearTorneo(int duenoId, int canchaId, const std::string &nombre,
                                        const Dinero &costo, int maxEquipos) {
    Torneo *t;
    {
        std::lock_guard<std::mutex> candado(mtx);
        canchaDelDueno(duenoId, canchaId);
        t = torneos.agregar(std::make_unique<Torneo>(duenoId, canchaId, nombre, costo, maxEquipos));
        std::vector<int> jugadores;
        for (Usuario *u : usuarios.filtrar([](const Usuario &x) { return x.puedeReservar(); }))
            jugadores.push_back(u->getId());
        encolarEvento("torneo", "Nuevo torneo \"" + nombre + "\" en " + nombreCancha(canchaId) +
                      ". Inscripción: " + costo.texto(), jugadores);
    }
    despacharEventos();
    return t;
}

void ModeloMatchPoint::inscribirEquipo(int capitanId, int torneoId, int equipoId) {
    {
        std::lock_guard<std::mutex> candado(mtx);
        Torneo *t = torneos.buscar(torneoId);
        Equipo *e = equipos.buscar(equipoId);
        if (!t || !e) throw TorneoError("El torneo o el equipo no existe");
        if (e->getCapitanId() != capitanId) throw TorneoError("Solo el capitán inscribe al equipo");
        if (e->cantidadJugadores() < 5) throw TorneoError("El equipo necesita al menos 5 jugadores");
        // Un jugador no puede estar en dos equipos del mismo torneo
        for (int otroId : t->getEquipos())
            if (otroId != equipoId) for (int j : equipos.buscar(otroId)->idsJugadores()) {
                auto mios = e->idsJugadores();
                if (std::find(mios.begin(), mios.end(), j) != mios.end())
                    throw TorneoError(usuarioValido(j).getNombre() + " ya juega en " + equipos.buscar(otroId)->getNombre() +
                                      ", inscrito en este torneo");
            }
        t->inscribir(equipoId);   // valida cupos y estado
        try {
            pagar(usuarioValido(capitanId), usuarioValido(t->getDuenoId()), t->getCostoInscripcion(),
                  "Inscripción al torneo " + t->getNombre());
        } catch (const PagoError &) {
            // Si el banco rechaza el pago se deshace la inscripción
            auto eq = t->getEquipos();
            eq.pop_back();
            t->restaurar(t->getEstado(), eq, t->getPartidos());
            throw;
        }
        encolarEvento("torneo", "El equipo " + e->getNombre() + " se inscribió en " + t->getNombre(),
                      {t->getDuenoId()});
        encolarEvento("torneo", "Tu equipo " + e->getNombre() + " está inscrito en " + t->getNombre(),
                      e->idsJugadores());
    }
    despacharEventos();
}

void ModeloMatchPoint::iniciarTorneo(int duenoId, int torneoId) {
    {
        std::lock_guard<std::mutex> candado(mtx);
        Torneo *t = torneos.buscar(torneoId);
        if (!t || t->getDuenoId() != duenoId) throw TorneoError("Ese torneo no es tuyo");
        t->iniciar();
        std::vector<int> jugadores;
        for (int eq : t->getEquipos())
            for (int j : equipos.buscar(eq)->idsJugadores()) jugadores.push_back(j);
        encolarEvento("torneo", "¡Comenzó el torneo " + t->getNombre() + "! Ya está el fixture", jugadores);
    }
    despacharEventos();
}

void ModeloMatchPoint::registrarResultado(int duenoId, int torneoId, int partido, int golesLocal,
                                            int golesVisitante) {
    {
        std::lock_guard<std::mutex> candado(mtx);
        Torneo *t = torneos.buscar(torneoId);
        if (!t || t->getDuenoId() != duenoId) throw TorneoError("Ese torneo no es tuyo");
        t->registrarResultado(partido, golesLocal, golesVisitante);
        const auto &p = t->getPartidos()[partido];
        std::vector<int> jugadores = equipos.buscar(p.localId)->idsJugadores();
        for (int j : equipos.buscar(p.visitanteId)->idsJugadores()) jugadores.push_back(j);
        encolarEvento("torneo", equipos.buscar(p.localId)->getNombre() + " " + std::to_string(golesLocal) + " - " +
                      std::to_string(golesVisitante) + " " + equipos.buscar(p.visitanteId)->getNombre(), jugadores);
        if (t->getEstado() == EstadoTorneo::FINALIZADO) {
            auto tabla = t->tablaPosiciones();
            if (tabla.size() > 1 && !(tabla[0] > tabla[1])) {
                encolarEvento("torneo", t->getNombre() + " terminó con empate en el primer lugar", jugadores);
            } else {
                Equipo *campeon = equipos.buscar(tabla.front().equipoId);
                encolarEvento("torneo", "🏆 " + campeon->getNombre() + " es campeón de " + t->getNombre(),
                              campeon->idsJugadores());
            }
        }
    }
    despacharEventos();
}

// ===================== Rankings y notificaciones =====================

std::vector<Cancha *> ModeloMatchPoint::rankingCanchas() const {
    auto lista = canchas.todos();
    // Usa el operator< de Cancha: la mejor calificada primero
    std::sort(lista.begin(), lista.end(), [](Cancha *a, Cancha *b) { return *b < *a; });
    return lista;
}

std::vector<Usuario *> ModeloMatchPoint::rankingJugadores() const {
    auto lista = usuarios.filtrar([](const Usuario &u) { return u.puedeReservar(); });
    // Mejor promedio primero; a igual promedio, quien tiene más calificaciones. Los nuevos van al final.
    std::sort(lista.begin(), lista.end(), [](Usuario *a, Usuario *b) {
        if (a->esNuevo() != b->esNuevo()) return b->esNuevo();
        if (a->getReputacion() != b->getReputacion()) return a->getReputacion() > b->getReputacion();
        return a->getCalificaciones().size() > b->getCalificaciones().size();
    });
    return lista;
}

std::vector<Notificacion> ModeloMatchPoint::notificacionesDe(int usuarioId) const {
    std::vector<Notificacion> lista;
    for (auto it = notificaciones.rbegin(); it != notificaciones.rend(); ++it)
        if (it->usuarioId == usuarioId) lista.push_back(*it);
    // La más reciente primero (stable_sort mantiene el orden de las que tienen la misma hora)
    std::stable_sort(lista.begin(), lista.end(),
                     [](const Notificacion &a, const Notificacion &b) { return b.fecha < a.fecha; });
    return lista;
}
