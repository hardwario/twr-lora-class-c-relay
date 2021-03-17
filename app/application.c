#include <application.h>
#include <at.h>

#define SEND_UPDATE_INTERVAL (30 * 60 * 1000)
#define MEASURE_INTERVAL (30 * 1000)
#define ALARM_INTERVAL (5 * 60 * 1000)

#define POWER_MODULE_RELAY 1
#define MODULE_RELAY_DEFAULT 2
#define MODULE_RELAY_ALTERNATE 3

#define MESSAGE_SIZE 7

// LED instance
twr_led_t led;
// Button instance
twr_button_t button;
// Lora instance
twr_cmwx1zzabz_t lora;
uint8_t rx_buffer[51];
twr_queue_t tx_queue;
twr_scheduler_task_id_t send_task_id;
// Accelerometer instance
twr_lis2dh12_t lis2dh12;
twr_dice_t dice;
twr_tick_t alarm_timeout;
// Thermometer instance
twr_tmp112_t tmp112;
// Relay module default
twr_module_relay_t relay_0;
// Relay module alternate
twr_module_relay_t relay_1;
// Relay
twr_scheduler_task_id_t relay_pulse_task_id;

TWR_DATA_STREAM_FLOAT_BUFFER(ds_temperature_buffer, (SEND_UPDATE_INTERVAL / MEASURE_INTERVAL))
twr_data_stream_t ds_temperature;

enum
{
    HEADER_BOOT = 0x00,
    HEADER_BEACON = 0x01,
    HEADER_UPDATE = 0x02,
    HEADER_BUTTON_CLICK = 0x03,
    HEADER_BUTTON_HOLD = 0x04,
    HEADER_ALARM = 0x05,
};

void send(uint8_t header)
{
    static uint8_t buffer[MESSAGE_SIZE];
    memset(buffer, 0xff, sizeof(buffer));

    buffer[0] = header;
    buffer[1] = (uint8_t)twr_dice_get_face(&dice); // orientation
    buffer[2] = twr_module_power_relay_get_state();

    twr_module_relay_state_t state;
    state = twr_module_relay_get_state(&relay_0);
    if (state != TWR_MODULE_RELAY_STATE_UNKNOWN)
    {
        buffer[3] = (uint8_t)state;
    }
    state = twr_module_relay_get_state(&relay_1);
    if (state != TWR_MODULE_RELAY_STATE_UNKNOWN)
    {
        buffer[4] = (uint8_t)state;
    }

    float temperature_avg = NAN;

    twr_data_stream_get_average(&ds_temperature, &temperature_avg);
    twr_data_stream_reset(&ds_temperature);
    twr_data_stream_feed(&ds_temperature, &temperature_avg);

    if (!isnan(temperature_avg))
    {
        int16_t temperature_i16 = (int16_t)(temperature_avg * 10.f);

        buffer[5] = temperature_i16 >> 8;
        buffer[6] = temperature_i16;
    }

    if (!twr_queue_put(&tx_queue, buffer, sizeof(buffer)))
    {
        twr_log_warning("TX QUEUE is full");
    }
    twr_scheduler_plan_now(send_task_id);
}

void send_task(void *param)
{
    (void)param;
    static twr_tick_t timeout = 0;

    if (!twr_cmwx1zzabz_is_ready(&lora) || timeout > twr_tick_get())
    {
        twr_scheduler_plan_current_relative(100);
        return;
    }

    static uint8_t buffer[MESSAGE_SIZE];
    size_t length = sizeof(buffer);

    if (twr_queue_get(&tx_queue, buffer, &length))
    {
        twr_cmwx1zzabz_send_message(&lora, buffer, sizeof(buffer));
        twr_atci_print("@SEND: \"");
        twr_atci_print_buffer_as_hex(buffer, sizeof(buffer));
        twr_atci_print("\"\r\n");

        timeout = twr_tick_get() + 10000;
        twr_scheduler_plan_current_absolute(timeout);
    }
}

void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param)
{
    (void)self;
    (void)event_param;
    if (event == TWR_BUTTON_EVENT_CLICK)
    {
        send(HEADER_BUTTON_CLICK);
        twr_scheduler_plan_now(0);
    }
    else if (event == TWR_BUTTON_EVENT_HOLD)
    {
        send(HEADER_BUTTON_HOLD);
        twr_scheduler_plan_now(0);
    }
}

void lis2dh12_event_handler(twr_lis2dh12_t *self, twr_lis2dh12_event_t event, void *event_param)
{
    (void)event_param;
    twr_lis2dh12_result_g_t g;

    if (event == TWR_LIS2DH12_EVENT_UPDATE)
    {
        if (twr_lis2dh12_get_result_g(self, &g))
        {
            twr_dice_feed_vectors(&dice, g.x_axis, g.y_axis, g.z_axis);
        }
    }
    else if (event == TWR_LIS2DH12_EVENT_ALARM)
    {
        twr_lis2dh12_get_result_g(self, &g);
        twr_atci_printfln("@ALARM: %.3f,%.3f,%.3f", g.x_axis, g.y_axis, g.z_axis);

        if (alarm_timeout < twr_tick_get())
        {
            alarm_timeout = twr_tick_get() + ALARM_INTERVAL;
            send(HEADER_ALARM);
            twr_scheduler_plan_now(0);
        }
    }
}

