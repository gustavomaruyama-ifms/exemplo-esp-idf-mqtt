#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_websocket_client.h"
#include "mqtt_client.h"

#define WIFI_SSID "IFMS-CX-LAB"
#define WIFI_PASSWORD "Coxim@@22"

#define MQTT_CLIENT_ID "10"
#define MQTT_BROKER_URI "mqtt://broker.hivemq.com"
#define MQTT_TOPIC_SUBSCRIBE "ifms/gustavo"
#define MQTT_TOPIC_PUBLISH "ifms/turma"

void publicar(void* cliente_mqtt){
	while (1){
		esp_mqtt_client_publish(cliente_mqtt, MQTT_TOPIC_PUBLISH, "Bom dia - aqui e ESP-32", 0, 1, 0);	// Realizar o publish
		vTaskDelay(2000 / portTICK_RATE_MS);		// Espera 2 segundo para realizar a proxima leitura
	}
}

void manipulador_mqtt(void* args, esp_event_base_t base, int32_t event_id, void *dados){
	esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) dados;
	if(event_id == MQTT_EVENT_CONNECTED){
		esp_mqtt_client_handle_t mqtt_client = event->client;

		printf("CONECTADO AO BROKER MQTT\n");
		esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_SUBSCRIBE, 0);
		xTaskCreatePinnedToCore(publicar, "publicar", 4*1024, mqtt_client, 10, NULL, 1);
	}

	if(event_id == MQTT_EVENT_SUBSCRIBED){
		printf("SUBACK RECEBIDO\n");
	}

	if(event_id == MQTT_EVENT_DATA){
		printf("PUBLISH RECEBIDO\n");
		printf("Topico: %.*s.\n", event->topic_len, event->topic);
		printf("Mensagem: %.*s.\n", event->data_len, event->data);

		char valor[6];
		strncpy(valor, &event->data[0],5);
		valor[5] = '\0';

		if(strcmp(valor, "ligar") == 0){
			gpio_set_level(GPIO_NUM_2, 1);
		}

		if(strcmp(valor, "desli") == 0){
			gpio_set_level(GPIO_NUM_2, 0);
		}
	}
}

void mqtt_init(){
	const esp_mqtt_client_config_t mqtt_config = {
			.uri = MQTT_BROKER_URI,
			.client_id = MQTT_CLIENT_ID
	};

	esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_config);
	esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, &manipulador_mqtt, mqtt_client);
	esp_mqtt_client_start(mqtt_client);
}

void manipulador_de_eventos_wifi(void *arg, esp_event_base_t event_base, int32_t event_id, void *dados){
	printf("Entrou no manipulador de eventos\n");
	if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
		printf("Modo Station de Wifi Iniciado\n");
		esp_wifi_connect();
	}

	if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED){
		printf("Conexao com a rede estabelecida\n");
	}

	if(event_base == IP_EVENT  && event_id == IP_EVENT_STA_GOT_IP){
		printf("Recebeu um endereco IP\n");
		mqtt_init();
	}
}

void app_main(void)
{
	nvs_flash_init(); // inicialização modulo flash
	esp_netif_init(); // inicialização do gerenciador de rede
	esp_event_loop_create_default(); // inicializa um event loop

	esp_netif_create_default_wifi_sta(); // configura o esp como station
	esp_event_handler_register(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &manipulador_de_eventos_wifi, NULL);

	wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT(); // parametros de inicialização do modulo de wifi

	wifi_config_t wifi_config = { // parametros da rede
			.sta = {
					.ssid = WIFI_SSID,
					.password = WIFI_PASSWORD
			}
	};

	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

	esp_wifi_init(&config);
	esp_wifi_set_mode(WIFI_MODE_STA);
	esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
	esp_wifi_start();
}
