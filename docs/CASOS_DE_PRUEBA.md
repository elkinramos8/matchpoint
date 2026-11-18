# Casos de prueba de aceptación — MatchPoint

Estas pruebas se hacen **a mano, sobre la aplicación**, como lo haría un usuario real. Complementan las
24 pruebas unitarias automáticas (`ctest`), que revisan las reglas del modelo sin abrir la ventana.

**Cómo usar esta lista**

- Cada integrante toma un bloque (A–E) y lo recorre en unos 15–20 minutos, marcando **Pasó** o **Falló**.
- Si algo falla, anotar qué se hizo y qué se vio. Cada error corregido se convierte en una prueba unitaria.
- Antes de empezar, borrar la base para partir de los datos de ejemplo:
  `rm ~/Library/Application\ Support/MatchPoint/matchpoint.db` (macOS) y abrir `./build/matchpoint`.
- Todas las cuentas de ejemplo usan la clave `1234`.
- **Hora recomendada:** abrir la app entre las **9:00 y las 21:59**. Antes de las 9:00 ningún partido de hoy
  ha empezado (C-9 y E-2 no tendrían filas). Después de las 22:00 no hay partido "por vencer" (C-8).
- Los bloques se pueden hacer en cualquier orden. Si un caso cambia datos que usa otro, se indica.

**Resultado de la ejecución (18/11/2026): 50 de 50 casos pasan (✔).** Los casos se ejecutaron con un programa
que maneja la interfaz real: hace clic en la grilla y en los botones de cada fila, llena los formularios y
responde los diálogos. Se probó como si la app se abriera en distintas fechas y horas, incluidos los días de
exposición (20, 26 y 27 de diciembre). Al ejecutarlos se encontraron y corrigieron tres problemas:

1. En los datos de ejemplo, el reto de Elkin a Los Cóndores fallaba porque un jugador de Los Pumas ya tenía
   otro partido a esa hora. Ahora los escenarios de la demo se crean en horarios donde todos están libres.
2. El partido "por vencer" de Elkin no aparecía si se abría la app después de las 20:00.
3. La devolución por cancelación tardía no decía que se cobró una multa.

---

## A. Cuenta y sesión — Integrante 1

| ID | Qué se prueba | Pasos | Resultado esperado | Pasó |
|---|---|---|---|---|
| A-1 | Iniciar sesión | Ingresar `elkin@matchpoint.pe` / `1234` | Se abre la ventana del jugador con su nombre, tarjeta •••• y reputación | ✔ |
| A-2 | Clave incorrecta | `elkin@matchpoint.pe` / `9999` | Mensaje rojo "Correo o contraseña incorrectos"; no entra | ✔ |
| A-3 | Crear cuenta de jugador | Pestaña **Crear cuenta** → Soy: Jugador, elegir posición en la lista, tarjeta `4321` | Entra; el encabezado dice "Tarjeta •••• 4321 · Reputación: Nuevo" | ✔ |
| A-4 | Tarjeta inválida | Crear cuenta con tarjeta `12a` | "Ingresa los 4 últimos dígitos de tu tarjeta"; no se crea | ✔ |
| A-5 | Correo repetido | Crear cuenta con `elkin@matchpoint.pe` | "Ya existe una cuenta con ese correo" | ✔ |
| A-6 | Crear cuenta de dueño | Soy: Dueño de cancha, nombre del negocio (no pide tarjeta) | Entra a la ventana del dueño (Mis canchas, Reservas recibidas…) | ✔ |
| A-7 | Datos guardados (SQLite) | Hacer una reserva, cerrar la app y volver a abrirla | La reserva y el pago siguen ahí | ✔ |
| A-8 | Cerrar sesión | Botón **Cerrar sesión** | Vuelve a la pantalla de inicio | ✔ |

## B. Reservar una cancha — Integrante 2 (cuenta: Elkin)