void tmp112_event_handler(twr_tmp112_t *self, twr_tmp112_event_t event, void *event_param)
{
    (void)event_param;

    if (event == TWR_TMP112_EVENT_UPDATE)
    {
        float celsius = NAN;
        // Read temperature
        twr_tmp112_get_temperature_celsius(self, &celsius);
        twr_data_stream_feed(&ds_temperature, &celsius);
    }
}

void relay_pulse_task(void *param)
{
    (void)param;
    twr_module_power_relay_set_state(false);
    send(HEADER_UPDATE);
}

bool execute_rx(size_t length)
{
    if (length % 2 != 0)
    {
        twr_log_error("Bad length");
        return false;
    }

    for (size_t i = 0; i < length; i += 2)
    {
        if (rx_buffer[i] == POWER_MODULE_RELAY)
        {
            twr_scheduler_plan_absolute(relay_pulse_task_id, TWR_TICK_INFINITY);

            if (rx_buffer[i + 1] == 0x00)
            {
                twr_module_power_relay_set_state(false);
                send(HEADER_UPDATE);
            }
            else if (rx_buffer[i + 1] == 0x01)
            {
                twr_module_power_relay_set_state(true);
                send(HEADER_UPDATE);
            }
            else if (rx_buffer[i + 1] == 0x02)
            {
                twr_module_power_relay_set_state(!twr_module_power_relay_get_state());
                send(HEADER_UPDATE);
            }
            else if (rx_buffer[i + 1] <= 0xf2)
            {
                uint32_t duration = (rx_buffer[i + 1] - 2) * 500;
                twr_log_debug("Pulse %lu ms", duration);

                twr_module_power_relay_set_state(true);
                send(HEADER_UPDATE);
                twr_scheduler_plan_from_now(relay_pulse_task_id, duration);
            }
            else
            {
                twr_log_error("Unknown command");
            }
        }
        else if (rx_buffer[i] == MODULE_RELAY_DEFAULT || rx_buffer[i] == MODULE_RELAY_ALTERNATE)
        {
            twr_module_relay_t *relay = rx_buffer[i] == MODULE_RELAY_DEFAULT ? &relay_0 : &relay_1;

            if (rx_buffer[i + 1] == 0x00)
            {
                twr_module_relay_set_state(relay, false);
            }
            else if (rx_buffer[i + 1] == 0x01)
            {
                twr_module_relay_set_state(relay, true);
            }
            else if (rx_buffer[i + 1] == 0x02)
            {
                twr_module_relay_toggle(relay);
            }
            else
            {
                twr_log_error("Unknown command");
            }
        }
        else
        {
            twr_log_error("Unknown relay");
            return false;
        }
    }

    return true;
}

void lora_callback(twr_cmwx1zzabz_t *self, twr_cmwx1zzabz_event_t event, void *event_param)
{
    (void)self;
    (void)event_param;

    if (event == TWR_CMWX1ZZABZ_EVENT_MESSAGE_RECEIVED)
    {

        uint8_t port = twr_cmwx1zzabz_get_received_message_port(self);
        uint32_t length = twr_cmwx1zzabz_get_received_message_data(self, rx_buffer, sizeof(rx_buffer));
        twr_atci_printfln("@RECEIVED: %d,%d", port, length);
        // twr_log_dump(rx_buffer, length, "rx_buffer:");

        if (port != 1)
        {
            twr_log_error("Bad port");
        }

        execute_rx(length);
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_ERROR)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_BLINK_FAST);
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_SEND_MESSAGE_START)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_ON);
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_SEND_MESSAGE_DONE)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_OFF);
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_READY)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_OFF);
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_JOIN_SUCCESS)
    {
        twr_atci_printfln("@JOIN: \"OK\"");
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_JOIN_ERROR)
    {
        twr_atci_printfln("@JOIN: \"ERROR\"");
    }
}

bool at_send(void)
{
    send(HEADER_UPDATE);
    return true;
}

