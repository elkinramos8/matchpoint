// ControladorSesion.h
// CONTROLADOR del inicio de sesión: conecta VentanaLogin con el Modelo.
#pragma once
#include "../modelo/ModeloMatchPoint.h"
#include "../vista/VentanaLogin.h"
#include <QObject>

class ControladorSesion : public QObject {
    Q_OBJECT
public:
    ControladorSesion(VentanaLogin *vista, ModeloMatchPoint *modelo, QObject *padre = nullptr);

signals:
    void sesionIniciada(int usuarioId);

private slots:
    void onIngreso(const QString &correo, const QString &clave);
    void onRegistro(const QString &rol, const QString &nombre, const QString &correo,
                    const QString &clave, const QString &detalle, const QString &tarjeta);

private:
    VentanaLogin *vista;
    ModeloMatchPoint *modelo;
};
