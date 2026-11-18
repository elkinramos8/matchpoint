// VentanaLogin.cpp
#include "VentanaLogin.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

VentanaLogin::VentanaLogin(QWidget *padre) : QWidget(padre) {
    setWindowTitle("MatchPoint");
    setMinimumWidth(420);

    auto *titulo = new QLabel("<h1 style='color:#1b7f3b'>⚽ MatchPoint</h1>"
                              "<p>Reserva y gestión de canchas deportivas</p>", this);
    titulo->setAlignment(Qt::AlignCenter);

    // --- Pestaña: iniciar sesión ---
    auto *ingreso = new QWidget(this);
    edtCorreo = new QLineEdit(ingreso);
    edtCorreo->setPlaceholderText("correo@ejemplo.com");
    edtClave = new QLineEdit(ingreso);
    edtClave->setEchoMode(QLineEdit::Password);
    auto *btnIngresar = new QPushButton("Iniciar sesión", ingreso);
    btnIngresar->setDefault(true);
    auto *f1 = new QFormLayout(ingreso);
    f1->addRow("Correo:", edtCorreo);
    f1->addRow("Contraseña:", edtClave);
    f1->addRow(btnIngresar);
    f1->addRow(new QLabel("<small>Demo: elkin@matchpoint.pe o rosa@matchpoint.pe · clave 1234</small>", ingreso));

    // --- Pestaña: crear cuenta ---
    auto *registro = new QWidget(this);
    cboRol = new QComboBox(registro);
    cboRol->addItems({"Jugador", "Dueño de cancha"});
    edtNombreReg = new QLineEdit(registro);
    edtCorreoReg = new QLineEdit(registro);
    edtClaveReg = new QLineEdit(registro);
    edtClaveReg->setEchoMode(QLineEdit::Password);
    cboPosicion = new QComboBox(registro);
    cboPosicion->addItems({"arquero", "defensa", "volante", "delantero"});
    edtDetalle = new QLineEdit(registro);   // nombre del negocio (solo dueños)
    edtDetalle->setPlaceholderText("Complejo deportivo ...");
    edtDetalle->hide();
    auto *detalle = new QWidget(registro);
    auto *ld = new QHBoxLayout(detalle);
    ld->setContentsMargins(0, 0, 0, 0);
    ld->addWidget(cboPosicion);
    ld->addWidget(edtDetalle);
    lblDetalle = new QLabel("Posición:", registro);
    edtTarjeta = new QLineEdit(registro);
    edtTarjeta->setMaxLength(4);
    edtTarjeta->setPlaceholderText("4 últimos dígitos (pago simulado)");
    lblTarjeta = new QLabel("Tarjeta:", registro);
    auto *btnRegistrar = new QPushButton("Crear cuenta", registro);
    auto *f2 = new QFormLayout(registro);
    f2->addRow("Soy:", cboRol);
    f2->addRow("Nombre completo:", edtNombreReg);
    f2->addRow("Correo:", edtCorreoReg);
    f2->addRow("Contraseña:", edtClaveReg);
    f2->addRow(lblDetalle, detalle);
    f2->addRow(lblTarjeta, edtTarjeta);
    f2->addRow(btnRegistrar);

    pestanas = new QTabWidget(this);
    pestanas->addTab(ingreso, "Iniciar sesión");
    pestanas->addTab(registro, "Crear cuenta");

    lblError = new QLabel(this);
    lblError->setStyleSheet("color:#b3261e");
    lblError->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titulo);
    layout->addWidget(pestanas);
    layout->addWidget(lblError);

    // La Vista solo traduce los eventos de sus widgets a sus propias señales
    connect(cboRol, &QComboBox::currentTextChanged, this, [this](const QString &rol) {
        bool jugador = rol == "Jugador";
        lblDetalle->setText(jugador ? "Posición:" : "Nombre del negocio:");
        cboPosicion->setVisible(jugador);
        edtDetalle->setVisible(!jugador);
        lblTarjeta->setVisible(jugador);   // el dueño no paga: solo recibe
        edtTarjeta->setVisible(jugador);
    });
    auto ingresar = [this] { emit solicitarIngreso(edtCorreo->text().trimmed(), edtClave->text()); };
    connect(btnIngresar, &QPushButton::clicked, this, ingresar);
    connect(edtClave, &QLineEdit::returnPressed, this, ingresar);
    connect(btnRegistrar, &QPushButton::clicked, this, [this] {
        bool jugador = cboRol->currentText() == "Jugador";
        emit solicitarRegistro(cboRol->currentText(), edtNombreReg->text().trimmed(),
                               edtCorreoReg->text().trimmed(), edtClaveReg->text(),
                               jugador ? cboPosicion->currentText() : edtDetalle->text().trimmed(),
                               edtTarjeta->text().trimmed());
    });
}

void VentanaLogin::mostrarError(const QString &mensaje) { lblError->setText(mensaje); }

void VentanaLogin::limpiar() {
    for (auto *e : {edtClave, edtNombreReg, edtCorreoReg, edtClaveReg, edtDetalle, edtTarjeta}) e->clear();
    lblError->clear();
    pestanas->setCurrentIndex(0);
}
