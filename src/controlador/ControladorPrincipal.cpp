// ControladorPrincipal.cpp
#include "ControladorPrincipal.h"
#include <QSet>

static QString q(const std::string &s) { return QString::fromStdString(s); }
static QString estrellas(double promedio, size_t votos) {
    return votos == 0 ? "Sin calificar" : QString("%1 ★ (%2)").arg(promedio, 0, 'f', 1).arg(votos);
}
static QString cuando(const FechaHora &f) { return q(f.nombreDia() + " " + f.texto()); }
static QString primerNombre(const std::string &nombre) { return q(nombre).section(' ', 0, 0); }

// "Hoy 18:00", "Ayer 21:15" o "12/11 19:30": así las notificaciones se leen agrupadas por día
static QString cuandoRelativo(const FechaHora &f, const FechaHora &ahora) {
    QString hora = q(f.texto()).mid(11, 5);
    long long dias = (ahora.inicioDelDia() - f.inicioDelDia()) / (24 * 60);
    if (dias == 0) return "Hoy " + hora;
    if (dias == 1) return "Ayer " + hora;
    QString fecha = q(f.textoFecha());
    return fecha.mid(8, 2) + "/" + fecha.mid(5, 2) + " " + hora;
}

static QString reputacion(const Usuario &u) {
    return u.esNuevo() ? QString("Nuevo") : QString("%1 ★").arg(u.getReputacion(), 0, 'f', 1);
}

static QString tipoNotificacion(const std::string &tipo) {
    if (tipo == "reserva") return "⚽ Reserva";
    if (tipo == "solicitud") return "📨 Solicitud";
    if (tipo == "pago") return "💰 Pago";
    if (tipo == "torneo") return "🏆 Torneo";
    if (tipo == "cancha") return "🏟 Cancha";
    return "👋 Cuenta";
}

ControladorPrincipal::ControladorPrincipal(VentanaPrincipal *vista, ModeloMatchPoint *modelo, int usuarioId,
                                           QObject *padre)
    : QObject(padre), vista(vista), modelo(modelo), usuarioId(usuarioId) {
    esDueno = usuario().puedeAdministrarCanchas();

    // Vista -> Controlador: señales y slots de Qt
    connect(vista, &VentanaPrincipal::accionSolicitada, this, &ControladorPrincipal::onAccion);
    connect(vista, &VentanaPrincipal::accionEnFila, this, &ControladorPrincipal::onAccionEnFila);
    connect(vista, &VentanaPrincipal::comboCambiado, this, &ControladorPrincipal::onComboCambiado);
    connect(vista, &VentanaPrincipal::seleccionCambiada, this, &ControladorPrincipal::onSeleccionCambiada);
    connect(vista, &VentanaPrincipal::celdaElegida, this, &ControladorPrincipal::onCeldaElegida);
    connect(vista, &VentanaPrincipal::formularioCambiado, this, &ControladorPrincipal::refrescarCotizacion);

    // Modelo -> Vista: OBSERVER. Cualquier cambio en el modelo refresca la ventana
    suscripcion = modelo->suscribir([this](const Evento &) { refrescarTodo(); });
    refrescarTodo();
}

ControladorPrincipal::~ControladorPrincipal() { modelo->desuscribir(suscripcion); }

Usuario &ControladorPrincipal::usuario() const { return *modelo->buscarUsuario(usuarioId); }

// ===================== Acciones del usuario =====================

void ControladorPrincipal::onAccion(Accion accion) {
    if (accion == Accion::CERRAR_SESION) {
        emit sesionCerrada();
        return;
    }
    intentar([this, accion] { ejecutarAccion(accion); });
}

void ControladorPrincipal::onAccionEnFila(Accion accion, int id) {
    intentar([this, accion, id] { ejecutarAccion(accion, id); });
}

std::vector<Equipo *> ControladorPrincipal::equiposComoCapitan() const {
    std::vector<Equipo *> lista;
    for (Equipo *e : modelo->equiposDe(usuarioId))
        if (e->getCapitanId() == usuarioId) lista.push_back(e);
    return lista;
}

QString ControladorPrincipal::textoJugadores(const Reserva *r) const {
    if (r->esContraRival()) {
        QString rival = r->getEquipoRivalId() >= 0 ? q(modelo->buscarEquipo(r->getEquipoRivalId())->getNombre()) : "¿rival?";
        return q(modelo->buscarEquipo(r->getEquipoId())->getNombre()) + " vs " + rival;
    }
    return QString("%1 / %2").arg(r->jugadoresConfirmados()).arg(r->totalJugadores());
}

void ControladorPrincipal::intentar(const std::function<void()> &operacion) {
    // MANEJO DE ERRORES: el Modelo lanza excepciones y aquí se muestran como mensajes
    try {
        operacion();
    } catch (const MatchPointError &e) {
        vista->mostrarMensaje(q(e.what()), false);
    } catch (const std::exception &e) {
        vista->mostrarMensaje("Error inesperado: " + q(e.what()), false);
    }
}

