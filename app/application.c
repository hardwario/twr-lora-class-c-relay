#include <application.h>
#include <at.h>

#define SEND_UPDATE_INTERVAL (30 * 60 * 1000)
#define MEASURE_INTERVAL (30 * 1000)
#define ALARM_INTERVAL (5 * 60 * 1000)

#define POWER_MODULE_RELAY 1
#define MODULE_RELAY_DEFAULT 2
#define MODULE_RELAY_ALTERNATE 3

// LED instance
twr_led_t led;
// Button instance
twr_button_t button;
// Lora instance
twr_cmwx1zzabz_t lora;
// Accelerometer instance
twr_lis2dh12_t lis2dh12;
twr_dice_t dice;
twr_tick_t alarm_timeout;
// Thermometer instance
twr_tmp112_t tmp112;
// Relay module default
twr_module_relay_t relay_d;
// Relay module alternate
twr_module_relay_t relay_a;

TWR_DATA_STREAM_FLOAT_BUFFER(ds_temperature_buffer, (SEND_UPDATE_INTERVAL / MEASURE_INTERVAL))
twr_data_stream_t ds_temperature;

enum
{
    HEADER_BOOT = 0x00,
    HEADER_UPDATE = 0x01,
    HEADER_BUTTON_CLICK = 0x02,
    HEADER_BUTTON_HOLD = 0x03,
    HEADER_ALARM = 0x04,

} header = HEADER_BOOT;

void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param)
{
    (void)self;
    (void)event_param;
    if (event == TWR_BUTTON_EVENT_CLICK)
    {
        header = HEADER_BUTTON_CLICK;
        twr_scheduler_plan_now(0);
    }
    else if (event == TWR_BUTTON_EVENT_HOLD)
    {
        header = HEADER_BUTTON_HOLD;
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
        twr_atci_printf("@ALARM: %.3f,%.3f,%.3f", g.x_axis, g.y_axis, g.z_axis);

        if (alarm_timeout < twr_tick_get())
        {
            alarm_timeout = twr_tick_get() + ALARM_INTERVAL;
            header = HEADER_ALARM;
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

void lora_callback(twr_cmwx1zzabz_t *self, twr_cmwx1zzabz_event_t event, void *event_param)
{
    (void)self;
    (void)event_param;
    if (event == TWR_CMWX1ZZABZ_EVENT_MESSAGE_RECEIVED)
    {
        static uint8_t recv_buffer[51];
        uint8_t port = twr_cmwx1zzabz_get_received_message_port(self);
        uint32_t length = twr_cmwx1zzabz_get_received_message_data(self, recv_buffer, sizeof(recv_buffer));
        twr_atci_printf("@RECEIVED: %d,%d", port, length);
        // twr_log_dump(recv_buffer, length, "recv_buffer:");

        if (port != 1)
        {
            twr_log_error("Bad port");
        }

        if (length % 2 != 0)
        {
            twr_log_error("Bad length");
            return;
        }

        for (uint32_t i = 0; i < length; i += 2)
        {
            if (recv_buffer[i] == POWER_MODULE_RELAY)
            {
                twr_module_power_relay_set_state(recv_buffer[i + 1] == 1);
            }
            else if (recv_buffer[i] == MODULE_RELAY_DEFAULT)
            {
                twr_module_relay_set_state(&relay_d, recv_buffer[i + 1] == 1);
            }
            else if (recv_buffer[i] == MODULE_RELAY_ALTERNATE)
            {
                twr_module_relay_set_state(&relay_a, recv_buffer[i + 1] == 1);
            }
        }
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
        twr_atci_printf("@JOIN: \"OK\"");
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_JOIN_ERROR)
    {
        twr_atci_printf("@JOIN: \"ERROR\"");
    }
}

bool at_send(void)
{
    twr_scheduler_plan_now(0);

    return true;
}

bool at_status(void)
{
    twr_atci_printf("$STATUS: \"Relay\",%d", twr_module_power_relay_get_state());
    twr_atci_printf("$STATUS: \"Relay Module Def\",%i", (int)twr_module_relay_get_state(&relay_d));
    twr_atci_printf("$STATUS: \"Relay Module Alt\",%i", (int)twr_module_relay_get_state(&relay_a));

    float temperature = NAN;

    twr_data_stream_get_average(&ds_temperature, &temperature);
    if (isnan(temperature))
    {
        twr_atci_printf("$STATUS: \"Temperature\",null");
    }
    else
    {
        twr_atci_printf("$STATUS: \"Temperature\",%.1f", temperature);
    }

    twr_lis2dh12_result_g_t g;
    if (twr_lis2dh12_get_result_g(&lis2dh12, &g))
    {
        twr_atci_printf("$STATUS: \"Acceleration\",%.3f,%.3f,%.3f", g.x_axis, g.y_axis, g.z_axis);
    }
    else
    {
        twr_atci_printf("$STATUS: \"Acceleration\",null,null,null");
    }

    twr_atci_printf("$STATUS: \"Orientation\",%d", (int)twr_dice_get_face(&dice));

    return true;
}

bool at_reboot(void)
{
    twr_atci_printf("OK");
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
    twr_module_relay_init(&relay_d, TWR_MODULE_RELAY_I2C_ADDRESS_DEFAULT);
    twr_module_relay_init(&relay_a, TWR_MODULE_RELAY_I2C_ADDRESS_ALTERNATE);

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
        {"$REBOOT", at_reboot, NULL, NULL, NULL, "Reboot"},
        TWR_ATCI_COMMAND_HELP,
        TWR_ATCI_COMMAND_CLAC};
    twr_atci_init(commands, TWR_ATCI_COMMANDS_LENGTH(commands));

    twr_scheduler_plan_current_relative(10 * 1000);
    alarm_timeout = twr_tick_get() + 30 * 1000;

    twr_atci_printf("@BOOT");
}

void application_task(void)
{
    if (!twr_cmwx1zzabz_is_ready(&lora))
    {
        twr_scheduler_plan_current_relative(100);

        return;
    }

    static uint8_t buffer[7];

    memset(buffer, 0xff, sizeof(buffer));

    buffer[0] = header;
    buffer[1] = (uint8_t)twr_dice_get_face(&dice); // orientation
    buffer[2] = twr_module_power_relay_get_state();

    twr_module_relay_state_t state;
    state = twr_module_relay_get_state(&relay_d);
    if (state != TWR_MODULE_RELAY_STATE_UNKNOWN) {
        buffer[3] = (uint8_t) state;
    }
    state = twr_module_relay_get_state(&relay_a);
    if (state != TWR_MODULE_RELAY_STATE_UNKNOWN) {
        buffer[4] = (uint8_t) state;
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

    twr_cmwx1zzabz_send_message(&lora, buffer, sizeof(buffer));

    static char tmp[sizeof(buffer) * 2 + 1];
    for (size_t i = 0; i < sizeof(buffer); i++)
    {
        sprintf(tmp + i * 2, "%02x", buffer[i]);
    }

    twr_atci_printf("@SEND: %s", tmp);

    header = HEADER_UPDATE;

    twr_scheduler_plan_current_relative(SEND_UPDATE_INTERVAL);
}
