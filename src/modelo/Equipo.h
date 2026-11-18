// Equipo.h
// PATRÓN COMPOSITE (estructural): un equipo se arma con jugadores sueltos
// (hojas) y grupos (titulares, suplentes) que a su vez contienen jugadores.
// Se tratan igual: a cualquiera se le puede pedir su cantidad de jugadores
// o la lista de sus integrantes.
#pragma once
#include <memory>
#include <string>
#include <vector>

// Componente común
class ComponenteEquipo {
public:
    virtual ~ComponenteEquipo() = default;
    virtual std::string getNombre() const = 0;
    virtual int cantidadJugadores() const = 0;
    virtual void listarJugadores(std::vector<int> &ids) const = 0;
    virtual std::string describir(int nivel = 0) const = 0;   // árbol en texto
};

// Hoja: un jugador
class MiembroEquipo : public ComponenteEquipo {
private:
    int jugadorId;
    std::string nombre;

public:
    MiembroEquipo(int jugadorId, std::string nombre) : jugadorId(jugadorId), nombre(std::move(nombre)) {}
    std::string getNombre() const override { return nombre; }
    int cantidadJugadores() const override { return 1; }
    void listarJugadores(std::vector<int> &ids) const override { ids.push_back(jugadorId); }
    std::string describir(int nivel) const override { return std::string(nivel * 2, ' ') + "- " + nombre + "\n"; }
};

// Compuesto: un grupo de componentes (puede contener jugadores u otros grupos)
class GrupoEquipo : public ComponenteEquipo {
protected:
    std::string nombre;
    std::vector<std::unique_ptr<ComponenteEquipo>> hijos;

public:
    explicit GrupoEquipo(std::string nombre) : nombre(std::move(nombre)) {}
    void agregar(std::unique_ptr<ComponenteEquipo> c) { hijos.push_back(std::move(c)); }

    std::string getNombre() const override { return nombre; }
    int cantidadJugadores() const override {
        int total = 0;
        for (const auto &h : hijos) total += h->cantidadJugadores();   // recursivo
        return total;
    }
    void listarJugadores(std::vector<int> &ids) const override {
        for (const auto &h : hijos) h->listarJugadores(ids);
    }
    std::string describir(int nivel) const override {
        std::string s = std::string(nivel * 2, ' ') + nombre + " (" + std::to_string(cantidadJugadores()) + ")\n";
        for (const auto &h : hijos) s += h->describir(nivel + 1);
        return s;
    }
    std::vector<int> idsJugadores() const {
        std::vector<int> ids;
        listarJugadores(ids);
        return ids;
    }
};

// Equipo inscribible en torneos: grupo raíz con titulares y suplentes
class Equipo : public GrupoEquipo {
private:
    int id = 0;
    int capitanId;
    GrupoEquipo *titulares;
    GrupoEquipo *suplentes;

public:
    Equipo(std::string nombre, int capitanId) : GrupoEquipo(std::move(nombre)), capitanId(capitanId) {
        auto t = std::make_unique<GrupoEquipo>("Titulares");
        auto s = std::make_unique<GrupoEquipo>("Suplentes");
        titulares = t.get();
        suplentes = s.get();
        agregar(std::move(t));
        agregar(std::move(s));
    }

    int getId() const { return id; }
    void setId(int nuevo) { id = nuevo; }
    int getCapitanId() const { return capitanId; }
    GrupoEquipo &getTitulares() { return *titulares; }
    GrupoEquipo &getSuplentes() { return *suplentes; }
    const GrupoEquipo &getTitulares() const { return *titulares; }
    const GrupoEquipo &getSuplentes() const { return *suplentes; }
};
