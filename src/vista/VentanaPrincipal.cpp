// VentanaPrincipal.cpp
#include "VentanaPrincipal.h"
#include "Tablas.h"
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QVBoxLayout>

VentanaPrincipal::VentanaPrincipal(bool esDueno, QWidget *padre) : QMainWindow(padre), esDueno(esDueno) {
    setWindowTitle("MatchPoint");
    resize(1280, 800);

    // Encabezado: usuario, medio de pago o ingresos y botón de salida
    auto *central = new QWidget(this);
    auto *cabecera = new QHBoxLayout();
    lblUsuario = new QLabel(central);
    lblUsuario->setTextFormat(Qt::RichText);
    cabecera->addWidget(new QLabel("<h2 style='color:#1b7f3b;margin:0'>⚽ MatchPoint</h2>", central));
    cabecera->addStretch();
    cabecera->addWidget(lblUsuario);
    cabecera->addWidget(boton("Cerrar sesión", Accion::CERRAR_SESION));

    pestanas = new QTabWidget(central);
    if (esDueno) {
        pestanas->addTab(crearPestanaMisCanchas(), "Mis canchas");
        pestanas->addTab(crearPestanaReservasRecibidas(), "Reservas recibidas");
        pestanas->addTab(crearPestanaTorneosDueno(), "Torneos");
    } else {
        pestanas->addTab(crearPestanaCanchas(), "Canchas y reservar");
        pestanas->addTab(crearPestanaMisReservas(), "Mis reservas");
        pestanaSolicitudes = crearPestanaSolicitudes();
        pestanas->addTab(pestanaSolicitudes, "Solicitudes");
        pestanas->addTab(crearPestanaEquiposTorneos(), "Equipos y torneos");
    }
    pestanas->addTab(crearPestanaPagos(), esDueno ? "Ingresos" : "Mis pagos");
    pestanas->addTab(crearPestanaRanking(), "Ranking");
    pestanaNotificaciones = crearPestanaNotificaciones();
    pestanas->addTab(pestanaNotificaciones, "Notificaciones");

    auto *layout = new QVBoxLayout(central);
    layout->addLayout(cabecera);
    layout->addWidget(pestanas);
    setCentralWidget(central);

    lblEstado = new QLabel(this);
    statusBar()->addWidget(lblEstado, 1);
}

// ===================== Ayudantes para construir la interfaz =====================

QTableWidget *VentanaPrincipal::nuevaTabla(Tabla t, const QStringList &encabezados) {
    auto *tabla = Tablas::crear(encabezados, this);
    tablas[t] = tabla;
    if (encabezados.last() == "Acciones") tabla->verticalHeader()->setDefaultSectionSize(34);   // caben los botones
    connect(tabla, &QTableWidget::itemSelectionChanged, this, [this, t] { emit seleccionCambiada(t); });
    return tabla;
}

QComboBox *VentanaPrincipal::nuevoCombo(Combo c) {
    auto *combo = new QComboBox(this);
    combos[c] = combo;
    connect(combo, &QComboBox::currentIndexChanged, this, [this, c] { emit comboCambiado(c); });
    return combo;
}

QPushButton *VentanaPrincipal::boton(const QString &texto, Accion accion) {
    auto *b = new QPushButton(texto, this);
    connect(b, &QPushButton::clicked, this, [this, accion] { emit accionSolicitada(accion); });
    return b;
}

// Grilla semanal con su título y la leyenda de colores
QWidget *VentanaPrincipal::nuevaGrilla(Grilla g) {
    auto *w = new QWidget(this);
    auto *titulo = new QLabel(w);
    titulo->setTextFormat(Qt::RichText);
    titulosGrilla[g] = titulo;
    auto *grilla = Tablas::crearGrilla(w);
    grillas[g] = grilla;
    connect(grilla, &QTableWidget::cellClicked, this,
            [this, g](int fila, int columna) { emit celdaElegida(g, fila, columna); });
    auto *leyenda = new QLabel(Tablas::leyendaGrilla(g == Grilla::RESERVAR), w);
    leyenda->setTextFormat(Qt::RichText);
    auto *l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(titulo);
    l->addWidget(grilla, 1);
    l->addWidget(leyenda);
    return w;
}

