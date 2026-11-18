// VentanaPrincipal.h
// VISTA de MVC: ventana con pestañas. No conoce el Modelo (ni Cancha, ni Reserva):
// recibe textos ya preparados por el Controlador y emite señales con lo que pide el usuario.
#pragma once
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include "Tablas.h"

enum class Tabla { CANCHAS, MIS_RESERVAS, SOLICITUDES, SOLICITUDES_ENVIADAS, MIS_EQUIPOS, TORNEOS, FIXTURE, POSICIONES,
                   MOVIMIENTOS, RANKING_CANCHAS, RANKING_JUGADORES, MIS_CANCHAS, RESERVAS_RECIBIDAS, NOTIFICACIONES };
enum class Combo { DISTRITO, EQUIPO_INSCRIBIR, CANCHA_TORNEO, EQUIPO_RIVAL };
enum class Grilla { RESERVAR, MIS_CANCHAS };   // grilla del jugador y grilla del dueño
enum class Accion { RESERVAR, ACEPTAR_SOLICITUD, RECHAZAR_SOLICITUD, CANCELAR, RETIRARSE,
                    CALIFICAR, CREAR_EQUIPO, INSCRIBIR_EQUIPO, REGISTRAR_CANCHA,
                    ACTUALIZAR_CANCHA, CALIFICAR_TODO_BIEN, CALIFICAR_TARDE, CALIFICAR_NO_VINIERON, JUGAR_ASI, CREAR_TORNEO, INICIAR_TORNEO,
                    REGISTRAR_RESULTADO, CERRAR_SESION };

// Botón dentro de una fila de tabla: al pulsarlo se emite accionEnFila(accion, id de la fila)
struct BotonFila {
    QString texto;
    Accion accion;
};

// Datos de los formularios (solo texto y números, sin clases del Modelo)
enum class ModoReserva { PRIVADA, BUSCAR_JUGADORES, BUSCAR_RIVAL };
struct FormReserva { QStringList extras; ModoReserva modo; int somos; int buscamos; int equipoId; };
struct FormEquipo { QString nombre; QStringList titulares, suplentes; };
struct FormCancha { QString nombre, distrito, direccion; double precio; int apertura, cierre; };
struct FormTorneo { int canchaId; QString nombre; double costo; int maxEquipos; };

class VentanaPrincipal : public QMainWindow {
    Q_OBJECT
public:
    explicit VentanaPrincipal(bool esDueno, QWidget *padre = nullptr);

    // ---- Órdenes que recibe del Controlador (solo presentación) ----
    void mostrarEncabezado(const QString &nombre, const QString &detalle);
    void mostrarGrilla(Grilla g, const QString &titulo, const QStringList &dias, const QStringList &horas,
                       const QList<QList<CeldaGrilla>> &celdas);
    void mostrarHorarioElegido(const QString &texto);
    void mostrarDetalleCancha(const QString &texto);
    void mostrarResumenPagos(const QString &resumen, const QString &medioDePago);
    void mostrarContadorSolicitudes(int cantidad);
    void mostrarTabla(Tabla t, const QList<QStringList> &filas, const QList<int> &ids = {},
                      const QList<QList<BotonFila>> &botones = {});
    void llenarCombo(Combo c, const QStringList &textos, const QList<int> &ids);
    void seleccionarEnCombo(Combo c, int id);
    void mostrarCotizacion(const QString &texto);
    void mostrarDetalleTorneo(const QString &texto);
    void mostrarDetalleEquipo(const QString &texto);
    void mostrarContadorNotificaciones(int deHoy);
    void cargarFormularioCancha(const FormCancha &f);
    void mostrarMensaje(const QString &texto, bool exito);

    // ---- Lo que el usuario ingresó ----
    int idSeleccionado(Tabla t) const;
    void seleccionarEnTabla(Tabla t, int id);   // -1 quita la selección
    int idCombo(Combo c) const;
    QString textoCombo(Combo c) const;
    FormReserva formReserva() const;
    FormEquipo formEquipo() const;
    FormCancha formCancha() const;
    FormTorneo formTorneo() const;
    int pedirEstrellas();                                         // 0 si cancela
    int elegirOpcion(const QString &pregunta, const QStringList &opciones);   // -1 si cancela
    int golesLocal() const { return spnGolesLocal->value(); }
    int golesVisitante() const { return spnGolesVisitante->value(); }
    bool confirmar(const QString &pregunta);

signals:
    void accionSolicitada(Accion accion);
    void accionEnFila(Accion accion, int id);
    void comboCambiado(Combo combo);
    void seleccionCambiada(Tabla tabla);
    void celdaElegida(Grilla grilla, int fila, int columna);
    void formularioCambiado();   // extras o cantidad de jugadores: hay que recalcular el precio

private:
    bool esDueno;
    QTabWidget *pestanas;
    QLabel *lblUsuario, *lblResumenPagos, *lblMedioPago, *lblCotizacion, *lblEstado;
    QLabel *lblHorario = nullptr, *lblDetalleCancha = nullptr;
    QMap<Tabla, QTableWidget *> tablas;
    QMap<Grilla, QTableWidget *> grillas;
    QMap<Grilla, QLabel *> titulosGrilla;
    QWidget *pestanaSolicitudes = nullptr, *pestanaNotificaciones = nullptr;
    QComboBox *cboModo = nullptr;
    QWidget *filaJugadores = nullptr, *filaEquipo = nullptr;
    QLabel *lblBuscamos = nullptr;
    QMap<Combo, QComboBox *> combos;
    QCheckBox *chkPelota, *chkChalecos, *chkArbitro;
    QSpinBox *spnSomos, *spnBuscamos, *spnGolesLocal, *spnGolesVisitante, *spnApertura, *spnCierre, *spnMaxEquipos;
    QDoubleSpinBox *spnPrecio, *spnCosto;
    QLineEdit *edtEquipo, *edtTitulares, *edtSuplentes, *edtNombreCancha, *edtDistrito, *edtDireccion, *edtTorneo;
    QPlainTextEdit *txtDetalleEquipo;
    QLabel *lblTorneo;

    QTableWidget *nuevaTabla(Tabla t, const QStringList &encabezados);
    QComboBox *nuevoCombo(Combo c);
    QPushButton *boton(const QString &texto, Accion accion);
    QWidget *nuevaGrilla(Grilla g);

    QWidget *crearPestanaCanchas();
    QWidget *crearPestanaMisReservas();
    QWidget *crearPestanaSolicitudes();
    QWidget *crearPestanaEquiposTorneos();
    QWidget *crearPestanaMisCanchas();
    QWidget *crearPestanaReservasRecibidas();
    QWidget *crearPestanaTorneosDueno();
    QWidget *crearPestanaPagos();
    QWidget *crearPestanaRanking();
    QWidget *crearPestanaNotificaciones();
};
