#include "lib.h"


//AP y STA
#define SSID "Cecilia" 
#define PASS "tomas2102" 
#define PORT 4444

bool internet_conectado = false;  
static const char *TAG = "WIFI_AP_STA"; 

#define ESP_AP_SSID        "ESP32_Pelele_AP"
#define ESP_AP_PASS        "12345678"
#define MAX_STA_CONN       4

#define ESP_MAXIMUM_RETRY  5
static int s_retry_num = 0;

//ADC GLOBALES
#define ADC_PIN ADC_CHANNEL_6
adc_oneshot_unit_handle_t adc1_handle;
int adc_raw; 
float convertion_adc = 3.3/4095.0;

void start_mdns_service() 
{ 
    ESP_ERROR_CHECK(mdns_init()); 
    mdns_hostname_set("esp32-pelele"); 
    mdns_instance_name_set("Grupo 1 esp32"); 
    mdns_service_add(NULL, "_my_udp", "_udp", PORT, NULL, 0); 
} 

static void udp_stream_task(void *pvParameters){
    char rx_buffer[128];
    char tx_buffer[128];

    struct sockaddr_in my_addr;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Escucha en cualquier IP
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Error creando socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int err = bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Error binding: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Esperando señal de inicio de la PC en puerto %d...", PORT);

    struct sockaddr_storage source_addr; 
    socklen_t socklen = sizeof(source_addr);
    struct sockaddr_in target_addr; 

    while (1) {
        // ESPERA EL START
        ESP_LOGI(TAG, "Esperando comando 'START'...");
        bool streaming = false;

        while (!streaming) {
            socklen = sizeof(source_addr);
            
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            if (len > 0) {
                rx_buffer[len] = 0;
                if (strncmp(rx_buffer, "START", 5) == 0) {
                    memcpy(&target_addr, &source_addr, sizeof(struct sockaddr_in));
                    ESP_LOGI(TAG, "START recibido. Enviando datos...");
                    streaming = true; 
                }
            }
        }
    while (streaming) {
        
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT, NULL, NULL);

        if (len > 0) {
                rx_buffer[len] = 0;
                // SI se recibe END se detiene streaming
                if (strncmp(rx_buffer, "END", 3) == 0) {
                    ESP_LOGI(TAG, "Comando END recibido. Deteniendo transmisión.");
                    streaming = false; 
                    break; 
                }
            }
        // Lectura del ADC
        int ret = (adc_oneshot_read(adc1_handle, ADC_PIN, &adc_raw));
        if (ret == ESP_OK){
            float value = convertion_adc * adc_raw; // 
            int64_t tiempo_us = esp_timer_get_time();
            
        
        sprintf(tx_buffer, "value:%.2f,timestamp:%lld", value, tiempo_us);
        sendto(sock, tx_buffer, strlen(tx_buffer), 0, (struct sockaddr *)&target_addr, sizeof(target_addr));
        }

        //Controlar la velocidad de muestreo
        vTaskDelay(pdMS_TO_TICKS(50)); 
        }
    }
    close(sock);
    vTaskDelete(NULL);
}

void init_adc() {
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12, //12 bits de resolucion
        .atten = ADC_ATTEN_DB_12 // rango completo 3.3V
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_PIN, &config));
}


static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    // Eventos de la conexión STA
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_MAXIMUM_RETRY) {
        esp_wifi_connect();
        s_retry_num++;
        ESP_LOGI(TAG, "Reintentando conectar  (Intento %d/%d)", 
                    s_retry_num, ESP_MAXIMUM_RETRY);
        }
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Conectado a WIFI! IP en la red local es: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
    }

    // Eventos AP
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Dispositivo conectado a red AP. MAC:"MACSTR, MAC2STR(event->mac));
    }
}

void wifi_init_ap_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // AP y STA  interfaces
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(ESP_EVENT_ANY_BASE,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    // Configuración del AP 
    wifi_config_t ap_config = {
        .ap = {
            .ssid = ESP_AP_SSID,
            .ssid_len = strlen(ESP_AP_SSID),
            .channel = 1, // Nota importante abajo sobre el canal
            .password = ESP_AP_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = { .required = false },
        },
    };

    //Configuración de STA
    wifi_config_t sta_config = {
        .sta = {
            .ssid = SSID,
            .password = PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    // ACTIVAR MODO HÍBRIDO
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    
    // Cargar configuraciones
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));

    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "Modo AP+STA iniciado. Esperando conexiones...");
}

void app_main(void)
{
    //Iniciar NVS 
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) { // si no hay espacio o hay una nueva version
        ESP_ERROR_CHECK(nvs_flash_erase()); // borra todo 
        ret = nvs_flash_init(); // inicia de nuevo
    }
    ESP_ERROR_CHECK(ret); //verifica que todo este bien 

    //inicio del ADC
    wifi_init_ap_sta();
    start_mdns_service(); 
    init_adc();
    ESP_LOGI(TAG,"SHAAAW");
    //FRERTOS crea una tarea siendo esta el envio de datos del ADC(pude haber hecho una funcion que leyera pero ya lo tenai avanzado cuando lo pense)
    xTaskCreate(udp_stream_task, "udp_stream", 4096, NULL, 5, NULL);
}