// Recuadro con la explicación de cada sección
static QLabel *ayuda(QWidget *padre, const QString &texto) {
    auto *l = new QLabel(texto, padre);
    l->setWordWrap(true);
    l->setTextFormat(Qt::RichText);
    l->setStyleSheet("background:#eef6ef;border:1px solid #c8e6c9;border-radius:4px;padding:6px;color:#1f3d27");
    return l;
}

static QWidget *columna(QWidget *padre, std::initializer_list<QWidget *> widgets) {
    auto *w = new QWidget(padre);
    auto *l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    for (auto *x : widgets) l->addWidget(x);
    return w;
}

// ===================== Pestañas del jugador =====================

QWidget *VentanaPrincipal::crearPestanaCanchas() {
    auto *w = new QWidget(this);
    auto *filtro = new QHBoxLayout();
    filtro->addWidget(new QLabel("Distrito:", w));
    filtro->addWidget(nuevoCombo(Combo::DISTRITO), 1);

    auto *tabla = nuevaTabla(Tabla::CANCHAS, {"Cancha", "Distrito", "Precio / h", "Calificación", "Abiertos"});
    tabla->setMinimumHeight(120);
    lblDetalleCancha = new QLabel("Elige una cancha para ver sus horarios", w);
    lblDetalleCancha->setWordWrap(true);

    // Formulario: el horario se elige haciendo clic en la grilla
    auto *grupo = new QGroupBox("Reservar el horario elegido", w);
    lblHorario = new QLabel(grupo);
    lblHorario->setStyleSheet("font-weight:bold");
    chkPelota = new QCheckBox("Pelota", grupo);
    chkChalecos = new QCheckBox("Chalecos", grupo);
    chkArbitro = new QCheckBox("Árbitro", grupo);
    // Modo de la reserva: privada, buscar jugadores o buscar un equipo rival
    cboModo = new QComboBox(grupo);
    cboModo->addItems({"Privada (vamos completos)", "Buscar jugadores", "Buscar equipo rival"});
    spnSomos = new QSpinBox(grupo);
    spnSomos->setRange(1, 14);
    spnSomos->setValue(10);
    spnBuscamos = new QSpinBox(grupo);
    spnBuscamos->setRange(1, 13);
    spnBuscamos->setValue(4);
    lblBuscamos = new QLabel("y buscamos", grupo);
    filaJugadores = new QWidget(grupo);
    auto *jugadores = new QHBoxLayout(filaJugadores);
    jugadores->setContentsMargins(0, 0, 0, 0);
    jugadores->addWidget(new QLabel("Somos", grupo));
    jugadores->addWidget(spnSomos);
    jugadores->addWidget(lblBuscamos);
    jugadores->addWidget(spnBuscamos);
    jugadores->addStretch();
    filaEquipo = new QWidget(grupo);
    auto *equipo = new QHBoxLayout(filaEquipo);
    equipo->setContentsMargins(0, 0, 0, 0);
    equipo->addWidget(new QLabel("Con mi equipo:", grupo));
    equipo->addWidget(nuevoCombo(Combo::EQUIPO_RIVAL), 1);
    // Cada modo muestra solo los datos que necesita (esto es presentación: no toca el Modelo)
    auto actualizarModo = [this](int modo) {
        filaJugadores->setVisible(modo != 2);
        lblBuscamos->setVisible(modo == 1);
        spnBuscamos->setVisible(modo == 1);
        filaEquipo->setVisible(modo == 2);
        emit formularioCambiado();
    };
    connect(cboModo, &QComboBox::currentIndexChanged, this, actualizarModo);
    actualizarModo(0);
    lblCotizacion = new QLabel(grupo);   // varias líneas fijas (sin ajuste automático para que no se corte)
    lblCotizacion->setStyleSheet("font-weight:bold;color:#1b7f3b");
    lblCotizacion->setMinimumHeight(lblCotizacion->fontMetrics().lineSpacing() * 3 + 6);
    auto *extras = new QHBoxLayout();
    extras->addWidget(chkPelota);
    extras->addWidget(chkChalecos);
    extras->addWidget(chkArbitro);
    extras->addStretch();
    auto *form = new QFormLayout(grupo);
    form->addRow("Horario:", lblHorario);
    form->addRow("Extras:", extras);
    form->addRow("Modo:", cboModo);
    form->addRow("", filaJugadores);
    form->addRow("", filaEquipo);
    form->addRow(lblCotizacion);
    form->addRow(boton("Reservar y pagar con mi tarjeta", Accion::RESERVAR));
    grupo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);   // el formulario nunca se aplasta
    for (auto *chk : {chkPelota, chkChalecos, chkArbitro})
        connect(chk, &QCheckBox::toggled, this, [this] { emit formularioCambiado(); });
    for (auto *spn : {spnSomos, spnBuscamos})
        connect(spn, &QSpinBox::valueChanged, this, [this] { emit formularioCambiado(); });
    connect(combos[Combo::EQUIPO_RIVAL], &QComboBox::currentIndexChanged, this, [this] { emit formularioCambiado(); });

    auto *izquierda = new QVBoxLayout();
    izquierda->addLayout(filtro);
    izquierda->addWidget(tabla, 1);
    izquierda->addWidget(lblDetalleCancha);
    izquierda->addWidget(grupo);
    auto *cuerpo = new QHBoxLayout();
    cuerpo->addLayout(izquierda, 2);
    cuerpo->addWidget(nuevaGrilla(Grilla::RESERVAR), 3);
    auto *l = new QVBoxLayout(w);
    l->addWidget(ayuda(w, "<b>Reservar una cancha:</b> 1) elige la cancha en la lista · 2) haz clic en un horario "
                          "<b>verde</b> de la grilla · 3) elige el modo y paga. Clic en un horario <b>amarillo</b> para "
                          "pedir unirte a un partido que busca jugadores, o <b>naranja</b> para retar con tu equipo a uno que busca rival."));
    l->addLayout(cuerpo, 1);
    return w;
}

