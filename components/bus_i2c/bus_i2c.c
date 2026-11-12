#include "bus_i2c.h"
#include "esp_log.h"

#define PIN_SDA 3
#define PIN_SCL 2

static const char *TAG = "bus_i2c";
static i2c_master_bus_handle_t s_bus;

esp_err_t bus_i2c_init(void)
{
    i2c_master_bus_config_t cfg = {
        .i2c_port = I2C_NUM_0,                 // l'ESP32-S3 a deux contrôleurs I2C, on prend le 0
        .sda_io_num = PIN_SDA,
        .scl_io_num = PIN_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,                // filtre les parasites très courts sur les fils
        .flags.enable_internal_pullup = true,  // résistances de tirage internes, en renfort
    };
    return i2c_new_master_bus(&cfg, &s_bus);
}

i2c_master_bus_handle_t bus_i2c_poignee(void)
{
    return s_bus;
}

void bus_i2c_scanner(void)
{
    ESP_LOGI(TAG, "Scan du bus I2C...");
    int trouves = 0;

    for (int adresse = 0x08; adresse <= 0x77; adresse++) {
        if (i2c_master_probe(s_bus, adresse, 50) == ESP_OK) {
            ESP_LOGI(TAG, "puce trouvé à l'adresse 0x%02x", adresse);
            trouves++;
        }
    }

    ESP_LOGI(TAG, "%d puce(s) trouvée(s)", trouves);
}