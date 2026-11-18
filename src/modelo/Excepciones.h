// Excepciones.h
// Jerarquía de errores propios. El Controlador los atrapa con try/catch
// y muestra el mensaje en la ventana.
#pragma once
#include <stdexcept>
#include <string>

class MatchPointError : public std::runtime_error {
public:
    explicit MatchPointError(const std::string &mensaje) : std::runtime_error(mensaje) {}
};

// Datos de entrada incorrectos (campos vacíos, valores fuera de rango)
class ValidacionError : public MatchPointError {
public:
    using MatchPointError::MatchPointError;
};

// Correo o contraseña incorrectos, correo repetido
class AutenticacionError : public MatchPointError {
public:
    using MatchPointError::MatchPointError;
};

// Horario ocupado, reserva llena, acción no permitida en el estado actual
class ReservaError : public MatchPointError {
public:
    using MatchPointError::MatchPointError;
};

// Saldo insuficiente u operaciones de dinero inválidas
class PagoError : public MatchPointError {
public:
    using MatchPointError::MatchPointError;
};

// Inscripciones y resultados de torneos
class TorneoError : public MatchPointError {
public:
    using MatchPointError::MatchPointError;
};