QWidget *VentanaPrincipal::crearPestanaMisReservas() {
    auto *w = new QWidget(this);
    auto *l = new QVBoxLayout(w);
    l->addWidget(ayuda(w, "<b>Mis reservas:</b> partidos que organizaste o en los que ya te aceptaron. "
                          "Cada fila tiene sus acciones: <b>Cancelar</b> o <b>Jugar así</b> (organizador), "
                          "<b>Retirarme</b> (participante) y <b>Calificar</b> cuando ya se jugó. Si buscas jugadores, "
                          "tienes hasta <b>24 h antes</b> para completar el partido: si no, se cancela y se devuelve todo."));
    l->addWidget(nuevaTabla(Tabla::MIS_RESERVAS, {"Cancha", "Fecha", "Detalle", "Total", "Tu parte",
                                                 "Jugadores", "Estado", "Tu rol", "Acciones"}));
    return w;
}

static QLabel *titulo(QWidget *padre, const QString &texto) {
    auto *l = new QLabel("<h3 style='margin:4px 0'>" + texto + "</h3>", padre);
    l->setTextFormat(Qt::RichText);
    return l;
}

QWidget *VentanaPrincipal::crearPestanaSolicitudes() {
    auto *w = new QWidget(this);
    auto *l = new QVBoxLayout(w);
    l->addWidget(ayuda(w, "<b>¿Cómo funcionan?</b> Alguien pide entrar a un partido (como jugador o con su equipo como "
                          "rival) → el organizador acepta o rechaza → si acepta, paga su parte y ocupa el cupo; si "
                          "rechaza, no se cobra nada. Si el partido ya está completo, la solicitud queda en <b>lista de "
                          "espera</b>: si alguien se retira, el primero de la lista puede entrar."));
    l->addWidget(titulo(w, "📥 Recibidas: quieren entrar a TUS partidos"));
    l->addWidget(nuevaTabla(Tabla::SOLICITUDES, {"Tu partido", "Quién", "Posición / equipo", "Reputación",
                                                "Te pagaría", "Estado", "Acciones"}), 1);
    l->addWidget(titulo(w, "📤 Enviadas: TÚ pediste entrar a estos partidos"));
    l->addWidget(nuevaTabla(Tabla::SOLICITUDES_ENVIADAS, {"Partido", "Organizador", "Jugadores", "Pagarías", "Estado"}), 1);
    return w;
}

