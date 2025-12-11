


#ifndef MIS_LIBRERIAS_H 
#define MIS_LIBRERIAS_H 

//Estándar de C
#include <stdio.h>
#include <string.h>
#include <sys/param.h>


// (Sistema Operativo)
#include "freertos/FreeRTOS.h"      // Configuración básica 
#include "freertos/task.h"          // Tareas (Hilos)
#include "freertos/timers.h"        // Temporizadores software
#include "freertos/event_groups.h"  // Sincronización (Banderas)
#include "freertos/queue.h"         // Colas de mensajes

//Wi-Fi y Sistema ESP32
#include "esp_wifi.h"       // Driver Wi-Fi
#include "esp_log.h"        // Logs (ESP_LOGI)
#include "esp_check.h"      // Macros de error
#include "esp_mac.h"        // Dirección MAC
#include "esp_netif.h"      // Interfaz de red
#include "esp_event.h"      // Loop de eventos
#include "esp_system.h"     // Sistema base
#include "nvs_flash.h"      // Memoria No Volátil (Vital para Wi-Fi)

//LwIP
#include "lwip/inet.h"      // Formateo de IP (inet_addr)
#include "lwip/sockets.h"   // API de Sockets (socket, sendto)
#include "lwip/ip_addr.h"   // Estructuras de IP


//MDNS
#include "mdns.h"
#endif // Fin del ifndef


//ADC
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_timer.h"