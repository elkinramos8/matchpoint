// Tablas.cpp
#include "Tablas.h"
#include <QBrush>
#include <QColor>
#include <QHeaderView>
#include <QSignalBlocker>

namespace Tablas {

QTableWidget *crear(const QStringList &encabezados, QWidget *padre) {
    auto *t = new QTableWidget(0, encabezados.size(), padre);
    t->setHorizontalHeaderLabels(encabezados);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    t->horizontalHeader()->setStretchLastSection(true);
    t->verticalHeader()->setVisible(false);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setAlternatingRowColors(true);
    return t;
}

void llenar(QTableWidget *tabla, const QList<QStringList> &filas, const QList<int> &ids) {
    int seleccionPrevia = idSeleccionado(tabla);
    QSignalBlocker bloqueo(tabla);
    tabla->setRowCount(0);   // borra también los botones de las filas anteriores
    tabla->setRowCount(filas.size());
    for (int i = 0; i < filas.size(); ++i) {
        for (int j = 0; j < filas[i].size() && j < tabla->columnCount(); ++j) {
            auto *item = new QTableWidgetItem(filas[i][j]);
            if (j == 0) item->setData(Qt::UserRole, i < ids.size() ? ids[i] : i);
            tabla->setItem(i, j, item);
        }
        int id = i < ids.size() ? ids[i] : i;
        if (id == seleccionPrevia) tabla->selectRow(i);   // conservar la fila elegida
    }
}

int idSeleccionado(const QTableWidget *tabla) {
    auto filas = tabla->selectionModel()->selectedRows();
    if (filas.isEmpty()) return -1;
    auto *item = tabla->item(filas.first().row(), 0);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

void seleccionar(QTableWidget *tabla, int id) {
    QSignalBlocker bloqueo(tabla);
    tabla->clearSelection();
    for (int i = 0; i < tabla->rowCount(); ++i)
        if (tabla->item(i, 0) && tabla->item(i, 0)->data(Qt::UserRole).toInt() == id) tabla->selectRow(i);
}

void llenarCombo(QComboBox *combo, const QStringList &textos, const QList<int> &ids) {
    QVariant anterior = combo->currentData();
    QSignalBlocker bloqueo(combo);
    combo->clear();
    for (int i = 0; i < textos.size(); ++i) combo->addItem(textos[i], i < ids.size() ? ids[i] : i);
    int idx = combo->findData(anterior);
    if (idx >= 0) combo->setCurrentIndex(idx);
}

// Colores de cada estado: fondo y texto
static QColor fondo(TipoCelda t) {
    switch (t) {
        case TipoCelda::LIBRE: return QColor("#c8e6c9");
        case TipoCelda::ELEGIDA: return QColor("#1b7f3b");
        case TipoCelda::ABIERTA: return QColor("#fff59d");
        case TipoCelda::RIVAL: return QColor("#ffcc80");
        case TipoCelda::MIA: return QColor("#90caf9");
        case TipoCelda::OCUPADA: return QColor("#ef9a9a");
        case TipoCelda::PASADA: return QColor("#eeeeee");
    }
    return QColor(Qt::white);
}

QTableWidget *crearGrilla(QWidget *padre) {
    auto *g = new QTableWidget(padre);
    g->setEditTriggers(QAbstractItemView::NoEditTriggers);
    g->setSelectionMode(QAbstractItemView::NoSelection);   // el color ya indica lo elegido
    g->setFocusPolicy(Qt::NoFocus);
    g->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    g->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    g->verticalHeader()->setMinimumSectionSize(20);
    g->setMinimumHeight(330);
    return g;
}

void llenarGrilla(QTableWidget *grilla, const QStringList &dias, const QStringList &horas,
                  const QList<QList<CeldaGrilla>> &celdas) {
    QSignalBlocker bloqueo(grilla);
    grilla->clear();
    grilla->setColumnCount(dias.size());
    grilla->setRowCount(horas.size());
    grilla->setHorizontalHeaderLabels(dias);
    grilla->setVerticalHeaderLabels(horas);
    for (int f = 0; f < celdas.size(); ++f)
        for (int c = 0; c < celdas[f].size(); ++c) {
            const CeldaGrilla &celda = celdas[f][c];
            auto *item = new QTableWidgetItem(celda.texto);
            item->setTextAlignment(Qt::AlignCenter);
            item->setToolTip(celda.ayuda);
            item->setBackground(QBrush(fondo(celda.tipo)));
            item->setForeground(QBrush(celda.tipo == TipoCelda::ELEGIDA ? QColor(Qt::white)
                                       : celda.tipo == TipoCelda::PASADA ? QColor("#9e9e9e") : QColor("#212121")));
            grilla->setItem(f, c, item);
        }
}

QString leyendaGrilla(bool conMiPartido) {
    auto muestra = [](TipoCelda t, const QString &texto) {
        return QString("<span style='background:%1'>&nbsp;&nbsp;&nbsp;&nbsp;</span> %2&nbsp;&nbsp; ")
            .arg(fondo(t).name(), texto);
    };
    return muestra(TipoCelda::LIBRE, "Libre") + muestra(TipoCelda::ABIERTA, "Busca jugadores") + muestra(TipoCelda::RIVAL, "Busca rival") +
           (conMiPartido ? muestra(TipoCelda::MIA, "Mi partido") : QString()) + muestra(TipoCelda::OCUPADA, "Ocupado") +
           muestra(TipoCelda::PASADA, "Ya pasó") + "★ Hora punta (+20 %)";
}

} // namespace Tablas