QWidget *VentanaPrincipal::crearPestanaEquiposTorneos() {
    auto *w = new QWidget(this);

    // Izquierda: equipos (Composite)
    auto *gEquipos = new QGroupBox("Mis equipos", w);
    edtEquipo = new QLineEdit(gEquipos);
    edtTitulares = new QLineEdit(gEquipos);
    edtTitulares->setPlaceholderText("Nombres separados por comas (tú ya eres titular)");
    edtSuplentes = new QLineEdit(gEquipos);
    edtSuplentes->setPlaceholderText("Opcional");
    txtDetalleEquipo = new QPlainTextEdit(gEquipos);
    txtDetalleEquipo->setReadOnly(true);
    txtDetalleEquipo->setMaximumHeight(120);
    auto *fe = new QFormLayout();
    fe->addRow("Nombre:", edtEquipo);
    fe->addRow("Titulares:", edtTitulares);
    fe->addRow("Suplentes:", edtSuplentes);
    fe->addRow(boton("Crear equipo", Accion::CREAR_EQUIPO));
    auto *le = new QVBoxLayout(gEquipos);
    le->addWidget(nuevaTabla(Tabla::MIS_EQUIPOS, {"Equipo", "Capitán", "Jugadores"}));
    le->addWidget(txtDetalleEquipo);
    le->addLayout(fe);

    // Derecha: torneos
    auto *gTorneos = new QGroupBox("Torneos", w);
    lblTorneo = new QLabel(gTorneos);
    auto *inscribir = new QHBoxLayout();
    inscribir->addWidget(new QLabel("Equipo que inscribes:", gTorneos));
    inscribir->addWidget(nuevoCombo(Combo::EQUIPO_INSCRIBIR), 1);
    auto *lt = new QVBoxLayout(gTorneos);
    lt->addLayout(inscribir);
    lt->addWidget(nuevaTabla(Tabla::TORNEOS, {"Torneo", "Cancha", "Inscripción", "Equipos", "Estado", "Acciones"}));
    lt->addWidget(lblTorneo);
    lt->addWidget(nuevaTabla(Tabla::FIXTURE, {"#", "Local", "Visitante", "Resultado"}));
    lt->addWidget(nuevaTabla(Tabla::POSICIONES, {"Pos", "Equipo", "PJ", "PG", "PE", "PP", "GF", "GC", "DG", "Pts"}));

    auto *cuerpo = new QHBoxLayout();
    cuerpo->addWidget(gEquipos, 2);
    cuerpo->addWidget(gTorneos, 3);
    auto *l = new QVBoxLayout(w);
    l->addWidget(ayuda(w, "<b>Equipos y torneos:</b> arma tu equipo con titulares y suplentes (mínimo 5 jugadores). "
                          "Como capitán, elige un torneo con inscripciones abiertas e inscribe a tu equipo pagando la inscripción."));
    l->addLayout(cuerpo, 1);
    return w;
}

// ===================== Pestañas del dueño =====================