void ControladorPrincipal::ejecutarAccion(Accion accion, int id) {
    switch (accion) {
    case Accion::RESERVAR: {
        if (canchaElegida < 0 || !hayHorario) throw ValidacionError("Elige una cancha y un horario libre en la grilla");
        FormReserva f = vista->formReserva();
        std::vector<std::string> extras;
        for (const QString &e : f.extras) extras.push_back(e.toStdString());
        FechaHora inicio = horarioElegido;   // al reservar, el refresco del Observer lo marca como ocupado
        if (f.modo == ModoReserva::BUSCAR_RIVAL && f.equipoId < 0)
            throw ValidacionError("Para buscar rival debes ser capitán de un equipo (créalo en \"Equipos y torneos\")");
        Reserva *r = modelo->reservar(usuarioId, canchaElegida, inicio, extras, f.somos, f.buscamos,
                                      f.modo == ModoReserva::BUSCAR_RIVAL ? f.equipoId : -1);
        QString comprobante = q(usuario().getBilletera().getHistorial().back().comprobante);
        vista->mostrarMensaje(QString("Reserva confirmada: %1 el %2. Pagaste %3 (comprobante %4)")
                                  .arg(q(modelo->buscarCancha(canchaElegida)->getNombre()), cuando(inicio),
                                       q(r->total().texto()), comprobante), true);
        break;
    }
    case Accion::ACEPTAR_SOLICITUD:
    case Accion::RECHAZAR_SOLICITUD: {
        if (id < 0 || id >= static_cast<int>(solicitudesMostradas.size())) return;
        Solicitud sol = solicitudesMostradas[id];   // el id de la fila es su posición en la lista
        bool aceptar = accion == Accion::ACEPTAR_SOLICITUD;
        modelo->responderSolicitud(usuarioId, sol.reservaId, sol.jugadorId, aceptar);
        QString quien = sol.equipoId >= 0 ? q(modelo->buscarEquipo(sol.equipoId)->getNombre())
                                          : q(modelo->buscarUsuario(sol.jugadorId)->getNombre());
        vista->mostrarMensaje(aceptar ? quien + " ya está en tu partido y te pagó su parte"
                                      : "Rechazaste la solicitud de " + quien, true);
        break;
    }
    case Accion::CANCELAR: {
        Reserva *r = modelo->buscarReserva(id);
        if (!r) return;
        bool tarde = (r->getInicio() - modelo->ahora()) <= Configuracion::obtenerInstancia().horasCancelacionGratis() * 60LL;
        QString aviso = tarde ? "Faltan menos de 24 horas: perderás el 50 % del total. ¿Cancelar de todos modos?"
                              : "Se te devolverá todo el dinero. ¿Cancelar la reserva?";
        if (!vista->confirmar(aviso)) return;
        modelo->cancelar(usuarioId, id);
        vista->mostrarMensaje("Reserva cancelada", true);
        break;
    }
    case Accion::RETIRARSE: {
        if (!vista->confirmar("¿Retirarte del partido? Con menos de 24 h no hay devolución.")) return;
        modelo->retirarse(usuarioId, id);
        vista->mostrarMensaje("Te retiraste del partido", true);
        break;
    }
    case Accion::CALIFICAR: {
        int estrellas = vista->pedirEstrellas();
        if (estrellas == 0) return;
        modelo->calificarCancha(usuarioId, id, estrellas);
        vista->mostrarMensaje("¡Gracias por calificar!", true);
        break;
    }
    case Accion::CREAR_EQUIPO: {
        FormEquipo f = vista->formEquipo();
        std::vector<std::string> t, s;
        for (const QString &n : f.titulares) t.push_back(n.toStdString());
        for (const QString &n : f.suplentes) s.push_back(n.toStdString());
        Equipo *e = modelo->crearEquipo(usuarioId, f.nombre.toStdString(), t, s);
        vista->mostrarMensaje(QString("Equipo %1 creado con %2 jugadores").arg(q(e->getNombre())).arg(e->cantidadJugadores()), true);
        break;
    }
    case Accion::INSCRIBIR_EQUIPO: {
        int equipo = vista->idCombo(Combo::EQUIPO_INSCRIBIR);
        if (equipo < 0) throw ValidacionError("Primero crea un equipo del que seas capitán");
        modelo->inscribirEquipo(usuarioId, id, equipo);
        vista->mostrarMensaje("Equipo inscrito", true);
        break;
    }
    case Accion::REGISTRAR_CANCHA: {
        FormCancha f = vista->formCancha();
        modelo->registrarCancha(usuarioId, f.nombre.toStdString(), f.distrito.toStdString(), f.direccion.toStdString(),
                                Dinero(f.precio), f.apertura, f.cierre);
        vista->mostrarMensaje("Cancha registrada", true);
        break;
    }
    case Accion::ACTUALIZAR_CANCHA: {
        int id = vista->idSeleccionado(Tabla::MIS_CANCHAS);
        if (id < 0) throw ValidacionError("Selecciona la cancha que quieres modificar");
        FormCancha f = vista->formCancha();
        modelo->actualizarCancha(usuarioId, id, f.nombre.toStdString(), f.distrito.toStdString(),
                                 f.direccion.toStdString(), Dinero(f.precio), f.apertura, f.cierre);
        vista->mostrarMensaje("Cambios guardados", true);
        break;
    }
    case Accion::CALIFICAR_TODO_BIEN:
    case Accion::CALIFICAR_TARDE:
    case Accion::CALIFICAR_NO_VINIERON: {
        Asistencia a = accion == Accion::CALIFICAR_TODO_BIEN ? Asistencia::TODO_BIEN
                     : accion == Accion::CALIFICAR_TARDE ? Asistencia::LLEGARON_TARDE : Asistencia::NO_SE_PRESENTARON;
        modelo->calificarPartido(usuarioId, id, a);
        vista->mostrarMensaje("Calificaste a los jugadores de ese partido", true);
        break;
    }
    case Accion::JUGAR_ASI: {
        if (!vista->confirmar("El partido se jugará con los que ya están: los cupos vacíos ya los pagaste y "
                              "las solicitudes pendientes se cerrarán. ¿Continuar?")) return;
        modelo->jugarConLosQueHay(usuarioId, id);
        vista->mostrarMensaje("Listo: el partido ya no busca jugadores y no se cancelará", true);
        break;
    }
    case Accion::CREAR_TORNEO: {
        FormTorneo f = vista->formTorneo();
        if (f.canchaId < 0) throw ValidacionError("Primero registra una cancha");
        modelo->crearTorneo(usuarioId, f.canchaId, f.nombre.toStdString(), Dinero(f.costo), f.maxEquipos);
        vista->mostrarMensaje("Torneo creado: los jugadores ya pueden inscribir sus equipos", true);
        break;
    }
    case Accion::INICIAR_TORNEO: {
        modelo->iniciarTorneo(usuarioId, id);
        vista->mostrarMensaje("Torneo iniciado: fixture generado", true);
        break;
    }
    case Accion::REGISTRAR_RESULTADO: {
        int torneo = vista->idSeleccionado(Tabla::TORNEOS);
        if (torneo < 0) return;
        modelo->registrarResultado(usuarioId, torneo, id, vista->golesLocal(), vista->golesVisitante());
        vista->mostrarMensaje("Resultado registrado", true);
        break;
    }
    case Accion::CERRAR_SESION:
        break;
    }
}

