
#include "fota.hpp"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <charconv>
#include <format>
#include "fsm.hpp"
#include "packet.hpp"`

#define FOTA_UART UART_NUM_1
#define PIN_TX 10
#define PIN_RX 11
#define BAUD_RATE 115200

static const char *TAG = "UART_TEST";

using namespace std;
static QueueHandle_t uart_queue;
static QueueHandle_t packet_queue;

void uartinit(void)
{

    ESP_LOGI(TAG, "Starting simple UART transmitter");
    const int uart_buffer_size = 256;

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(FOTA_UART, uart_buffer_size, uart_buffer_size, 256, &uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(FOTA_UART, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(FOTA_UART, PIN_TX, PIN_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_ERROR_CHECK(gpio_set_pull_mode((gpio_num_t)PIN_RX, GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(gpio_set_pull_mode((gpio_num_t)PIN_TX, GPIO_PULLUP_ONLY));
    vTaskDelay(pdMS_TO_TICKS(100));
}

static void uart_task(void *arg)
{
    fsm_state_t fsm_state = READ_ID;
    uint8_t idx;
    Packet *packet;

    while (true)
    {
        uint8_t byte;
        uart_read_bytes(FOTA_UART, &byte, 1, portMAX_DELAY);
        // ESP_LOG_BUFFER_HEX("Byte Received", &byte, 1);

        switch (fsm_state)
        {
        case READ_ID:
        {
            std::cout << "Reading ID\n";
            packet = static_cast<Packet_t *>(pvPortMalloc(sizeof(Packet_t)));
            if (!packet)
            {
                fsm_state = READ_ID;
                break;
            }
            packet->id = byte;
            fsm_state = READ_LENGTH;
            break;
        }
        case READ_LENGTH:
        {
            std::cout << "Reading Length\n";
            if (byte > MAX_PAYLOAD_SIZE)
            {
                vPortFree(packet);
                fsm_state = READ_ID;
                break;
            }

            packet->length = byte;
            idx = 0;
            fsm_state = (byte == 0) ? READ_CRC : READ_PAYLOAD;
            break;
        }
        case READ_PAYLOAD:
        {
            std::cout << "Reading Payload\n";
            packet->payload[idx++] = byte;
            if (idx == packet->length)
            {
                fsm_state = READ_CRC;
                idx = 0;
            }
            break;
        }

        case READ_CRC:
        {
            std::cout << "Reading CRC\n";

            ((uint8_t *)&packet->crc32)[idx++] = byte;
            if (idx == 4)
            {
                uint32_t pcrc = packet->calculate_packet_crc();
                // ESP_LOG_BUFFER_HEX("Packet crc is", &pcrc, sizeof(uint32_t));
                // ESP_LOG_BUFFER_HEX("Packet received crc is", &packet->crc32, sizeof(uint32_t));

                if (pcrc == packet->crc32)
                {
                    if (xQueueSend(packet_queue, &packet, pdMS_TO_TICKS(50)) != pdTRUE)
                    {
                        std::cout << "Command sending to queue failed, freeing\n";
                        vPortFree(packet);
                    }
                    else
                    {
                        std::cout << "Command sent to queue\n";
                    }
                }
                else
                {
                    std::cout << "Command failed to crc: deleting\n";
                    vPortFree(packet);
                }

                fsm_state = READ_ID;
                idx = 0;
            }
            break;
        }

        default:
            break;
        }
    }
}

static std::vector<uint8_t> serialize_packet(const Packet &pkt)
{
    std::vector<uint8_t> out;
    out.reserve(2 + pkt.length + sizeof(uint32_t));

    // Header
    out.push_back(pkt.id);
    out.push_back(pkt.length);

    // Payload
    for (uint8_t i = 0; i < pkt.length; ++i)
    {
        out.push_back(pkt.payload[i]);
    }

    // CRC32 (little-endian, wire-safe)
    const uint32_t crc = pkt.crc32;
    out.push_back(static_cast<uint8_t>(crc >> 0));
    out.push_back(static_cast<uint8_t>(crc >> 8));
    out.push_back(static_cast<uint8_t>(crc >> 16));
    out.push_back(static_cast<uint8_t>(crc >> 24));

    return out;
}

static void send_fota_command(const Packet_t &p)
{
    if (!uart_is_driver_installed(FOTA_UART))
    {
        return;
    }
    std::vector<uint8_t> cmd = serialize_packet(p);
    if (cmd.empty())
    {
        return;
    }

    const int bytes_written = uart_write_bytes(
        FOTA_UART,
        reinterpret_cast<const char *>(cmd.data()),
        cmd.size());

    if (bytes_written != static_cast<int>(cmd.size()))
    {
        ESP_LOGE("FOTA", "UART write failed or partial (%d/%d)",
                 bytes_written, cmd.size());
        return;
    }

    // block until all bytes are physically sent
    uart_wait_tx_done(FOTA_UART, pdMS_TO_TICKS(100));
}

static void fota_task(void *arg)
{
    uint16_t counter = 0;

    fota::FotaTransport *ft = (fota::FotaTransport *)arg;
    Command *cmd = new CommandGetBootloaderVersion{};
    Packet_t p;
    cmd->cmd(p);
    cout << p << endl;
    send_fota_command(p);
    while (1)
    {
        Packet_t *rx_pkt = nullptr;

        // Block until UART task posts a packet
        std::cout << std::format("Waiting for packet\n");
        if (xQueueReceive(packet_queue, &rx_pkt, portMAX_DELAY) == pdTRUE)
        {
            std::cout << std::format("Waiting for packet\n");
            if (rx_pkt == nullptr)
            {
                ESP_LOGE("FOTA", "Received null packet pointer");
                continue;
            }

            // 3. Print packet
            cout << "Received valid packet\n"
                 << *rx_pkt << endl;

            // 4. Validate CRC if needed
            if (rx_pkt->calculate_packet_crc() != rx_pkt->crc32)
            {
                ESP_LOGE("FOTA", "CRC mismatch");
            }

            // 5. Free packet (ownership ends here)
            vPortFree(rx_pkt);
        }
    }
}

extern "C" void app_main(void)
{
    uartinit();
    packet_queue = xQueueCreate(8, sizeof(Packet_t *));
    configASSERT(packet_queue);
    std::cout << "\n\n\nStart \n\n\n";
    fota::FotaTransport ft{};

    xTaskCreate(uart_task, "uart_task", 2048, nullptr, 6, nullptr);
    xTaskCreate(fota_task, "fota_task", 2048, &ft, 5, nullptr);
}