QWidget *VentanaPrincipal::crearPestanaMisCanchas() {
    auto *w = new QWidget(this);
    auto *grupo = new QGroupBox("Datos de la cancha", w);
    edtNombreCancha = new QLineEdit(grupo);
    edtDistrito = new QLineEdit(grupo);
    edtDireccion = new QLineEdit(grupo);
    spnPrecio = new QDoubleSpinBox(grupo);
    spnPrecio->setRange(1, 1000);
    spnPrecio->setPrefix("S/ ");
    spnPrecio->setValue(60);
    spnApertura = new QSpinBox(grupo);
    spnApertura->setRange(0, 23);
    spnApertura->setValue(8);
    spnCierre = new QSpinBox(grupo);
    spnCierre->setRange(1, 24);
    spnCierre->setValue(23);
    auto *horario = new QHBoxLayout();
    horario->addWidget(spnApertura);
    horario->addWidget(new QLabel("a", grupo));
    horario->addWidget(spnCierre);
    horario->addStretch();
    auto *botones = new QHBoxLayout();
    botones->addWidget(boton("Registrar nueva cancha", Accion::REGISTRAR_CANCHA));
    botones->addWidget(boton("Guardar cambios de la seleccionada", Accion::ACTUALIZAR_CANCHA));
    auto *f = new QFormLayout(grupo);
    f->addRow("Nombre:", edtNombreCancha);
    f->addRow("Distrito:", edtDistrito);
    f->addRow("Dirección:", edtDireccion);
    f->addRow("Precio por hora:", spnPrecio);
    f->addRow("Atiende de (h):", horario);
    f->addRow(botones);

    auto *izquierda = new QVBoxLayout();
    izquierda->addWidget(nuevaTabla(Tabla::MIS_CANCHAS, {"Cancha", "Distrito", "Dirección", "Precio / h", "Horario",
                                                        "Calificación"}), 1);
    izquierda->addWidget(grupo);
    auto *cuerpo = new QHBoxLayout();
    cuerpo->addLayout(izquierda, 2);
    cuerpo->addWidget(nuevaGrilla(Grilla::MIS_CANCHAS), 3);
    auto *l = new QVBoxLayout(w);
    l->addWidget(ayuda(w, "<b>Mis canchas:</b> registra o edita tus canchas. A la derecha ves la ocupación de la semana "
                          "de la cancha seleccionada (clic en una casilla para ver quién reservó)."));
    l->addLayout(cuerpo, 1);
    return w;
}

QWidget *VentanaPrincipal::crearPestanaReservasRecibidas() {
    auto *w = new QWidget(this);
    auto *l = new QVBoxLayout(w);
    l->addWidget(ayuda(w, "<b>Reservas recibidas:</b> todas las reservas hechas en tus canchas. Después del partido "
                          "califica a los jugadores con un botón (es la reputación que ven los demás): "
                          "<b>Todo bien</b> = 5 ★, <b>Llegaron tarde</b> = 3 ★, <b>No vinieron</b> = 0 ★ (se habilita a los "
                          "25 minutos de tolerancia y no hay devolución). En partidos equipo contra equipo se califica a todos."));
    l->addWidget(nuevaTabla(Tabla::RESERVAS_RECIBIDAS, {"Cancha", "Fecha", "Organizador", "Jugadores", "Total",
                                                       "Estado", "Acciones"}));
    return w;
}