void ControladorPrincipal::onComboCambiado(Combo combo) {
    if (combo == Combo::DISTRITO) refrescarJugador();
}

void ControladorPrincipal::onSeleccionCambiada(Tabla tabla) {
    if (tabla == Tabla::TORNEOS) refrescarDetalleTorneo();
    else if (tabla == Tabla::MIS_EQUIPOS) refrescarDetalleEquipo();
    else if (tabla == Tabla::MIS_CANCHAS) {
        if (Cancha *c = modelo->buscarCancha(vista->idSeleccionado(Tabla::MIS_CANCHAS))) {
            vista->cargarFormularioCancha({q(c->getNombre()), q(c->getDistrito()), q(c->getDireccion()),
                                           c->getPrecioHora().enSoles(),
                                           c->getHoraApertura(), c->getHoraCierre()});
            pintarGrilla(Grilla::MIS_CANCHAS, c->getId());
        }
    } else if (tabla == Tabla::CANCHAS) {
        // Elegir una cancha en la lista muestra su grilla de la semana
        int id = vista->idSeleccionado(Tabla::CANCHAS);
        if (id >= 0 && id != canchaElegida) elegirCancha(id);
    }
}

void ControladorPrincipal::elegirCancha(int canchaId) {
    canchaElegida = canchaId;
    hayHorario = false;
    if (Cancha *c = modelo->buscarCancha(canchaId))
        vista->mostrarDetalleCancha(QString("📍 %1, %2 · Atiende de %3:00 a %4:00")
                                        .arg(q(c->getDireccion()), q(c->getDistrito()))
                                        .arg(c->getHoraApertura()).arg(c->getHoraCierre()));
    pintarGrilla(Grilla::RESERVAR, canchaId);
    refrescarCotizacion();
}

// Clic en una casilla de la grilla: la acción depende del estado de ese horario
void ControladorPrincipal::onCeldaElegida(Grilla grilla, int fila, int columna) {
    intentar([&] {
        int canchaId = grilla == Grilla::RESERVAR ? canchaElegida : vista->idSeleccionado(Tabla::MIS_CANCHAS);
        auto semana = modelo->disponibilidad(canchaId, usuarioId);
        if (columna >= static_cast<int>(semana.size()) || fila >= static_cast<int>(semana[columna].size())) return;
        const Horario h = semana[columna][fila];
        Reserva *r = modelo->buscarReserva(h.reservaId);

        if (grilla == Grilla::MIS_CANCHAS) {   // el dueño solo consulta
            if (!r) vista->mostrarMensaje(cuando(h.inicio) + ": libre · " + q(h.precio.texto()), true);
            else vista->mostrarMensaje(QString("%1: reservado por %2 · %3/%4 jugadores · %5")
                                           .arg(cuando(h.inicio), q(modelo->buscarUsuario(r->getOrganizadorId())->getNombre()))
                                           .arg(r->jugadoresConfirmados()).arg(r->totalJugadores())
                                           .arg(q(textoEstado(r->getEstado()))), true);
            return;
        }
        switch (h.estado) {
        case EstadoHorario::LIBRE:
            hayHorario = true;
            horarioElegido = h.inicio;
            pintarGrilla(Grilla::RESERVAR, canchaElegida);
            refrescarCotizacion();
            break;
        case EstadoHorario::BUSCA_RIVAL: {
            if (r->tieneSolicitud(usuarioId)) {
                vista->mostrarMensaje("Ya retaste a ese equipo: espera la respuesta del capitán", true);
                break;
            }
            auto misEquipos = equiposComoCapitan();
            if (misEquipos.empty())
                throw ReservaError("Para retar a un equipo debes ser capitán de un equipo de 5 o más jugadores");
            QStringList nombres;
            for (Equipo *e : misEquipos) nombres << q(e->getNombre()) + QString(" (%1 jugadores)").arg(e->cantidadJugadores());
            int elegido = vista->elegirOpcion(QString("%1 busca rival para el %2.\nSi te aceptan, tu equipo paga %3. "
                                                      "¿Con qué equipo quieres jugar?")
                                                  .arg(q(modelo->buscarEquipo(r->getEquipoId())->getNombre()), cuando(h.inicio),
                                                       q(r->parte().texto())), nombres);
            if (elegido < 0) break;
            modelo->solicitarUnirse(usuarioId, r->getId(), misEquipos[elegido]->getId());
            vista->mostrarMensaje("Reto enviado. Tu equipo paga su mitad solo si lo aceptan", true);
            break;
        }
        case EstadoHorario::PARTIDO_ABIERTO: {
            if (r->tieneSolicitud(usuarioId)) {
                vista->mostrarMensaje("Ya enviaste tu solicitud: espera la respuesta del organizador", true);
                break;
            }
            QString pregunta = QString("Partido de %1 el %2 (%3/%4 jugadores).\nSi te acepta, pagarás %5. "
                                       "¿Enviar solicitud para unirte?")
                                   .arg(q(modelo->buscarUsuario(r->getOrganizadorId())->getNombre()), cuando(h.inicio))
                                   .arg(r->jugadoresConfirmados()).arg(r->totalJugadores()).arg(q(r->parte().texto()));
            if (!vista->confirmar(pregunta)) break;
            modelo->solicitarUnirse(usuarioId, r->getId());
            vista->mostrarMensaje("Solicitud enviada. Pagarás tu parte solo si el organizador te acepta", true);
            break;
        }
        case EstadoHorario::MI_PARTIDO:
            vista->mostrarMensaje("Ya estás en este partido: míralo en \"Mis reservas\"", true);
            break;
        case EstadoHorario::OCUPADO: {
            if (!r->admiteListaDeEspera()) {
                vista->mostrarMensaje("Ese horario ya está reservado", true);
                break;
            }
            if (r->tieneSolicitud(usuarioId)) {
                vista->mostrarMensaje(QString("Ya estás en la lista de espera (puesto %1)").arg(r->puestoEnEspera(usuarioId)), true);
                break;
            }
            int equipo = -1;   // en equipo contra equipo, se anota un equipo
            if (r->esContraRival()) {
                auto misEquipos = equiposComoCapitan();
                if (misEquipos.empty()) throw ReservaError("Para anotarte necesitas ser capitán de un equipo");
                QStringList nombres;
                for (Equipo *e : misEquipos) nombres << q(e->getNombre());
                int i = vista->elegirOpcion("El partido ya tiene rival. ¿Con qué equipo te anotas en la lista de espera?", nombres);
                if (i < 0) break;
                equipo = misEquipos[i]->getId();
            } else if (!vista->confirmar("El partido está completo. ¿Anotarte en la lista de espera? "
                                         "Si alguien se retira, el organizador podrá aceptarte.")) {
                break;
            }
            modelo->solicitarUnirse(usuarioId, r->getId(), equipo);
            vista->mostrarMensaje("Te anotaste en la lista de espera", true);
            break;
        }
        case EstadoHorario::PASADO:
            vista->mostrarMensaje("Ese horario ya pasó", true);
            break;
        }
    });
}

