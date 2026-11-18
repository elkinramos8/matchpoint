// main.cpp
// Arma el MVC: crea el Modelo, carga los datos guardados y conecta
// las ventanas (Vista) con sus Controladores.
#include "controlador/ControladorPrincipal.h"
#include "controlador/ControladorSesion.h"
#include "persistencia/BaseDatos.h"
#include "persistencia/DatosDemo.h"
#include "persistencia/GuardadoAsincrono.h"
#include "persistencia/Persistencia.h"
#include <QApplication>
#include <QDir>
#include <QPointer>
#include <QStandardPaths>
#include <QTimer>
#include <iostream>
#include <memory>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("MatchPoint");

    // ---------- MODELO + datos ----------
    QString carpeta = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(carpeta);
    QString ruta = carpeta + "/matchpoint.db";
    BaseDatos baseDatos(ruta);   // SQLite

    auto modeloPtr = std::make_unique<ModeloMatchPoint>();
    try {
        auto registros = baseDatos.leer();
        if (!registros.empty()) Persistencia::cargarRegistros(*modeloPtr, registros);
        else cargarDatosDemo(*modeloPtr);   // primera vez: datos de ejemplo
    } catch (const std::exception &e) {
        std::cerr << "No se pudo leer la base (" << e.what() << "). Se usan datos de ejemplo." << std::endl;
        modeloPtr = std::make_unique<ModeloMatchPoint>();
        cargarDatosDemo(*modeloPtr);
    }
    ModeloMatchPoint &modelo = *modeloPtr;
    std::cout << "Base de datos: " << ruta.toStdString() << std::endl;

    // MULTITHREADING: cada cambio del modelo se guarda en SQLite desde un hilo aparte (productor-consumidor)
    GuardadoAsincrono guardado([&baseDatos](const std::string &foto) {
        return baseDatos.guardar(Persistencia::aRegistros(foto));
    });
    guardado.encolar(Persistencia::serializar(modelo));
    modelo.suscribir([&](const Evento &) { guardado.encolar(Persistencia::serializar(modelo)); });

    // Partidos que no se completan: se revisan al abrir y luego cada 30 segundos.
    // Se usa un QTimer (en el hilo de la interfaz) porque la ventana lee el modelo sin bloquearlo:
    // así la revisión nunca corre al mismo tiempo que un refresco de pantalla.
    modelo.revisarConvocatorias();
    QTimer revision;
    QObject::connect(&revision, &QTimer::timeout, [&modelo] { modelo.revisarConvocatorias(); });
    revision.start(30 * 1000);

    // ---------- VISTA + CONTROLADOR ----------
    VentanaLogin login;
    ControladorSesion sesion(&login, &modelo);
    QPointer<VentanaPrincipal> ventana;

    QObject::connect(&sesion, &ControladorSesion::sesionIniciada, [&](int usuarioId) {
        bool esDueno = modelo.buscarUsuario(usuarioId)->puedeAdministrarCanchas();
        ventana = new VentanaPrincipal(esDueno);
        ventana->setAttribute(Qt::WA_DeleteOnClose);
        auto *controlador = new ControladorPrincipal(ventana, &modelo, usuarioId, ventana);  // muere con la ventana
        QObject::connect(controlador, &ControladorPrincipal::sesionCerrada, [&] {
            login.show();
            if (ventana) ventana->close();
        });
        ventana->show();
        login.hide();
    });

    login.show();
    int codigo = app.exec();
    guardado.detener();   // espera a que el hilo termine de escribir
    return codigo;
}