QWidget *VentanaPrincipal::crearPestanaTorneosDueno() {
    auto *w = new QWidget(this);
    auto *crear = new QGroupBox("Organizar un torneo", w);
    edtTorneo = new QLineEdit(crear);
    spnCosto = new QDoubleSpinBox(crear);
    spnCosto->setRange(0, 1000);
    spnCosto->setPrefix("S/ ");
    spnCosto->setValue(40);
    spnMaxEquipos = new QSpinBox(crear);
    spnMaxEquipos->setRange(2, 8);
    spnMaxEquipos->setValue(4);
    auto *fc = new QFormLayout(crear);
    fc->addRow("Cancha:", nuevoCombo(Combo::CANCHA_TORNEO));
    fc->addRow("Nombre:", edtTorneo);
    fc->addRow("Inscripción por equipo:", spnCosto);
    fc->addRow("Máximo de equipos:", spnMaxEquipos);
    fc->addRow(boton("Crear torneo", Accion::CREAR_TORNEO));

    spnGolesLocal = new QSpinBox(w);
    spnGolesVisitante = new QSpinBox(w);
    lblTorneo = new QLabel(w);
    auto *resultado = new QHBoxLayout();
    resultado->addWidget(new QLabel("Goles local:", w));
    resultado->addWidget(spnGolesLocal);
    resultado->addWidget(new QLabel("Goles visitante:", w));
    resultado->addWidget(spnGolesVisitante);
    resultado->addWidget(new QLabel("→ luego pulsa <b>Registrar</b> en el partido", w));
    resultado->addStretch();

    auto *izquierda = new QVBoxLayout();
    izquierda->addWidget(crear);
    izquierda->addWidget(nuevaTabla(Tabla::TORNEOS, {"Torneo", "Cancha", "Inscripción", "Equipos", "Estado", "Acciones"}));
    auto *derecha = new QVBoxLayout();
    derecha->addWidget(lblTorneo);
    derecha->addLayout(resultado);
    derecha->addWidget(nuevaTabla(Tabla::FIXTURE, {"#", "Local", "Visitante", "Resultado", "Acciones"}));
    derecha->addWidget(nuevaTabla(Tabla::POSICIONES, {"Pos", "Equipo", "PJ", "PG", "PE", "PP", "GF", "GC", "DG", "Pts"}));
    auto *cuerpo = new QHBoxLayout();
    cuerpo->addLayout(izquierda, 2);
    cuerpo->addLayout(derecha, 3);
    auto *l = new QVBoxLayout(w);
    l->addWidget(ayuda(w, "<b>Torneos:</b> crea un torneo, espera que los capitanes inscriban a sus equipos y pulsa "
                          "<b>Iniciar</b>: se genera el fixture todos contra todos. Pon los goles y pulsa <b>Registrar</b> en "
                          "cada partido; la tabla se actualiza sola."));
    l->addLayout(cuerpo, 1);
    return w;
}

// ===================== Pestañas comunes =====================

QWidget *VentanaPrincipal::crearPestanaPagos() {
    auto *w = new QWidget(this);
    lblResumenPagos = new QLabel(w);
    lblMedioPago = new QLabel(w);
    auto *l = new QVBoxLayout(w);
    l->addWidget(lblResumenPagos);
    auto *medio = new QHBoxLayout();
    medio->addWidget(lblMedioPago);
    medio->addStretch();
    l->addLayout(medio);
    l->addWidget(nuevaTabla(Tabla::MOVIMIENTOS, {"Fecha", "Comprobante", "Concepto", "Monto"}));
    l->addWidget(new QLabel(esDueno
        ? "<small>Los pagos de las reservas y torneos llegan a tu cuenta al instante, con su comprobante.</small>"
        : "<small>Pago simulado: no se cobra dinero real. Una tarjeta terminada en 0000 simula un pago "
          "rechazado por el banco.</small>", w));
    return w;
}

QWidget *VentanaPrincipal::crearPestanaRanking() {
    auto *w = new QWidget(this);
    auto *cuerpo = new QHBoxLayout();
    auto *externo = new QVBoxLayout(w);
    externo->addWidget(ayuda(w, "<b>Ranking:</b> reputación en los dos sentidos. Los jugadores califican las canchas y "
                                "el dueño califica a los jugadores: todo bien 5 ★, llegaron tarde 3 ★, no vinieron 0 ★. "
                                "El sistema pone 2 ★ por retirarse tarde y 1 ★ por cancelar tarde."));
    externo->addLayout(cuerpo, 1);
    auto *l = cuerpo;
    l->addWidget(columna(w, {new QLabel("<b>Mejores canchas</b>", w),
                             nuevaTabla(Tabla::RANKING_CANCHAS, {"#", "Cancha", "Distrito", "Calificación", "Votos"})}));
    l->addWidget(columna(w, {new QLabel("<b>Jugadores mejor calificados</b>", w),
                             nuevaTabla(Tabla::RANKING_JUGADORES, {"#", "Jugador", "Posición", "Reputación", "Calificaciones"})}));
    return w;
}

