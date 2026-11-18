// Repositorio.h
// TEMPLATE: almacén genérico que sirve para usuarios, canchas, reservas,
// equipos y torneos. T solo necesita tener getId() y setId(int).
#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

template <typename T>
class Repositorio {
private:
    std::vector<std::unique_ptr<T>> elementos;
    int siguienteId = 1;

public:
    // Asigna un id nuevo y se queda con el objeto (dueño único)
    T *agregar(std::unique_ptr<T> elemento) {
        elemento->setId(siguienteId++);
        elementos.push_back(std::move(elemento));
        return elementos.back().get();
    }

    // Para datos cargados de archivo: respeta el id guardado
    T *restaurar(std::unique_ptr<T> elemento) {
        siguienteId = std::max(siguienteId, elemento->getId() + 1);
        elementos.push_back(std::move(elemento));
        return elementos.back().get();
    }

    T *buscar(int id) const {
        for (const auto &e : elementos)
            if (e->getId() == id) return e.get();
        return nullptr;
    }

    // Busca el primero que cumple la condición (se pasa una lambda)
    T *buscarSi(const std::function<bool(const T &)> &condicion) const {
        auto it = std::find_if(elementos.begin(), elementos.end(),
                               [&](const std::unique_ptr<T> &e) { return condicion(*e); });
        return it == elementos.end() ? nullptr : it->get();
    }

    std::vector<T *> filtrar(const std::function<bool(const T &)> &condicion) const {
        std::vector<T *> resultado;
        for (const auto &e : elementos)
            if (condicion(*e)) resultado.push_back(e.get());
        return resultado;
    }

    std::vector<T *> todos() const {
        return filtrar([](const T &) { return true; });
    }

    int cantidad() const { return static_cast<int>(elementos.size()); }
};