bool at_status(void)
{
    twr_atci_printfln("$STATUS: \"Relay\",%d", twr_module_power_relay_get_state());
    twr_atci_printfln("$STATUS: \"Relay_0\",%i", (int)twr_module_relay_get_state(&relay_0));
    twr_atci_printfln("$STATUS: \"Relay_1\",%i", (int)twr_module_relay_get_state(&relay_1));

    float temperature = NAN;

    twr_data_stream_get_average(&ds_temperature, &temperature);
    if (isnan(temperature))
    {
        twr_atci_printfln("$STATUS: \"Temperature\",null");
    }
    else
    {
        twr_atci_printfln("$STATUS: \"Temperature\",%.1f", temperature);
    }

    twr_lis2dh12_result_g_t g;
    if (twr_lis2dh12_get_result_g(&lis2dh12, &g))
    {
        twr_atci_printfln("$STATUS: \"Acceleration\",%.3f,%.3f,%.3f", g.x_axis, g.y_axis, g.z_axis);
    }
    else
    {
        twr_atci_printfln("$STATUS: \"Acceleration\",null,null,null");
    }

    twr_atci_printfln("$STATUS: \"Orientation\",%d", (int)twr_dice_get_face(&dice));

    return true;
}

bool at_receive_set(twr_atci_param_t *param)
{
    size_t length = sizeof(rx_buffer);
    if (!twr_atci_get_buffer_from_hex_string(param, rx_buffer, &length))
    {
        return false;
    }

    return execute_rx(length);
}

bool at_reboot(void)
{
    twr_atci_printfln("OK");
    twr_system_reset();
    return true;
}

void application_init(void)
{
    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);

    // Initialize LED
    twr_led_init(&led, TWR_GPIO_LED, false, false);
    twr_led_pulse(&led, 2000);

    twr_data_stream_init(&ds_temperature, 1, &ds_temperature_buffer);

    static uint8_t tx_queue_buffer[128];
    twr_queue_init(&tx_queue, tx_queue_buffer, sizeof(tx_queue_buffer));

    // Initialize button
    twr_button_init(&button, TWR_GPIO_BUTTON, TWR_GPIO_PULL_DOWN, false);
    twr_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize accelerometer on core module
    twr_lis2dh12_init(&lis2dh12, TWR_I2C_I2C0, 0x19);
    twr_lis2dh12_set_event_handler(&lis2dh12, lis2dh12_event_handler, NULL);
    twr_lis2dh12_set_update_interval(&lis2dh12, MEASURE_INTERVAL);
    twr_lis2dh12_alarm_t alarm;
    memset(&alarm, 0, sizeof(alarm));
    alarm.z_high = true;
    alarm.threshold = 0.25;
    alarm.duration = 0;
    twr_lis2dh12_set_alarm(&lis2dh12, &alarm);
    twr_dice_init(&dice, TWR_DICE_FACE_UNKNOWN);

    // Initialize thermometer on core module
    twr_tmp112_init(&tmp112, TWR_I2C_I2C0, 0x49);
    twr_tmp112_set_event_handler(&tmp112, tmp112_event_handler, NULL);
    twr_tmp112_set_update_interval(&tmp112, MEASURE_INTERVAL);

    // Initialize power module
    twr_module_power_init();

    // Relay module
    twr_module_relay_init(&relay_0, TWR_MODULE_RELAY_I2C_ADDRESS_DEFAULT);
    twr_module_relay_init(&relay_1, TWR_MODULE_RELAY_I2C_ADDRESS_ALTERNATE);

    // Initialize lora module
    twr_cmwx1zzabz_init(&lora, TWR_UART_UART1);
    twr_cmwx1zzabz_set_event_handler(&lora, lora_callback, NULL);
    twr_cmwx1zzabz_set_class(&lora, TWR_CMWX1ZZABZ_CONFIG_CLASS_C);
    twr_cmwx1zzabz_set_debug(&lora, true);

    // Initialize AT command interface
    at_init(&led, &lora);
    static const twr_atci_command_t commands[] = {
        AT_LORA_COMMANDS,
        {"$SEND", at_send, NULL, NULL, NULL, "Immediately send packet"},
        {"$STATUS", at_status, NULL, NULL, NULL, "Show status"},
        AT_LED_COMMANDS,
        {"$RECEIVE", NULL, at_receive_set, NULL, NULL, "Simulate receive payload"},
        {"$REBOOT", at_reboot, NULL, NULL, NULL, "Reboot"},
        TWR_ATCI_COMMAND_HELP,
        TWR_ATCI_COMMAND_CLAC};
    twr_atci_init(commands, TWR_ATCI_COMMANDS_LENGTH(commands));

    alarm_timeout = twr_tick_get() + 30 * 1000;

    relay_pulse_task_id = twr_scheduler_register(relay_pulse_task, NULL, TWR_TICK_INFINITY);
    send_task_id = twr_scheduler_register(send_task, NULL, TWR_TICK_INFINITY);

    twr_atci_printfln("@BOOT");

    twr_scheduler_plan_current_relative(10 * 1000);
}

void application_task(void)
{
    static bool boot = false;
    if (!boot)
    {
        send(HEADER_BOOT);
        boot = true;
    }
    else
    {
        send(HEADER_BEACON);
    }

    twr_scheduler_plan_current_relative(SEND_UPDATE_INTERVAL);
}