QWidget *VentanaPrincipal::crearPestanaNotificaciones() {
    auto *w = new QWidget(this);
    auto *l = new QVBoxLayout(w);
    l->addWidget(nuevaTabla(Tabla::NOTIFICACIONES, {"Cuándo", "Tipo", "Mensaje"}));
    return w;
}

// ===================== Órdenes del Controlador =====================

void VentanaPrincipal::mostrarEncabezado(const QString &nombre, const QString &detalle) {
    lblUsuario->setText(QString("<b>%1</b> · %2").arg(nombre, detalle));
}

void VentanaPrincipal::mostrarGrilla(Grilla g, const QString &titulo, const QStringList &dias,
                                     const QStringList &horas, const QList<QList<CeldaGrilla>> &celdas) {
    if (!grillas.contains(g)) return;
    titulosGrilla[g]->setText(titulo);
    Tablas::llenarGrilla(grillas[g], dias, horas, celdas);
}

void VentanaPrincipal::mostrarHorarioElegido(const QString &texto) {
    if (lblHorario) lblHorario->setText(texto);
}

void VentanaPrincipal::mostrarDetalleCancha(const QString &texto) {
    if (lblDetalleCancha) lblDetalleCancha->setText(texto);
}

void VentanaPrincipal::mostrarResumenPagos(const QString &resumen, const QString &medioDePago) {
    lblResumenPagos->setText("<h2>" + resumen + "</h2>");
    lblMedioPago->setText(medioDePago);
}

void VentanaPrincipal::mostrarContadorSolicitudes(int cantidad) {
    if (!pestanaSolicitudes) return;
    int i = pestanas->indexOf(pestanaSolicitudes);
    pestanas->setTabText(i, cantidad == 0 ? QString("Solicitudes") : QString("Solicitudes (%1 por responder)").arg(cantidad));
}

void VentanaPrincipal::mostrarTabla(Tabla t, const QList<QStringList> &filas, const QList<int> &ids,
                                    const QList<QList<BotonFila>> &botones) {
    if (!tablas.contains(t)) return;
    QTableWidget *tabla = tablas[t];
    Tablas::llenar(tabla, filas, ids);
    // Botones de cada fila en la última columna ("Acciones")
    for (int i = 0; i < botones.size() && i < filas.size(); ++i) {
        if (botones[i].isEmpty()) continue;
        int id = i < ids.size() ? ids[i] : i;
        auto *celda = new QWidget(tabla);
        auto *l = new QHBoxLayout(celda);
        l->setContentsMargins(2, 0, 2, 0);
        l->setSpacing(4);
        for (const BotonFila &b : botones[i]) {
            auto *btn = new QPushButton(b.texto, celda);
            Accion accion = b.accion;
            // Se emite "en diferido": la acción refresca la tabla y destruye este mismo botón
            connect(btn, &QPushButton::clicked, this, [this, accion, id] {
                QMetaObject::invokeMethod(this, [this, accion, id] { emit accionEnFila(accion, id); },
                                          Qt::QueuedConnection);
            });
            l->addWidget(btn);
        }
        l->addStretch();
        tabla->setCellWidget(i, tabla->columnCount() - 1, celda);
    }
}

void VentanaPrincipal::llenarCombo(Combo c, const QStringList &textos, const QList<int> &ids) {
    if (combos.contains(c)) Tablas::llenarCombo(combos[c], textos, ids);
}

void VentanaPrincipal::seleccionarEnCombo(Combo c, int id) {
    if (!combos.contains(c)) return;
    int idx = combos[c]->findData(id);
    if (idx >= 0) combos[c]->setCurrentIndex(idx);
}

void VentanaPrincipal::mostrarCotizacion(const QString &texto) { lblCotizacion->setText(texto); }
void VentanaPrincipal::mostrarDetalleTorneo(const QString &texto) { lblTorneo->setText(texto); }
void VentanaPrincipal::mostrarDetalleEquipo(const QString &texto) { txtDetalleEquipo->setPlainText(texto); }