bool ControladorPrincipal::horarioSigueLibre() const {
    for (const auto &dia : modelo->disponibilidad(canchaElegida, usuarioId))
        for (const Horario &h : dia)
            if (h.inicio == horarioElegido) return h.estado == EstadoHorario::LIBRE;
    return false;
}

std::string ControladorPrincipal::nombreCanchaDe(const Reserva *r) const {
    Cancha *c = modelo->buscarCancha(r->getCanchaId());
    return c ? c->getNombre() : "?";
}

// ===================== Refrescos: Modelo -> Vista =====================

void ControladorPrincipal::refrescarTodo() {
    refrescarComun();
    if (esDueno) refrescarDueno();
    else refrescarJugador();
}

void ControladorPrincipal::refrescarComun() {
    Usuario &u = usuario();
    Dinero saldo = u.getBilletera().getSaldo();
    if (esDueno) {
        vista->mostrarEncabezado(q(u.getNombre()), q(u.getDetalle()) + " · Ingresos: <b style='color:#1b7f3b'>" +
                                                       q(saldo.texto()) + "</b>");
        vista->mostrarResumenPagos("Ingresos acumulados: " + q(saldo.texto()), "Recibes en: " + q(u.medioDePago()));
    } else {
        vista->mostrarEncabezado(q(u.getNombre()), QString("%1 (%2) · %3 · Reputación: %4")
                                                       .arg(q(u.getRol()), q(u.getDetalle()), q(u.medioDePago()),
                                                            reputacion(u)));
        vista->mostrarResumenPagos("Gasto neto en reservas: " + q((Dinero() - saldo).texto()),
                                   "Pagas con: <b>" + q(u.medioDePago()) + "</b>");
    }

    QList<QStringList> movs;
    const auto &h = u.getBilletera().getHistorial();
    for (auto it = h.rbegin(); it != h.rend(); ++it) {
        QString monto = (it->monto < Dinero() ? "- " + q((Dinero() - it->monto).texto())
                                              : "+ " + q(it->monto.texto()));
        movs.append({q(it->fecha.texto()), q(it->comprobante), q(it->concepto), monto});
    }
    vista->mostrarTabla(Tabla::MOVIMIENTOS, movs);

    QList<QStringList> rc;
    int pos = 1;
    for (Cancha *c : modelo->rankingCanchas())
        rc.append({QString::number(pos++), q(c->getNombre()), q(c->getDistrito()),
                   estrellas(c->promedioCalificacion(), c->getCalificaciones().size()),
                   QString::number(c->getCalificaciones().size())});
    vista->mostrarTabla(Tabla::RANKING_CANCHAS, rc);

    QList<QStringList> rj;
    pos = 1;
    for (Usuario *j : modelo->rankingJugadores())
        rj.append({QString::number(pos++), q(j->getNombre()), q(j->getDetalle()), reputacion(*j),
                   QString::number(j->getCalificaciones().size())});
    vista->mostrarTabla(Tabla::RANKING_JUGADORES, rj);

    QList<QStringList> notis;
    int deHoy = 0;
    FechaHora ahora = modelo->ahora();
    for (const auto &n : modelo->notificacionesDe(usuarioId)) {   // ya vienen de la más reciente a la más antigua
        QString cuandoFue = cuandoRelativo(n.fecha, ahora);
        if (cuandoFue.startsWith("Hoy")) deHoy++;
        notis.append({cuandoFue, tipoNotificacion(n.tipo), q(n.mensaje)});
    }
    vista->mostrarTabla(Tabla::NOTIFICACIONES, notis);
    vista->mostrarContadorNotificaciones(deHoy);
}

