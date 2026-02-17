@echo off
setlocal

:: ==========================================
::        CONFIGURACIÓN DE LA PRUEBA
:: ==========================================

:: 1. Lista de usuarios a probar. Separados por espacios.
set LISTA_USUARIOS=10 50 100 500 1000

:: 2. Configuración global para todos los escenarios
set RAMP_UP=10
set LOOPS=20
set HOST=3.239.243.92
set PORT=8082

:: 3. Script de prueba
set SCRIPT_NAME=pruebas_mqtt.jmx

:: ==========================================
::        INICIO DE LA EJECUCIÓN
:: ==========================================

echo.
echo ========================================================
echo  INICIANDO SUITE DE PRUEBAS
echo  Escenarios a ejecutar: %LISTA_USUARIOS% usuarios
echo  Host Objetivo: %HOST%:%PORT%
echo ========================================================
echo.

for %%U in (%LISTA_USUARIOS%) do (
    
    echo --------------------------------------------------------
    echo  [ EJECUTANDO ESCENARIO: %%U USUARIOS ]
    echo --------------------------------------------------------

    :: 1. Limpieza: Borrar reportes y JTL viejos de este escenario específico
    if exist Reporte_%%U_Usuarios rmdir /s /q Reporte_%%U_Usuarios
    if exist resultados_%%U.jtl del resultados_%%U.jtl

    :: 2. Ejecución de JMeter
    call jmeter -n -t %SCRIPT_NAME% ^
        -Jhost=%HOST% ^
        -Jport=%PORT% ^
        -Jthreads=%%U ^
        -Jrampup=%RAMP_UP% ^
        -Jloops=%LOOPS% ^
        -l resultados_%%U.jtl ^
        -e -o Reporte_%%U_Usuarios

    echo.
    echo  [!] Escenario de %%U usuarios completado.
    echo  [!] Reporte generado en: Reporte_%%U_Usuarios\index.html
    echo.

    :: 3. Tiempo de espera
    echo  Esperando 5 segundos para ejecutar siguiente prueba...
    timeout /t 5 >nul
)

echo.
echo ========================================================
echo  PRUEBAS FINALIZADAS EXITOSAMENTE
echo ========================================================
pause