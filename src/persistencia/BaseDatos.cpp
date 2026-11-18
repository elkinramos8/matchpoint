// BaseDatos.cpp
#include "BaseDatos.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QThread>
#include <QVariant>
#include <iostream>

namespace {

// Tipo de registro -> tabla y columnas (en el mismo orden que los campos de Persistencia)
struct Tabla {
    std::string tipo;
    QString nombre;
    QStringList columnas;
};

const std::vector<Tabla> &tablas() {
    static const std::vector<Tabla> lista = {
        {"U", "usuarios", {"id", "rol", "nombre", "correo", "clave_hash", "detalle", "calificaciones", "saldo_centimos",
                           "tarjeta"}},
        {"M", "movimientos", {"usuario_id", "fecha", "monto_centimos", "concepto", "comprobante"}},
        {"C", "canchas", {"id", "dueno_id", "nombre", "distrito", "direccion", "precio_centimos", "apertura", "cierre",
                          "calificaciones"}},
        {"R", "reservas", {"id", "cancha_id", "organizador_id", "inicio", "grupo", "buscados", "precio_base_centimos",
                           "extras", "estado", "participantes", "calificaron", "solicitudes", "equipo_id",
                           "equipo_rival_id", "aviso_convocatoria"}},
        {"E", "equipos", {"id", "capitan_id", "nombre", "titulares", "suplentes"}},
        {"T", "torneos", {"id", "dueno_id", "cancha_id", "nombre", "costo_centimos", "max_equipos", "estado", "equipos"}},
        {"P", "partidos_torneo", {"torneo_id", "local_id", "visitante_id", "goles_local", "goles_visitante", "jugado"}},
        {"K", "comprobantes", {"ultimo"}},
        {"N", "notificaciones", {"usuario_id", "fecha", "mensaje", "tipo"}},
    };
    return lista;
}

} // namespace

QString BaseDatos::abrirConexion() {
    // Qt exige una conexión distinta por hilo
    QString nombre = "matchpoint_" + QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    if (!QSqlDatabase::contains(nombre)) {
        QSqlDatabase bd = QSqlDatabase::addDatabase("QSQLITE", nombre);
        bd.setDatabaseName(ruta);
        if (!bd.open()) std::cerr << "No se pudo abrir " << ruta.toStdString() << std::endl;
        QSqlQuery q(bd);
        for (const Tabla &t : tablas())   // todas las columnas como TEXT: guardan los campos tal cual
            q.exec("CREATE TABLE IF NOT EXISTS " + t.nombre + " (" + t.columnas.join(" TEXT, ") + " TEXT)");
    }
    return nombre;
}

bool BaseDatos::guardar(const std::vector<Persistencia::Registro> &registros) {
    QSqlDatabase bd = QSqlDatabase::database(abrirConexion());
    bd.transaction();   // todo o nada: nunca queda una base a medio escribir
    QSqlQuery q(bd);
    for (const Tabla &t : tablas()) q.exec("DELETE FROM " + t.nombre);
    for (const Tabla &t : tablas()) {
        QStringList marcas;
        for (int i = 0; i < t.columnas.size(); ++i) marcas << "?";
        q.prepare("INSERT INTO " + t.nombre + " (" + t.columnas.join(", ") + ") VALUES (" + marcas.join(", ") + ")");
        for (const auto &r : registros) {
            if (r.empty() || r[0] != t.tipo) continue;
            for (int i = 0; i < t.columnas.size(); ++i)
                q.addBindValue(i + 1 < static_cast<int>(r.size()) ? QString::fromStdString(r[i + 1]) : QString());
            if (!q.exec()) {
                std::cerr << "Error SQLite: " << q.lastError().text().toStdString() << std::endl;
                bd.rollback();
                return false;
            }
        }
    }
    return bd.commit();
}

std::vector<Persistencia::Registro> BaseDatos::leer() {
    std::vector<Persistencia::Registro> registros;
    QSqlDatabase bd = QSqlDatabase::database(abrirConexion());
    QSqlQuery q(bd);
    for (const Tabla &t : tablas()) {
        q.exec("SELECT " + t.columnas.join(", ") + " FROM " + t.nombre + " ORDER BY rowid");
        while (q.next()) {
            Persistencia::Registro r{t.tipo};
            for (int i = 0; i < t.columnas.size(); ++i) r.push_back(q.value(i).toString().toStdString());
            registros.push_back(r);
        }
    }
    return registros;
}
