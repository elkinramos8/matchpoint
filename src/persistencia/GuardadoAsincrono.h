// GuardadoAsincrono.h
// MULTITHREADING (productor-consumidor, como en la Actividad 11):
//  - Productor: la ventana encola una "foto" del modelo cada vez que algo cambia.
//  - Consumidor: un hilo aparte escribe esa foto en la base de datos SQLite.
// Así la ventana nunca se congela esperando al disco.
#pragma once
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

class GuardadoAsincrono {
private:
    std::function<bool(const std::string &)> escribir;   // lambda que guarda la foto (en SQLite)
    std::queue<std::string> cola;   // cola compartida
    std::mutex mtx;                 // protege la cola
    bool terminado = false;         // avisa al consumidor que ya no habrá más datos
    std::thread hilo;

    void consumidor();

public:
    explicit GuardadoAsincrono(std::function<bool(const std::string &)> escribir);
    ~GuardadoAsincrono();           // espera a que se escriba lo pendiente

    GuardadoAsincrono(const GuardadoAsincrono &) = delete;
    GuardadoAsincrono &operator=(const GuardadoAsincrono &) = delete;

    void encolar(std::string foto); // productor
    void detener();
};
