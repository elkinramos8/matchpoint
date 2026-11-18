// GuardadoAsincrono.cpp
#include "GuardadoAsincrono.h"
#include <chrono>

GuardadoAsincrono::GuardadoAsincrono(std::function<bool(const std::string &)> escribir)
    : escribir(std::move(escribir)), hilo(&GuardadoAsincrono::consumidor, this) {}

GuardadoAsincrono::~GuardadoAsincrono() { detener(); }

void GuardadoAsincrono::encolar(std::string foto) {
    std::lock_guard<std::mutex> candado(mtx);
    cola.push(std::move(foto));
}

void GuardadoAsincrono::detener() {
    {
        std::lock_guard<std::mutex> candado(mtx);
        terminado = true;
    }
    if (hilo.joinable()) hilo.join();
}

void GuardadoAsincrono::consumidor() {
    while (true) {
        std::string foto;
        bool hayFoto = false;
        {
            std::lock_guard<std::mutex> candado(mtx);
            // Si se acumularon varias fotos, solo importa la más reciente
            while (!cola.empty()) {
                foto = std::move(cola.front());
                cola.pop();
                hayFoto = true;
            }
            if (!hayFoto && terminado) break;   // cola vacía y no habrá más datos
        }
        if (hayFoto) {
            escribir(foto);
            continue;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
