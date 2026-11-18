// ModeloMatchPoint.h
// MODELO de MVC: datos y reglas de negocio. No conoce Qt ni las ventanas.
// Avisa los cambios con el patrón OBSERVER (lambdas suscritas).
#pragma once
#include "Cancha.h"
#include "Equipo.h"
#include "Repositorio.h"
#include "Reserva.h"
#include "Torneo.h"
#include "UsuarioFactory.h"
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

// Lo que se envía a los observadores cuando algo cambia
struct Evento {
    std::string tipo;               // "reserva", "pago", "torneo", "usuario", "cancha"
    std::string mensaje;
    std::vector<int> destinatarios; // usuarios a los que les interesa
};

// Un jugador que pidió unirse a un partido y espera respuesta del organizador
struct Solicitud {
    int reservaId;
    int jugadorId;   // el jugador, o el capitán si es un equipo rival
    int equipoId;    // -1 si es un jugador suelto
};

// Lo que el dueño marca después del partido (cada opción da estrellas a los jugadores)
enum class Asistencia { TODO_BIEN, LLEGARON_TARDE, NO_SE_PRESENTARON };

// Cómo está cada hora de la grilla semanal de una cancha
enum class EstadoHorario { LIBRE, OCUPADO, PARTIDO_ABIERTO, BUSCA_RIVAL, MI_PARTIDO, PASADO };

struct Horario {
    FechaHora inicio;
    EstadoHorario estado;
    int reservaId;       // -1 si está libre
    bool horaPunta;
    Dinero precio;       // precio de la hora con la tarifa aplicada
};

struct Notificacion {
    int usuarioId;
    FechaHora fecha;
    std::string mensaje;
    std::string tipo;   // "reserva", "solicitud", "pago", "torneo", "cancha", "usuario"
};

class ModeloMatchPoint {
public:
    using Observador = std::function<void(const Evento &)>;
    using Reloj = std::function<FechaHora()>;
    using Tarifa = std::function<Dinero(const Dinero &precioHora, const FechaHora &inicio)>;

    // El reloj se inyecta para poder probar con fechas fijas
    explicit ModeloMatchPoint(Reloj reloj = FechaHora::ahora);

    // Devuelve un número para poder desuscribirse (por ejemplo, al cerrar sesión)
    int suscribir(Observador obs) {
        std::lock_guard<std::mutex> candado(mtx);
        observadores[siguienteObservador] = std::move(obs);
        return siguienteObservador++;
    }
    void desuscribir(int id) {
        std::lock_guard<std::mutex> candado(mtx);
        observadores.erase(id);
    }
    FechaHora ahora() const { return reloj(); }
    // Solo para generar el historial de ejemplo: se "viaja" a días pasados y luego se vuelve
    Reloj cambiarReloj(Reloj nuevo) {   // devuelve el reloj anterior para poder restaurarlo
        Reloj anterior = std::move(reloj);
        reloj = std::move(nuevo);
        return anterior;
    }

    // ---------- Usuarios ----------
    Usuario *registrarUsuario(const std::string &rol, const std::string &nombre, const std::string &correo,
                              const std::string &clave, const std::string &detalle,
                              const std::string &tarjeta = "");
    Usuario *iniciarSesion(const std::string &correo, const std::string &clave) const;
    Usuario *buscarUsuario(int id) const { return usuarios.buscar(id); }
    Usuario *buscarJugadorPorNombre(const std::string &nombre) const;

    // ---------- Canchas ----------
    Cancha *registrarCancha(int duenoId, const std::string &nombre, const std::string &distrito,
                            const std::string &direccion, const Dinero &precio, int apertura, int cierre);
    void actualizarCancha(int duenoId, int canchaId, const std::string &nombre, const std::string &distrito,
                          const std::string &direccion, const Dinero &precio, int apertura, int cierre);
    Cancha *buscarCancha(int id) const { return canchas.buscar(id); }
    std::vector<Cancha *> listarCanchas() const { return canchas.todos(); }
    std::vector<Cancha *> canchasDe(int duenoId) const;
    std::vector<FechaHora> diasReservables() const;
    // Grilla de la semana: [día][hora desde la apertura]. paraUsuarioId marca "mis partidos"
    std::vector<std::vector<Horario>> disponibilidad(int canchaId, int paraUsuarioId = -1) const;
    Dinero cotizar(int canchaId, const FechaHora &inicio, const std::vector<std::string> &extras) const;

