// Tablas.h
// Funciones de apoyo de la Vista para llenar tablas y combos.
// Cada fila guarda un id oculto que el Controlador usa para saber qué se eligió.
#pragma once
#include <QComboBox>
#include <QList>
#include <QStringList>
#include <QTableWidget>

// Una casilla de la grilla semanal: la Vista solo sabe su texto y su color
enum class TipoCelda { LIBRE, ELEGIDA, ABIERTA, RIVAL, MIA, OCUPADA, PASADA };
struct CeldaGrilla {
    QString texto;
    QString ayuda;   // tooltip
    TipoCelda tipo;
};

namespace Tablas {
QTableWidget *crear(const QStringList &encabezados, QWidget *padre);
void llenar(QTableWidget *tabla, const QList<QStringList> &filas, const QList<int> &ids = {});
int idSeleccionado(const QTableWidget *tabla);   // -1 si no hay selección
void seleccionar(QTableWidget *tabla, int id);    // sin emitir señales
void llenarCombo(QComboBox *combo, const QStringList &textos, const QList<int> &ids);

// Grilla de disponibilidad: filas = horas, columnas = días
QTableWidget *crearGrilla(QWidget *padre);
void llenarGrilla(QTableWidget *grilla, const QStringList &dias, const QStringList &horas,
                  const QList<QList<CeldaGrilla>> &celdas);   // celdas[fila][columna]
QString leyendaGrilla(bool conMiPartido);
}
