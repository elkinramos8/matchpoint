// ControladorSesion.cpp
#include "ControladorSesion.h"

ControladorSesion::ControladorSesion(VentanaLogin *vista, ModeloMatchPoint *modelo, QObject *padre)
    : QObject(padre), vista(vista), modelo(modelo) {
    connect(vista, &VentanaLogin::solicitarIngreso, this, &ControladorSesion::onIngreso);
    connect(vista, &VentanaLogin::solicitarRegistro, this, &ControladorSesion::onRegistro);
}

void ControladorSesion::onIngreso(const QString &correo, const QString &clave) {
    try {
        Usuario *u = modelo->iniciarSesion(correo.toStdString(), clave.toStdString());
        vista->limpiar();
        emit sesionIniciada(u->getId());
    } catch (const MatchPointError &e) {   // manejo de errores: el mensaje llega a la ventana
        vista->mostrarError(QString::fromStdString(e.what()));
    }
}

void ControladorSesion::onRegistro(const QString &rol, const QString &nombre, const QString &correo,
                                   const QString &clave, const QString &detalle, const QString &tarjeta) {
    try {
        Usuario *u = modelo->registrarUsuario(rol.toStdString(), nombre.toStdString(), correo.toStdString(),
                                              clave.toStdString(), detalle.toStdString(), tarjeta.toStdString());
        vista->limpiar();
        emit sesionIniciada(u->getId());
    } catch (const MatchPointError &e) {
        vista->mostrarError(QString::fromStdString(e.what()));
    }
}