void ControladorPrincipal::refrescarJugador() {
    // Canchas (con filtro por distrito)
    QSet<QString> distritos;
    for (Cancha *c : modelo->listarCanchas()) distritos.insert(q(c->getDistrito()));
    QStringList listaDistritos = distritos.values();
    listaDistritos.sort();
    listaDistritos.prepend("Todos");
    QList<int> idsDistritos;
    for (int i = 0; i < listaDistritos.size(); ++i) idsDistritos << i;
    QString filtro = vista->textoCombo(Combo::DISTRITO);
    vista->llenarCombo(Combo::DISTRITO, listaDistritos, idsDistritos);
    if (filtro.isEmpty()) filtro = "Todos";

    QList<QStringList> filas;
    QList<int> ids;
    auto abiertos = modelo->partidosAbiertos(usuarioId);
    for (Cancha *c : modelo->listarCanchas()) {
        if (filtro != "Todos" && q(c->getDistrito()) != filtro) continue;
        long cuantos = std::count_if(abiertos.begin(), abiertos.end(),
                                     [c](Reserva *r) { return r->getCanchaId() == c->getId(); });
        filas.append({q(c->getNombre()), q(c->getDistrito()), q(c->getPrecioHora().texto()),
                      estrellas(c->promedioCalificacion(), c->getCalificaciones().size()),
                      cuantos ? QString::number(cuantos) : "—"});
        ids << c->getId();
    }
    vista->mostrarTabla(Tabla::CANCHAS, filas, ids);
    if (!ids.contains(canchaElegida)) {   // al entrar (o al cambiar de distrito) se elige la primera
        vista->seleccionarEnTabla(Tabla::CANCHAS, ids.isEmpty() ? -1 : ids.first());
        elegirCancha(ids.isEmpty() ? -1 : ids.first());
    } else {
        if (hayHorario && !horarioSigueLibre()) hayHorario = false;   // alguien lo tomó o ya pasó
        pintarGrilla(Grilla::RESERVAR, canchaElegida);
        refrescarCotizacion();
    }

    // Mis reservas
    QList<QStringList> mias;
    QList<int> idsMias;
    QList<QList<BotonFila>> botones;
    FechaHora ahora = modelo->ahora();
    for (Reserva *r : modelo->reservasDe(usuarioId)) {
        bool org = r->getOrganizadorId() == usuarioId;
        bool porJugar = r->estaActiva() && ahora < r->getInicio();
        QString estado = q(textoEstado(r->getEstado()));
        if (r->buscaJugadores()) {
            QString limite = cuando(r->getInicio().masMinutos(-Configuracion::obtenerInstancia().horasCancelacionGratis() * 60LL));
            estado = (r->esContraRival() ? QString("Buscando rival") : QString("Faltan %1").arg(r->lugaresLibres())) +
                     " (hasta " + limite.section(' ', 0, 0).left(3) + " " + limite.section(' ', 2, 2) + ")";
        }
        mias.append({q(nombreCanchaDe(r)), cuando(r->getInicio()), q(r->descripcion()), q(r->total().texto()),
                     q(r->parte().texto()), textoJugadores(r), estado,
                     org ? "Organizador" : r->tieneParticipante(usuarioId) ? "Participante" : "Con mi equipo"});
        idsMias << r->getId();
        // Solo los botones que tienen sentido en esa fila
        QList<BotonFila> acciones;
        bool participante = r->tieneParticipante(usuarioId);   // los demás del equipo juegan, pero no pagaron
        if (porJugar && org && r->buscaJugadores() && !r->esContraRival()) acciones.append({"Jugar así", Accion::JUGAR_ASI});
        if (porJugar && org) acciones.append({"Cancelar", Accion::CANCELAR});
        if (porJugar && !org && participante) acciones.append({"Retirarme", Accion::RETIRARSE});
        if (r->getEstado() == EstadoReserva::JUGADA && !r->yaCalifico(usuarioId))
            acciones.append({"Calificar ★", Accion::CALIFICAR});
        botones.append(acciones);
    }
    vista->mostrarTabla(Tabla::MIS_RESERVAS, mias, idsMias, botones);
    refrescarSolicitudes();

    // Equipos (Composite) y torneos
    QList<QStringList> equipos;
    QList<int> idsEquipos;
    QStringList misEquiposCapitan;
    QList<int> idsCapitan;
    for (Equipo *e : modelo->equiposDe(usuarioId)) {
        equipos.append({q(e->getNombre()), q(modelo->buscarUsuario(e->getCapitanId())->getNombre()),
                        QString::number(e->cantidadJugadores())});
        idsEquipos << e->getId();
    }
    for (Equipo *e : equiposComoCapitan()) { misEquiposCapitan << q(e->getNombre()); idsCapitan << e->getId(); }
    vista->mostrarTabla(Tabla::MIS_EQUIPOS, equipos, idsEquipos);
    vista->llenarCombo(Combo::EQUIPO_INSCRIBIR, misEquiposCapitan, idsCapitan);
    vista->llenarCombo(Combo::EQUIPO_RIVAL, misEquiposCapitan, idsCapitan);
    refrescarDetalleEquipo();

    QList<QStringList> torneos;
    QList<int> idsTorneos;
    QList<QList<BotonFila>> botonesTorneo;
    for (Torneo *t : modelo->listarTorneos()) {
        torneos.append({q(t->getNombre()), q(modelo->buscarCancha(t->getCanchaId())->getNombre()),
                        q(t->getCostoInscripcion().texto()),
                        QString("%1 / %2").arg(t->getEquipos().size()).arg(t->getMaxEquipos()),
                        q(textoEstadoTorneo(t->getEstado()))});
        idsTorneos << t->getId();
        QList<BotonFila> b;
        if (t->getEstado() == EstadoTorneo::INSCRIPCIONES && !idsCapitan.isEmpty())
            b.append({"Inscribir mi equipo", Accion::INSCRIBIR_EQUIPO});
        botonesTorneo.append(b);
    }
    vista->mostrarTabla(Tabla::TORNEOS, torneos, idsTorneos, botonesTorneo);
    refrescarDetalleTorneo();
}

