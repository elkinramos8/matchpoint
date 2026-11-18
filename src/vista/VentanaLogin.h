// VentanaLogin.h
// VISTA: inicio de sesión y registro. Solo widgets y señales.
#pragma once
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QWidget>

class VentanaLogin : public QWidget {
    Q_OBJECT
public:
    explicit VentanaLogin(QWidget *padre = nullptr);

    void mostrarError(const QString &mensaje);
    void limpiar();

signals:
    void solicitarIngreso(const QString &correo, const QString &clave);
    void solicitarRegistro(const QString &rol, const QString &nombre, const QString &correo,
                           const QString &clave, const QString &detalle, const QString &tarjeta);

private:
    QTabWidget *pestanas;
    QLineEdit *edtCorreo, *edtClave;
    QComboBox *cboRol, *cboPosicion;
    QLineEdit *edtNombreReg, *edtCorreoReg, *edtClaveReg, *edtDetalle, *edtTarjeta;
    QLabel *lblDetalle, *lblTarjeta, *lblError;
};
