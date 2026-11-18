cmake_minimum_required(VERSION 3.16)
project(MatchPoint VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8 /W3)   # los textos tienen tildes, ñ y ★ (el código está en UTF-8)
else()
    add_compile_options(-Wall -Wextra)
endif()

find_package(Threads REQUIRED)

# ---------- Modelo + persistencia: biblioteca sin Qt (se prueba sola) ----------
add_library(matchpoint_nucleo STATIC
    src/modelo/Dinero.cpp
    src/modelo/FechaHora.cpp
    src/modelo/Billetera.cpp
    src/modelo/Cancha.cpp
    src/modelo/ServicioReserva.cpp
    src/modelo/Reserva.cpp
    src/modelo/Torneo.cpp
    src/modelo/ModeloMatchPoint.cpp
    src/persistencia/Persistencia.cpp
    src/persistencia/GuardadoAsincrono.cpp
    src/persistencia/DatosDemo.cpp
)
target_include_directories(matchpoint_nucleo PUBLIC src)
target_link_libraries(matchpoint_nucleo PUBLIC Threads::Threads)

# ---------- Pruebas unitarias ----------
enable_testing()
add_executable(pruebas tests/pruebas_modelo.cpp)
target_link_libraries(pruebas PRIVATE matchpoint_nucleo)
add_test(NAME pruebas_modelo COMMAND pruebas)

# ---------- Aplicación Qt (Vista + Controlador) ----------
find_package(Qt6 COMPONENTS Widgets Sql)
if(Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)
    add_executable(matchpoint
        src/main.cpp
        src/persistencia/BaseDatos.cpp
        src/vista/VentanaLogin.cpp
        src/vista/VentanaPrincipal.cpp
        src/vista/Tablas.cpp
        src/controlador/ControladorSesion.cpp
        src/controlador/ControladorPrincipal.cpp
    )
    target_link_libraries(matchpoint PRIVATE matchpoint_nucleo Qt6::Widgets Qt6::Sql)
    set_target_properties(matchpoint PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE OFF)
else()
    message(WARNING "No se encontró Qt6: solo se compilan el modelo y las pruebas")
endif()