void ControladorPrincipal::refrescarDueno() {
    QList<QStringList> canchas;
    QList<int> ids;
    QStringList nombres;
    for (Cancha *c : modelo->canchasDe(usuarioId)) {
        canchas.append({q(c->getNombre()), q(c->getDistrito()), q(c->getDireccion()), q(c->getPrecioHora().texto()),
                        QString("%1:00 - %2:00").arg(c->getHoraApertura()).arg(c->getHoraCierre()),
                        estrellas(c->promedioCalificacion(), c->getCalificaciones().size())});
        ids << c->getId();
        nombres << q(c->getNombre());
    }
    vista->mostrarTabla(Tabla::MIS_CANCHAS, canchas, ids);
    vista->llenarCombo(Combo::CANCHA_TORNEO, nombres, ids);
    if (vista->idSeleccionado(Tabla::MIS_CANCHAS) < 0 && !ids.isEmpty()) {   // al entrar se elige la primera
        vista->seleccionarEnTabla(Tabla::MIS_CANCHAS, ids.first());
        onSeleccionCambiada(Tabla::MIS_CANCHAS);
    } else {
        pintarGrilla(Grilla::MIS_CANCHAS, vista->idSeleccionado(Tabla::MIS_CANCHAS));
    }

    QList<QStringList> recibidas;
    QList<int> idsRec;
    QList<QList<BotonFila>> botonesRec;
    FechaHora ahora = modelo->ahora();
    int tolerancia = Configuracion::obtenerInstancia().toleranciaMinutos();
    auto lista = modelo->reservasRecibidas(usuarioId);
    // Primero las que esperan que el dueño las marque (ya empezaron y siguen activas)
    std::stable_partition(lista.begin(), lista.end(),
                          [&ahora](Reserva *r) { return r->estaActiva() && !(ahora < r->getInicio()); });
    for (Reserva *r : lista) {
        recibidas.append({q(nombreCanchaDe(r)), cuando(r->getInicio()),
                          q(modelo->buscarUsuario(r->getOrganizadorId())->getNombre()), textoJugadores(r),
                          q(r->total().texto()), q(textoEstado(r->getEstado()))});
        idsRec << r->getId();
        QList<BotonFila> b;   // calificar a los jugadores: solo cuando ya empezó; "no vinieron" tras la tolerancia
        if (r->estaActiva() && !(ahora < r->getInicio())) {
            b.append({"⭐ Todo bien", Accion::CALIFICAR_TODO_BIEN});
            b.append({"⏰ Llegaron tarde", Accion::CALIFICAR_TARDE});
        }
        if (r->estaActiva() && ahora - r->getInicio() >= tolerancia) b.append({"❌ No vinieron", Accion::CALIFICAR_NO_VINIERON});
        botonesRec.append(b);
    }
    vista->mostrarTabla(Tabla::RESERVAS_RECIBIDAS, recibidas, idsRec, botonesRec);

    QList<QStringList> torneos;
    QList<int> idsTorneos;
    QList<QList<BotonFila>> botonesTorneo;
    for (Torneo *t : modelo->listarTorneos()) {
        if (t->getDuenoId() != usuarioId) continue;
        torneos.append({q(t->getNombre()), q(modelo->buscarCancha(t->getCanchaId())->getNombre()),
                        q(t->getCostoInscripcion().texto()),
                        QString("%1 / %2").arg(t->getEquipos().size()).arg(t->getMaxEquipos()),
                        q(textoEstadoTorneo(t->getEstado()))});
        idsTorneos << t->getId();
        QList<BotonFila> b;
        if (t->getEstado() == EstadoTorneo::INSCRIPCIONES) b.append({"Iniciar", Accion::INICIAR_TORNEO});
        botonesTorneo.append(b);
    }
    vista->mostrarTabla(Tabla::TORNEOS, torneos, idsTorneos, botonesTorneo);
    refrescarDetalleTorneo();
}