| ID | Qué se prueba | Pasos | Resultado esperado | Pasó |
|---|---|---|---|---|
| B-1 | Grilla de la semana | **Canchas y reservar** → elegir El Golazo | Grilla de 7 días por hora: verde libre, amarillo busca jugadores, naranja busca rival, azul "Mi partido", rojo ocupado, gris ya pasó; ★ = hora punta | ✔ |
| B-2 | Elegir horario | Clic en una casilla verde a 3 o más días | La casilla dice "✔ Elegido" y aparece el total | ✔ |
| B-3 | Hora punta | Comparar una casilla de las 18:00 con una de las 19:00 | 19:00 cuesta 20 % más (S/ 60 → S/ 72) y dice "hora punta ★" | ✔ |
| B-4 | Extras (Decorator) | Marcar **Pelota** | El total sube S/ 5 | ✔ |
| B-5 | Reserva privada | Modo "Privada" → **Reservar y pagar con mi tarjeta** | Mensaje con comprobante `MP-…`; aparece en **Mis reservas** y en **Mis pagos** | ✔ |
| B-6 | Horario ocupado | Entrar con otra cuenta (diego) y hacer clic en ese mismo horario | "Ese horario ya está reservado" | ✔ |
| B-7 | Choque de horario | Con Elkin, reservar otra cancha a la misma hora que B-5 | "Elkin Ramos ya tiene otro partido a esa misma hora" | ✔ |
| B-8 | Tarjeta rechazada | Crear un jugador con tarjeta `0000` e intentar reservar | "El banco rechazó el pago con la tarjeta •••• 0000"; no se reserva | ✔ |
| B-9 | Cancelar a tiempo | **Mis reservas** → **Cancelar** en la reserva de B-5 | Avisa "Se te devolverá todo el dinero"; en Mis pagos aparece la devolución completa | ✔ |
| B-10 | Cancelar tarde | Reservar un horario para dentro de pocas horas y cancelarlo | Avisa "perderás el 50 %"; devolución "cancelación tardía (multa del 50 %)"; la reputación baja (1 ★) | ✔ |

## C. Buscar jugadores, solicitudes y lista de espera — Integrante 3

| ID | Qué se prueba | Pasos | Resultado esperado | Pasó |
|---|---|---|---|---|
| C-1 | "Somos X, buscamos Y" | Con Diego: Modo **Buscar jugadores**, somos 6, buscamos 4, a 3 o más días → reservar | La casilla queda amarilla "Abierto 6/10"; el precio indica la parte de cada jugador (total ÷ 10) | ✔ |
| C-2 | Pedir unirse | Con Lucía: clic en esa casilla amarilla → Sí | "Solicitud enviada"; **no** se cobra; aparece en **Solicitudes → Enviadas** | ✔ |
| C-3 | Solicitud repetida | Lucía vuelve a hacer clic | "Ya enviaste tu solicitud" | ✔ |
| C-4 | Aceptar | Con Diego: **Solicitudes → Recibidas** → ✔ Aceptar a Lucía | Lucía paga su parte (Mis pagos) y Diego la recibe; el partido pasa a 7/10 | ✔ |
| C-5 | Rechazar | Otro jugador pide unirse; Diego pulsa ✖ Rechazar | No se cobra; el jugador recibe la notificación "El organizador no aceptó…" | ✔ |
| C-6 | Lista de espera | Completar el partido (10/10) y que otro jugador haga clic en "Completo" | Queda "📋 Lista de espera #1"; Diego solo ve el botón Rechazar | ✔ |
| C-7 | Retiro a tiempo | Lucía → **Mis reservas** → **Retirarme** | Se le devuelve su parte; el partido se reabre y Diego ya puede **Aceptar** al de la lista de espera | ✔ |
| C-8 | Partido por vencer | Con Elkin: ver **Notificaciones** y **Mis reservas** | Aviso "Tu partido … aún busca N jugadores…"; botón **Jugar así** → el partido deja de buscar y no se cancela | ✔ |
| C-9 | Solicitudes de la demo | Con Elkin: **Solicitudes → Recibidas** | Lucía y Pedro (con su reputación) y el reto de Halcones FC; se pueden aceptar | ✔ |
| C-10 | Buscar con menos de 24 h | Modo Buscar jugadores en un horario de hoy o mañana temprano | "Para buscar jugadores o rival reserva con más de 24 h de anticipación…" | ✔ |
| C-11 | Retiro tarde | Retirarse de un partido que empieza en menos de 24 h | No hay devolución y la reputación baja (2 ★) | ✔ |

> La **cancelación automática** (si el partido no se completa a 24 h se cancela y se devuelve todo) no se
> puede esperar en vivo: la cubre la prueba unitaria *"Convocatoria: plazo de 24 h"*.

## D. Equipos, equipo contra equipo y torneos — Integrante 4

