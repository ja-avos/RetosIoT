# Pruebas de Carga y Estrés MQTT con Apache JMeter

Este repositorio contiene un conjunto de pruebas automatizadas para evaluar el rendimiento de un bróker MQTT bajo diferentes escenarios de carga utilizando Apache JMeter.

## 📋 Descripción del Escenario

El plan de pruebas (`pruebas_mqtt.jmx`) simula el comportamiento de múltiples dispositivos IoT interactuando con el bróker simultáneamente. El flujo de trabajo por usuario/hilo es el siguiente:

1.  **Conexión SSL**: Cada usuario virtual se conecta al bróker MQTT seguro usando credenciales de un archivo CSV (`usuarios.csv`) y un almacén de confianza (`truststore.p12`).
2.  **Suscripción**: El usuario se suscribe a un tópico específico (`colombia/cundinamarca/${city_topic}/${user}`).
3.  **Publicación**: El usuario publica un mensaje JSON con un ID único y timestamp en el mismo tópico.
4.  **Validación**: Se verifica que el mensaje recibido por la suscripción coincida con el ID del mensaje publicado, garantizando la integridad de la entrega.
5.  **Desconexión**: El usuario cierra la conexión con el bróker de manera limpia.

## 🛠️ Requisitos Previos

1.  **Java**: Tener instalado Java (JRE o JDK) versión 8 o superior.
2.  **Apache JMeter**:
    *   Descomprimir el archivo `.zip` de JMeter incluido en esta carpeta (o descargar Apache JMeter 5.x).
    *   Este paquete ya incluye los plugins necesarios para MQTT (xmeter-plugins).
3.  **Variables de Entorno**:
    *   Agregar la ruta de la carpeta `bin` de JMeter a la variable de entorno `PATH` de su sistema operativo.
    *   *Ejemplo en Windows*: `C:\Herramientas\apache-jmeter-5.6.3\bin`

## 🚀 Ejecución de las Pruebas

Se ha provisto un script automatizado (`ejecutar_pruebas.bat`) que ejecuta secuencialmente varios escenarios de carga para comparar el rendimiento.

### Escenarios Configurados
El script ejecuta pruebas con las siguientes cantidades de usuarios concurrentes (hilos):
*   10 Usuarios
*   50 Usuarios
*   100 Usuarios
*   500 Usuarios
*   1000 Usuarios

**Parámetros por defecto:**
*   **Host**: `3.239.243.92`
*   **Puerto**: `8082` (SSL)
*   **Ramp-up**: 10 segundos
*   **Iteraciones (Loops)**: 20

### Pasos para ejecutar:

1.  Abra una terminal (CMD o PowerShell) en la carpeta donde se encuentra este README.
2.  Ejecute el script:
    ```cmd
    ejecutar_pruebas.bat
    ```
3.  El script limpiará ejecuciones anteriores, correrá las pruebas para cada escenario y generará reportes HTML automáticamente.

## 📊 Resultados y Reportes

Al finalizar la ejecución, encontrará nuevas carpetas en el directorio con el formato `Reporte_X_Usuarios` (ej. `Reporte_10_Usuarios`, `Reporte_50_Usuarios`).

Dentro de cada carpeta, abra el archivo `index.html` en su navegador web para visualizar el **Dashboard de Reporte de JMeter**, que incluye:
*   Estadísticas de APDEX (Satisfacción de usuario).
*   Resumen de Peticiones (Éxitos vs. Errores).
*   Tiempos de Respuesta (Promedio, Percentiles 90/95/99).
*   Rendimiento (Throughput/Transacciones por segundo).

## 🎥 Demo

A continuación se presenta un video demostrativo de la ejecución de las pruebas y la generación de reportes:



---
**Nota**: Asegúrese de que el archivo `truststore.p12` y `usuarios.csv` estén en el mismo directorio desde donde se ejecuta la prueba para evitar errores de archivo no encontrado.
