#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_bt.h"
#include "soc/uhci_periph.h"
#include "driver/uart.h"
#include "esp_private/periph_ctrl.h" // for enabling UHCI module, remove it after UHCI driver is released
#include "esp_log.h"				 //Device loggin

//For error reporting
static const char *tag = "CONTROLLER_UART_HCI";

//The UART1/2 pin conflict with flash pin, do matrix of the signal and pin
static void uart_gpio_reset(void)
{
#if CONFIG_BTDM_CTRL_HCI_UART_NO == 1
    periph_module_enable(PERIPH_UART1_MODULE);
#elif CONFIG_BTDM_CTRL_HCI_UART_NO == 2
    periph_module_enable(PERIPH_UART2_MODULE);
#endif
    periph_module_enable(PERIPH_UHCI0_MODULE);

#ifdef CONFIG_BTDM_CTRL_HCI_UART_NO
    ESP_LOGI(tag, "HCI UART%d Pin select: TX 5, RX 18, CTS 23, RTS 19", CONFIG_BTDM_CTRL_HCI_UART_NO);

    uart_set_pin(CONFIG_BTDM_CTRL_HCI_UART_NO, 5, 18, 19, 23);
#endif
}

void app_main(void)
{
    
	//Used for error reporting. This is actually just 
	//an int, but is used similar to an ENUM
    esp_err_t ret;

    //Initialize non-volatile storage (NVS). Needed to store  Single 
    //Port 10/100 Fast Ethernet Transceiver (PHY) calibration data
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        
    	//Error reporting and reset
        ESP_ERROR_CHECK(nvs_flash_erase());
        //Try again
        ret = nvs_flash_init();
    }

    //Double check just to be sure
    ESP_ERROR_CHECK( ret );

    uart_gpio_reset();

    //Default intialization state macro. We want to intialize the BT device with this state
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    //Initialize BT controller to allocate task and other resource. This 
    //function should be called only once, before any other BT functions 
    //are called. 
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(tag, "Bluetooth Controller initialize failed: %s", esp_err_to_name(ret));
        return;
    }

    //Enables bt controller. Cannot be called a second time. Must disable and then reenable.
    //Additionally, we are enabling this with BTDM mode which is "dual mode". There is also a 
    //classic bluetooth mode, BLE, and idle.
    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (ret != ESP_OK) {
        ESP_LOGE(tag, "Bluetooth Controller initialize failed: %s", esp_err_to_name(ret));
        return;
    }
}