| ID | Qué se prueba | Pasos | Resultado esperado | Pasó |
|---|---|---|---|---|
| D-1 | Crear equipo (Composite) | Con José: **Equipos y torneos** → nombre y titulares (nombres separados por comas) → **Crear equipo** | "Equipo … creado con N jugadores"; al seleccionarlo se ve el árbol titulares/suplentes | ✔ |
| D-2 | Jugador inexistente | Crear equipo con un nombre que no está registrado | "No existe el jugador …" | ✔ |
| D-3 | Retar a un equipo | Con Elkin: clic en la casilla naranja "Los Cóndores busca rival" → elegir Los Pumas | "Reto enviado. Tu equipo paga su mitad solo si lo aceptan" | ✔ |
| D-4 | Aceptar un reto | Con Elkin: **Solicitudes** → ✔ Aceptar "Equipo Halcones FC" | En Mis reservas: "Los Pumas vs Halcones FC"; el capitán de Halcones pagó la mitad | ✔ |
| D-5 | Jugador en ambos equipos | Retar a Los Pumas con un equipo que tenga a Kevin Apaza | "Kevin Apaza está en los dos equipos: no pueden jugar entre sí" | ✔ |
| D-6 | Inscribir en torneo | Con un capitán: **Inscribir mi equipo** en "Copa El Golazo 2026" | La columna Equipos sube (p. ej. 2/4 → 3/4) | ✔ |
| D-7 | Iniciar torneo | Con Rosa: **Torneos** → **Iniciar** | Fixture todos contra todos (3 equipos → 3 partidos) | ✔ |
| D-8 | Registrar resultado | Elegir el torneo, poner 3 – 1 y pulsar **Registrar** en el partido 1 | La tabla de posiciones da 3 puntos al ganador | ✔ |
| D-9 | Torneo terminado | Seleccionar "Copa Misti Relámpago" | Tabla final y "🏆 Campeón: …" | ✔ |

## E. Dueño de cancha — Integrante 5 (cuenta: Rosa)

| ID | Qué se prueba | Pasos | Resultado esperado | Pasó |
|---|---|---|---|---|
| E-1 | Ocupación de mi cancha | **Mis canchas** → elegir una cancha; clic en una casilla | Grilla con quién reservó; al hacer clic muestra organizador, jugadores y estado | ✔ |
| E-2 | Partidos por calificar | **Reservas recibidas** | Arriba los partidos de hoy que ya empezaron, con ⭐ Todo bien / ⏰ Llegaron tarde (y ❌ No vinieron pasados 25 min) | ✔ |
| E-3 | Todo bien | Pulsar ⭐ Todo bien | El partido queda "Jugada"; cada jugador recibe 5 ★ y su reputación se actualiza en **Ranking** | ✔ |
| E-4 | Tolerancia | En un partido que empezó hace menos de 25 min | No aparece ❌ No vinieron | ✔ |
| E-5 | El jugador califica la cancha | Con el organizador de E-3: **Mis reservas** → **Calificar ★** → 4 | La cancha suma un voto en **Ranking**; el botón desaparece (no se califica dos veces) | ✔ |
| E-6 | Registrar cancha | Llenar los datos → **Registrar nueva cancha** | Aparece en Mis canchas y los jugadores ya la ven | ✔ |
| E-7 | Nombre repetido | Registrar otra con el mismo nombre | Mensaje de error; no se crea | ✔ |
| E-8 | Cambiar horario | Elegir El Golazo, poner horario 8 a 12 → **Guardar cambios de la seleccionada** | Se rechaza porque quedarían reservas fuera del horario | ✔ |
| E-9 | Ingresos | **Ingresos** | Pagos recibidos, devoluciones y multas, cada uno con su comprobante `MP-…` | ✔ |

## F. Comunes (cualquiera)

| ID | Qué se prueba | Pasos | Resultado esperado | Pasó |
|---|---|---|---|---|
| F-1 | Notificaciones | Pestaña **Notificaciones** | De la más reciente a la más antigua, con "Hoy / Ayer / fecha" y tipo (⚽ 📨 💰 🏆) | ✔ |
| F-2 | Ranking | Pestaña **Ranking** | Canchas por estrellas; jugadores por reputación (los nuevos al final) | ✔ |
| F-3 | Refresco automático (Observer) | Aceptar una solicitud (C-4) y revisar las demás pestañas sin recargar | Mis reservas, Mis pagos, la grilla y Notificaciones ya muestran el cambio | ✔ |
