// BaseDatos.h
// Guarda la "foto" del Modelo en una base de datos SQLite (con el módulo Qt SQL).
// Cada tipo de registro tiene su tabla con columnas con nombre (usuarios, canchas, reservas...).
// Cada hilo usa su propia conexión: la escritura la hace el hilo de GuardadoAsincrono.
#pragma once
#include "Persistencia.h"
#include <QString>
#include <string>
#include <vector>

class BaseDatos {
public:
    explicit BaseDatos(const QString &ruta) : ruta(ruta) {}

    bool guardar(const std::vector<Persistencia::Registro> &registros);   // reemplaza todo en una transacción
    std::vector<Persistencia::Registro> leer();                           // vacío si la base es nueva

private:
    QString ruta;
    QString abrirConexion();   // nombre de la conexión del hilo actual
};