void VentanaPrincipal::mostrarContadorNotificaciones(int deHoy) {
    int i = pestanas->indexOf(pestanaNotificaciones);
    pestanas->setTabText(i, deHoy > 0 ? QString("Notificaciones (%1 hoy)").arg(deHoy) : QString("Notificaciones"));
}

void VentanaPrincipal::cargarFormularioCancha(const FormCancha &f) {
    edtNombreCancha->setText(f.nombre);
    edtDistrito->setText(f.distrito);
    edtDireccion->setText(f.direccion);
    spnPrecio->setValue(f.precio);
    spnApertura->setValue(f.apertura);
    spnCierre->setValue(f.cierre);
}

void VentanaPrincipal::mostrarMensaje(const QString &texto, bool exito) {
    lblEstado->setStyleSheet(exito ? "color:#1b7f3b" : "color:#b3261e");
    lblEstado->setText(texto);
    if (!exito) QMessageBox::warning(this, "MatchPoint", texto);
}

int VentanaPrincipal::pedirEstrellas() {
    bool ok = false;
    int n = QInputDialog::getInt(this, "Calificar cancha", "¿Cuántas estrellas le das? (1 a 5)", 5, 1, 5, 1, &ok);
    return ok ? n : 0;
}

int VentanaPrincipal::elegirOpcion(const QString &pregunta, const QStringList &opciones) {
    bool ok = false;
    QString elegido = QInputDialog::getItem(this, "MatchPoint", pregunta, opciones, 0, false, &ok);
    return ok ? static_cast<int>(opciones.indexOf(elegido)) : -1;
}

bool VentanaPrincipal::confirmar(const QString &pregunta) {
    return QMessageBox::question(this, "MatchPoint", pregunta) == QMessageBox::Yes;
}

// ===================== Lo que ingresó el usuario =====================

int VentanaPrincipal::idSeleccionado(Tabla t) const {
    return tablas.contains(t) ? Tablas::idSeleccionado(tablas[t]) : -1;
}

void VentanaPrincipal::seleccionarEnTabla(Tabla t, int id) {
    if (tablas.contains(t)) Tablas::seleccionar(tablas[t], id);
}

int VentanaPrincipal::idCombo(Combo c) const {
    if (!combos.contains(c) || combos[c]->currentIndex() < 0) return -1;
    return combos[c]->currentData().toInt();
}

QString VentanaPrincipal::textoCombo(Combo c) const {
    return combos.contains(c) ? combos[c]->currentText() : QString();
}

FormReserva VentanaPrincipal::formReserva() const {
    QStringList extras;
    if (chkPelota->isChecked()) extras << "pelota";
    if (chkChalecos->isChecked()) extras << "chalecos";
    if (chkArbitro->isChecked()) extras << "arbitro";
    auto modo = static_cast<ModoReserva>(cboModo->currentIndex());
    return {extras, modo, spnSomos->value(), modo == ModoReserva::BUSCAR_JUGADORES ? spnBuscamos->value() : 0,
            idCombo(Combo::EQUIPO_RIVAL)};
}

static QStringList listaNombres(const QString &texto) {
    QStringList r;
    for (const QString &s : texto.split(',', Qt::SkipEmptyParts))
        if (!s.trimmed().isEmpty()) r << s.trimmed();
    return r;
}

FormEquipo VentanaPrincipal::formEquipo() const {
    return {edtEquipo->text().trimmed(), listaNombres(edtTitulares->text()), listaNombres(edtSuplentes->text())};
}

FormCancha VentanaPrincipal::formCancha() const {
    return {edtNombreCancha->text().trimmed(), edtDistrito->text().trimmed(), edtDireccion->text().trimmed(),
            spnPrecio->value(), spnApertura->value(), spnCierre->value()};
}

FormTorneo VentanaPrincipal::formTorneo() const {
    return {idCombo(Combo::CANCHA_TORNEO), edtTorneo->text().trimmed(), spnCosto->value(), spnMaxEquipos->value()};
}
