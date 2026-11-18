// Torneo.h
// Torneo todos contra todos organizado por un dueño en su cancha.
#pragma once
#include "Dinero.h"
#include <string>
#include <vector>

enum class EstadoTorneo { INSCRIPCIONES, EN_JUEGO, FINALIZADO };
std::string textoEstadoTorneo(EstadoTorneo e);
EstadoTorneo estadoTorneoDesdeTexto(const std::string &t);

struct PartidoTorneo {
    int localId, visitanteId;
    int golesLocal = 0, golesVisitante = 0;
    bool jugado = false;
};

// Fila de la tabla de posiciones
struct FilaTabla {
    int equipoId = 0;
    int pj = 0, pg = 0, pe = 0, pp = 0, gf = 0, gc = 0;
    int puntos() const { return pg * 3 + pe; }
    int diferencia() const { return gf - gc; }
    // Sobrecarga: "a > b" significa que a va más arriba en la tabla
    bool operator>(const FilaTabla &o) const {
        if (puntos() != o.puntos()) return puntos() > o.puntos();
        if (diferencia() != o.diferencia()) return diferencia() > o.diferencia();
        return gf > o.gf;
    }
};

class Torneo {
private:
    int id = 0;
    int duenoId, canchaId;
    std::string nombre;
    Dinero costoInscripcion;
    int maxEquipos;
    EstadoTorneo estado = EstadoTorneo::INSCRIPCIONES;
    std::vector<int> equipos;
    std::vector<PartidoTorneo> partidos;

public:
    Torneo(int duenoId, int canchaId, std::string nombre, Dinero costoInscripcion, int maxEquipos);

    int getId() const { return id; }
    void setId(int nuevo) { id = nuevo; }
    int getDuenoId() const { return duenoId; }
    int getCanchaId() const { return canchaId; }
    const std::string &getNombre() const { return nombre; }
    Dinero getCostoInscripcion() const { return costoInscripcion; }
    int getMaxEquipos() const { return maxEquipos; }
    EstadoTorneo getEstado() const { return estado; }
    const std::vector<int> &getEquipos() const { return equipos; }
    const std::vector<PartidoTorneo> &getPartidos() const { return partidos; }

    bool tieneEquipo(int equipoId) const;
    void inscribir(int equipoId);
    void iniciar();                     // genera el fixture todos contra todos
    void registrarResultado(int indicePartido, int golesLocal, int golesVisitante);
    std::vector<FilaTabla> tablaPosiciones() const;

    void restaurar(EstadoTorneo e, std::vector<int> eq, std::vector<PartidoTorneo> p) {
        estado = e; equipos = std::move(eq); partidos = std::move(p);
    }
};