    // ---------- Reservas y pagos ----------
    Reserva *reservar(int jugadorId, int canchaId, const FechaHora &inicio,
                      const std::vector<std::string> &extras, int grupo, int buscados = 0, int equipoId = -1);
    // equipoId >= 0: el capitán pide jugar con su equipo contra el del organizador
    void solicitarUnirse(int jugadorId, int reservaId, int equipoId = -1);
    void responderSolicitud(int organizadorId, int reservaId, int jugadorId, bool aceptar);
    std::vector<Solicitud> solicitudesPendientes(int organizadorId) const;   // las que recibí
    std::vector<Reserva *> solicitudesEnviadas(int jugadorId) const;          // partidos a los que pedí unirme
    void retirarse(int jugadorId, int reservaId);
    void cancelar(int jugadorId, int reservaId);
    void calificarPartido(int duenoId, int reservaId, Asistencia asistencia);   // reputación bidireccional
    void jugarConLosQueHay(int organizadorId, int reservaId);   // deja de buscar: cubre los cupos vacíos
    // Partidos que no se completan: aviso unas horas antes del límite y cancelación con devolución total
    // al llegar a él (24 h antes). La interfaz la llama cada cierto tiempo.
    void revisarConvocatorias();
    void calificarCancha(int jugadorId, int reservaId, int estrellas);
    Reserva *buscarReserva(int id) const { return reservas.buscar(id); }
    std::vector<Reserva *> reservasDe(int jugadorId) const;
    std::vector<Reserva *> partidosAbiertos(int paraJugadorId) const;
    std::vector<Reserva *> reservasRecibidas(int duenoId) const;

    // ---------- Equipos y torneos ----------
    Equipo *crearEquipo(int capitanId, const std::string &nombre,
                        const std::vector<std::string> &titulares, const std::vector<std::string> &suplentes);
    Equipo *buscarEquipo(int id) const { return equipos.buscar(id); }
    std::vector<Equipo *> equiposDe(int jugadorId) const;
    Torneo *crearTorneo(int duenoId, int canchaId, const std::string &nombre, const Dinero &costo, int maxEquipos);
    void inscribirEquipo(int capitanId, int torneoId, int equipoId);
    void iniciarTorneo(int duenoId, int torneoId);
    void registrarResultado(int duenoId, int torneoId, int partido, int golesLocal, int golesVisitante);
    Torneo *buscarTorneo(int id) const { return torneos.buscar(id); }
    std::vector<Torneo *> listarTorneos() const { return torneos.todos(); }

    // ---------- Rankings y notificaciones ----------
    std::vector<Cancha *> rankingCanchas() const;
    std::vector<Usuario *> rankingJugadores() const;
    std::vector<Notificacion> notificacionesDe(int usuarioId) const;

    // ---------- Acceso para la persistencia ----------
    Repositorio<Usuario> &repoUsuarios() { return usuarios; }
    Repositorio<Cancha> &repoCanchas() { return canchas; }
    Repositorio<Reserva> &repoReservas() { return reservas; }
    Repositorio<Equipo> &repoEquipos() { return equipos; }
    Repositorio<Torneo> &repoTorneos() { return torneos; }
    const std::vector<Notificacion> &todasLasNotificaciones() const { return notificaciones; }
    int getUltimoComprobante() const { return ultimoComprobante; }
    void setUltimoComprobante(int n) { ultimoComprobante = n; }
    void restaurarNotificacion(const Notificacion &n) { notificaciones.push_back(n); }

private:
    Repositorio<Usuario> usuarios;
    Repositorio<Cancha> canchas;
    Repositorio<Reserva> reservas;
    Repositorio<Equipo> equipos;
    Repositorio<Torneo> torneos;
    std::vector<Notificacion> notificaciones;
    std::map<int, Observador> observadores;
    int siguienteObservador = 1;
    std::vector<Evento> pendientes;   // eventos por enviar al terminar la operación
    int ultimoComprobante = 0;        // numeración de los comprobantes de pago
    Reloj reloj;
    Tarifa tarifa;                    // lambda que calcula el precio según la hora

    // MULTITHREADING: protege horarios, cupos y pagos para que dos operaciones
    // simultáneas no reserven el mismo horario ni ocupen el mismo cupo.
    mutable std::mutex mtx;

    Usuario &usuarioValido(int id) const;
    Reserva &reservaValida(int id) const;
    Cancha &canchaDelDueno(int duenoId, int canchaId) const;
    std::string nombreCancha(int canchaId) const;
    bool horarioOcupado(int canchaId, const FechaHora &inicio) const;
    void validarJugadorLibre(int jugadorId, const FechaHora &inicio) const;   // no estar en dos partidos a la vez
    void validarEquipoDelCapitan(int capitanId, int equipoId) const;
    std::vector<int> jugadoresDe(const Reserva &r) const;   // participantes y, si es equipo vs equipo, todos sus jugadores
    bool juegaEn(const Reserva &r, int jugadorId) const;
    void validarEquiposSinRepetidos(int equipoA, int equipoB) const;   // nadie juega en los dos equipos
    long long minutosLimite() const;                         // límite para completar un partido (24 h)
    // Pasarela de pago simulada: cobra con la tarjeta guardada y emite un comprobante
    std::string pagar(Usuario &origen, Usuario &destino, const Dinero &monto, const std::string &concepto,
                      bool esDevolucion = false);
    void encolarEvento(const std::string &tipo, const std::string &mensaje, const std::vector<int> &destinos);
    void despacharEventos();
};
