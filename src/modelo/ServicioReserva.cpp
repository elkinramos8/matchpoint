// ServicioReserva.cpp
#include "ServicioReserva.h"
#include "Excepciones.h"

std::unique_ptr<ServicioReserva> construirServicio(Dinero precioBase, const std::vector<std::string> &extras) {
    std::unique_ptr<ServicioReserva> servicio = std::make_unique<AlquilerCancha>(precioBase);
    for (const auto &extra : extras) {
        if (extra == "pelota") servicio = std::make_unique<ConPelota>(std::move(servicio));
        else if (extra == "chalecos") servicio = std::make_unique<ConChalecos>(std::move(servicio));
        else if (extra == "arbitro") servicio = std::make_unique<ConArbitro>(std::move(servicio));
        else throw ValidacionError("Extra desconocido: " + extra);
    }
    return servicio;
}