void ControladorPrincipal::refrescarSolicitudes() {
    solicitudesMostradas = modelo->solicitudesPendientes(usuarioId);
    QList<QStringList> filas;
    QList<QList<BotonFila>> botones;
    for (const Solicitud &s : solicitudesMostradas) {
        Reserva *r = modelo->buscarReserva(s.reservaId);
        Usuario *j = modelo->buscarUsuario(s.jugadorId);
        QString quien = q(j->getNombre()), detalle = q(j->getDetalle());
        if (s.equipoId >= 0) {   // reto de un equipo: se muestra el equipo y su capitán
            Equipo *e = modelo->buscarEquipo(s.equipoId);
            quien = "Equipo " + q(e->getNombre());
            detalle = QString("%1 jugadores · cap. %2").arg(e->cantidadJugadores()).arg(primerNombre(j->getNombre()));
        }
        bool enEspera = !r->buscaJugadores();   // el partido está completo
        filas.append({q(nombreCanchaDe(r)) + " · " + cuando(r->getInicio()), quien, detalle, reputacion(*j),
                      q(r->parte().texto()),
                      enEspera ? QString("📋 Lista de espera #%1").arg(r->puestoEnEspera(s.jugadorId)) : "Pendiente"});
        if (enEspera) botones.append({{"✖ Rechazar", Accion::RECHAZAR_SOLICITUD}});
        else botones.append({{"✔ Aceptar", Accion::ACEPTAR_SOLICITUD}, {"✖ Rechazar", Accion::RECHAZAR_SOLICITUD}});
    }
    vista->mostrarTabla(Tabla::SOLICITUDES, filas, {}, botones);   // el id de cada fila es su posición en la lista
    vista->mostrarContadorSolicitudes(static_cast<int>(solicitudesMostradas.size()));

    QList<QStringList> enviadas;
    QList<int> ids;
    for (Reserva *r : modelo->solicitudesEnviadas(usuarioId)) {
        enviadas.append({q(nombreCanchaDe(r)) + " · " + cuando(r->getInicio()),
                         q(modelo->buscarUsuario(r->getOrganizadorId())->getNombre()), textoJugadores(r),
                         q(r->parte().texto()),
                         r->buscaJugadores() ? QString("⏳ Esperando respuesta")
                                             : QString("📋 Lista de espera (puesto %1)").arg(r->puestoEnEspera(usuarioId))});
        ids << r->getId();
    }
    vista->mostrarTabla(Tabla::SOLICITUDES_ENVIADAS, enviadas, ids);
}

// Traduce la disponibilidad del Modelo a casillas de colores para la Vista
void ControladorPrincipal::pintarGrilla(Grilla g, int canchaId) {
    Cancha *c = modelo->buscarCancha(canchaId);
    if (!c) {
        vista->mostrarGrilla(g, "Elige una cancha para ver su disponibilidad", {}, {}, {});
        return;
    }
    auto semana = modelo->disponibilidad(canchaId, usuarioId);
    QStringList dias, horas;
    auto lista = modelo->diasReservables();
    for (size_t d = 0; d < lista.size(); ++d) {
        QString fecha = q(lista[d].textoFecha());
        dias << (d == 0 ? QString("Hoy") : q(lista[d].nombreDia()).left(3)) + " " + fecha.mid(8, 2) + "/" + fecha.mid(5, 2);
    }
    for (int h = c->getHoraApertura(); h < c->getHoraCierre(); ++h) horas << QString("%1:00").arg(h, 2, 10, QChar('0'));

    QList<QList<CeldaGrilla>> celdas;
    for (int fila = 0; fila < horas.size(); ++fila) {
        QList<CeldaGrilla> renglon;
        for (const auto &dia : semana) {
            const Horario &h = dia[fila];
            Reserva *r = modelo->buscarReserva(h.reservaId);
            QString estrella = h.horaPunta ? " ★" : "";
            QString organizador = r ? primerNombre(modelo->buscarUsuario(r->getOrganizadorId())->getNombre()) : "";
            QString jugadores = r ? QString("%1/%2").arg(r->jugadoresConfirmados()).arg(r->totalJugadores()) : "";
            CeldaGrilla celda;
            switch (h.estado) {
            case EstadoHorario::LIBRE:
                if (g == Grilla::RESERVAR && hayHorario && h.inicio == horarioElegido)
                    celda = {"✔ Elegido", "Horario elegido", TipoCelda::ELEGIDA};
                else
                    celda = {"Libre" + estrella, q(h.precio.texto()) + " la hora" +
                             (g == Grilla::RESERVAR ? " · clic para elegir" : ""), TipoCelda::LIBRE};
                break;
            case EstadoHorario::PARTIDO_ABIERTO:
                if (g == Grilla::MIS_CANCHAS)
                    celda = {organizador + " " + jugadores, "Partido abierto, buscando jugadores", TipoCelda::ABIERTA};
                else if (r->tieneSolicitud(usuarioId))
                    celda = {"Solicitado", "Esperando que " + organizador + " te acepte", TipoCelda::ABIERTA};
                else
                    celda = {"Abierto " + jugadores, "Partido de " + organizador + " · tu parte " +
                             q(r->parte().texto()) + " · clic para pedir unirte", TipoCelda::ABIERTA};
                break;
            case EstadoHorario::BUSCA_RIVAL: {
                QString equipo = q(modelo->buscarEquipo(r->getEquipoId())->getNombre());
                if (g == Grilla::MIS_CANCHAS)
                    celda = {equipo + " vs ?", "Busca equipo rival", TipoCelda::RIVAL};
                else if (r->tieneSolicitud(usuarioId))
                    celda = {"Reto enviado", "Esperando que " + equipo + " acepte", TipoCelda::RIVAL};
                else
                    celda = {equipo + " busca rival", "Clic para retarlos con tu equipo · pagarías " +
                             q(r->parte().texto()), TipoCelda::RIVAL};
                break;
            }
            case EstadoHorario::MI_PARTIDO:
                celda = {"Mi partido", q(textoEstado(r->getEstado())) + " · " + jugadores + " jugadores", TipoCelda::MIA};
                break;
            case EstadoHorario::OCUPADO:
                if (g == Grilla::MIS_CANCHAS) celda = {organizador + " ✔", "Reservado", TipoCelda::OCUPADA};
                else if (r->admiteListaDeEspera() && r->tieneSolicitud(usuarioId))
                    celda = {"En espera", "Estás en la lista de espera", TipoCelda::OCUPADA};
                else if (r->admiteListaDeEspera())
                    celda = {"Completo", "Partido completo · clic para anotarte en la lista de espera", TipoCelda::OCUPADA};
                else celda = {"Ocupado", "Reservado", TipoCelda::OCUPADA};
                break;
            case EstadoHorario::PASADO:
                celda = {r && g == Grilla::MIS_CANCHAS ? organizador : "", "Este horario ya pasó", TipoCelda::PASADA};
                break;
            }
            renglon << celda;
        }
        celdas << renglon;
    }
    QString titulo = "<b>" + q(c->getNombre()) + "</b> · próximos 7 días" +
                     (g == Grilla::RESERVAR ? " · haz clic en un horario verde para reservar" : " · ocupación de tu cancha");
    vista->mostrarGrilla(g, titulo, dias, horas, celdas);
}

