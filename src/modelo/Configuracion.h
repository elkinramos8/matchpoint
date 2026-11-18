// Configuracion.h
// PATRÓN SINGLETON (creacional): reglas globales del sistema que todas las
// partes deben compartir (multas, tolerancia, precios de extras...).
#pragma once
#include "Dinero.h"

class Configuracion {
public:
    // Punto de acceso global a la única instancia
    static Configuracion &obtenerInstancia() {
        static Configuracion instancia;   // se crea una sola vez (thread-safe desde C++11)
        return instancia;
    }

    // Sin copias ni movimientos: no puede existir una segunda configuración
    Configuracion(const Configuracion &) = delete;
    Configuracion &operator=(const Configuracion &) = delete;
    Configuracion(Configuracion &&) = delete;
    Configuracion &operator=(Configuracion &&) = delete;

    const char *tarjetaRechazada() const { return "0000"; }   // simula que el banco rechaza el pago
    int horasCancelacionGratis() const { return horasCancelacionGratis_; }
    double porcentajeMulta() const { return porcentajeMulta_; }
    int toleranciaMinutos() const { return toleranciaMinutos_; }
    int diasReservables() const { return diasReservables_; }
    double recargoHoraPunta() const { return recargoHoraPunta_; }
    int horaPuntaInicio() const { return horaPuntaInicio_; }
    Dinero precioPelota() const { return precioPelota_; }
    Dinero precioChalecos() const { return precioChalecos_; }
    Dinero precioArbitro() const { return precioArbitro_; }
    int maxJugadoresPorPartido() const { return maxJugadores_; }
    int horasAvisoConvocatoria() const { return horasAviso_; }
    // Estrellas que recibe un jugador según lo que pasó (reputación bidireccional)
    int estrellasTodoBien() const { return 5; }
    int estrellasLlegoTarde() const { return 3; }
    int estrellasRetiroTardio() const { return 2; }
    int estrellasCancelacionTardia() const { return 1; }
    int estrellasNoSePresento() const { return 0; }

private:
    Configuracion() = default;   // constructor privado

    int horasCancelacionGratis_ = 24; // cancelar antes de esto no tiene multa; también es el límite para completar
    double porcentajeMulta_ = 0.5;    // multa por cancelar tarde (50 %)
    int toleranciaMinutos_ = 25;      // espera máxima antes de "No se presentó"
    int diasReservables_ = 7;         // se reserva para los próximos 7 días
    double recargoHoraPunta_ = 0.2;   // +20 % desde las 19:00
    int horaPuntaInicio_ = 19;
    Dinero precioPelota_{5.0};
    Dinero precioChalecos_{8.0};
    Dinero precioArbitro_{20.0};
    int maxJugadores_ = 14;
    int horasAviso_ = 3;              // aviso al organizador antes del límite para completar el partido
};
