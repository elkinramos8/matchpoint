// UsuarioFactory.h
// PATRÓN FACTORY METHOD (creacional): único lugar que conoce las subclases
// de Usuario. El resto del programa solo trabaja con Usuario.
#pragma once
#include "DuenoCancha.h"
#include "Excepciones.h"
#include "Jugador.h"
#include <memory>

enum class TipoUsuario { JUGADOR, DUENO };

class UsuarioFactory {
public:
    static std::unique_ptr<Usuario> crear(TipoUsuario tipo, const std::string &nombre,
                                          const std::string &correo, const std::string &claveHash,
                                          const std::string &detalle) {
        switch (tipo) {
            case TipoUsuario::JUGADOR:
                return std::make_unique<Jugador>(nombre, correo, claveHash, detalle);
            case TipoUsuario::DUENO:
                return std::make_unique<DuenoCancha>(nombre, correo, claveHash, detalle);
        }
        throw ValidacionError("Tipo de usuario no reconocido");
    }

    // El formulario y el archivo de datos envían el rol como texto
    static TipoUsuario tipoDesdeTexto(const std::string &rol) {
        if (rol == "Jugador") return TipoUsuario::JUGADOR;
        if (rol == "Dueño de cancha") return TipoUsuario::DUENO;
        throw ValidacionError("Rol desconocido: " + rol);
    }
};