void ControladorPrincipal::refrescarCotizacion() {
    if (esDueno) return;
    Cancha *c = modelo->buscarCancha(canchaElegida);
    if (!c || !hayHorario) {
        vista->mostrarHorarioElegido(c ? "Haz clic en un horario verde de la grilla" : "Elige una cancha");
        vista->mostrarCotizacion("");
        return;
    }
    vista->mostrarHorarioElegido(q(c->getNombre()) + " · " + cuando(horarioElegido));
    FormReserva f = vista->formReserva();
    std::vector<std::string> extras;
    for (const QString &e : f.extras) extras.push_back(e.toStdString());
    Dinero total = modelo->cotizar(canchaElegida, horarioElegido, extras);
    int totalJugadores = f.somos + f.buscamos;
    QString texto;
    if (f.modo == ModoReserva::PRIVADA) texto = QString("Reserva privada para %1 jugadores").arg(f.somos);
    else if (f.modo == ModoReserva::BUSCAR_JUGADORES)
        texto = QString("Son %1 y buscan %2 (%3 en total)").arg(f.somos).arg(f.buscamos).arg(totalJugadores);
    else texto = f.equipoId < 0 ? QString("Necesitas ser capitán de un equipo")
                                : "Equipo " + q(modelo->buscarEquipo(f.equipoId)->getNombre()) + " busca rival";
    texto += "\nTotal: " + q(total.texto()) + " (lo pagas ahora)";
    if (horarioElegido.getHora() >= Configuracion::obtenerInstancia().horaPuntaInicio()) texto += " · hora punta ★";
    if (f.modo == ModoReserva::BUSCAR_JUGADORES)
        texto += "\nCada jugador que aceptes te pagará " + q((total / totalJugadores).texto()) +
                 " · tienes hasta 24 h antes para completarlo";
    if (f.modo == ModoReserva::BUSCAR_RIVAL) texto += "\nEl equipo rival que aceptes te pagará la mitad: " + q((total / 2).texto());
    vista->mostrarCotizacion(texto);
}

void ControladorPrincipal::refrescarDetalleTorneo() {
    Torneo *t = modelo->buscarTorneo(vista->idSeleccionado(Tabla::TORNEOS));
    if (!t) {
        vista->mostrarDetalleTorneo("Selecciona un torneo para ver su fixture y tabla");
        vista->mostrarTabla(Tabla::FIXTURE, {});
        vista->mostrarTabla(Tabla::POSICIONES, {});
        return;
    }
    auto nombreEquipo = [this](int id) { return q(modelo->buscarEquipo(id)->getNombre()); };
    QString detalle = "<b>" + q(t->getNombre()) + "</b> · " + q(textoEstadoTorneo(t->getEstado()));
    if (t->getEstado() == EstadoTorneo::FINALIZADO) {
        auto tabla = t->tablaPosiciones();
        if (tabla.size() > 1 && !(tabla[0] > tabla[1])) detalle += " · Empate en el primer lugar";
        else detalle += " · 🏆 Campeón: " + nombreEquipo(tabla.front().equipoId);
    }
    vista->mostrarDetalleTorneo(detalle);

    QList<QStringList> fixture;
    QList<int> idsPartidos;
    QList<QList<BotonFila>> botones;
    int i = 0;
    for (const auto &p : t->getPartidos()) {
        fixture.append({QString::number(i + 1), nombreEquipo(p.localId), nombreEquipo(p.visitanteId),
                        p.jugado ? QString("%1 - %2").arg(p.golesLocal).arg(p.golesVisitante) : "Pendiente"});
        idsPartidos << i++;
        QList<BotonFila> b;
        if (esDueno && !p.jugado) b.append({"Registrar", Accion::REGISTRAR_RESULTADO});
        botones.append(b);
    }
    vista->mostrarTabla(Tabla::FIXTURE, fixture, idsPartidos, botones);

    QList<QStringList> tabla;
    int pos = 1;
    for (const auto &f : t->tablaPosiciones())
        tabla.append({QString::number(pos++), nombreEquipo(f.equipoId), QString::number(f.pj), QString::number(f.pg),
                      QString::number(f.pe), QString::number(f.pp), QString::number(f.gf), QString::number(f.gc),
                      QString::number(f.diferencia()), QString::number(f.puntos())});
    vista->mostrarTabla(Tabla::POSICIONES, tabla);
}

void ControladorPrincipal::refrescarDetalleEquipo() {
    if (esDueno) return;
    Equipo *e = modelo->buscarEquipo(vista->idSeleccionado(Tabla::MIS_EQUIPOS));
    // COMPOSITE: describir() recorre el árbol equipo -> grupos -> jugadores
    vista->mostrarDetalleEquipo(e ? q(e->describir(0)) : "Selecciona un equipo para ver sus integrantes");
}
