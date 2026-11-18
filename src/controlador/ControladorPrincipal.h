// ControladorPrincipal.h
// CONTROLADOR de MVC: recibe las señales de la Vista, llama al Modelo y
// traduce los datos del Modelo a textos para la Vista.
#pragma once
#include "../modelo/ModeloMatchPoint.h"
#include "../vista/VentanaPrincipal.h"
#include <QObject>
#include <functional>
#include <vector>

class ControladorPrincipal : public QObject {
    Q_OBJECT
public:
    ControladorPrincipal(VentanaPrincipal *vista, ModeloMatchPoint *modelo, int usuarioId, QObject *padre = nullptr);
    ~ControladorPrincipal() override;

signals:
    void sesionCerrada();

private slots:
    void onAccion(Accion accion);
    void onAccionEnFila(Accion accion, int id);   // botón dentro de una fila
    void onComboCambiado(Combo combo);
    void onSeleccionCambiada(Tabla tabla);
    void onCeldaElegida(Grilla grilla, int fila, int columna);

private:
    VentanaPrincipal *vista;
    ModeloMatchPoint *modelo;
    int usuarioId;
    int suscripcion;
    bool esDueno;
    int canchaElegida = -1;          // cancha cuya grilla se muestra al jugador
    bool hayHorario = false;         // si ya hizo clic en un horario libre
    FechaHora horarioElegido;
    std::vector<Solicitud> solicitudesMostradas;

    Usuario &usuario() const;
    std::string nombreCanchaDe(const Reserva *r) const;
    void ejecutarAccion(Accion accion, int id = -1);
    std::vector<Equipo *> equiposComoCapitan() const;   // equipos con los que puedo retar o inscribirme
    QString textoJugadores(const Reserva *r) const;      // "8 / 10" o "Los Pumas vs Halcones FC"
    void intentar(const std::function<void()> &operacion);   // muestra las excepciones como mensajes
    void elegirCancha(int canchaId);
    bool horarioSigueLibre() const;

    // Refrescos (Modelo -> Vista)
    void refrescarTodo();
    void refrescarComun();
    void refrescarJugador();
    void refrescarDueno();
    void refrescarCotizacion();
    void pintarGrilla(Grilla g, int canchaId);
    void refrescarSolicitudes();
    void refrescarDetalleTorneo();
    void refrescarDetalleEquipo();
};
