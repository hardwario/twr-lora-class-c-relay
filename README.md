<a href="https://www.hardwario.com/"><img src="https://www.hardwario.com/ci/assets/hw-logo.svg" width="200" alt="HARDWARIO Logo" align="right"></a>

# Firmware for HARDWARIO Lora CLASS C Relay

[![Travis](https://img.shields.io/travis/hardwario/twr-lora-class-c-relay/master.svg)](https://travis-ci.org/hardwario/twr-lora-class-c-relay)
[![Release](https://img.shields.io/github/release/hardwario/twr-lora-class-c-relay.svg)](https://github.com/hardwario/twr-lora-class-c-relay/releases)
[![License](https://img.shields.io/github/license/hardwario/twr-lora-class-c-relay.svg)](https://github.com/hardwario/twr-lora-class-c-relay/blob/master/LICENSE)
[![Twitter](https://img.shields.io/twitter/follow/hardwario_en.svg?style=social&label=Follow)](https://twitter.com/hardwario_en)

## Description

Relay control via LoRaWan, measure temperature and react on accelerometer event.

Support [relay on Power Module](https://shop.hardwario.com/power-module) and [Relay Module](https://shop.hardwario.com/relay-module/).

The alarm can be sent every 5 minutes.

If no alarm device sends data every 30 minutes.

The temperature is measured every 30 seconds and the value sent is the average temperature between messages.

## Payload

### Downlink

Port: 1

Relay enum:
* 0x01 Power Module Relay
* 0x02 Module Relay Def
* 0x03 Module Relay Alt

State enum:
* 0x00 OFF
* 0x01 ON

Examples:

* Power Module Relay ON: `0x0101`
* Power Module Relay OFF: `0x0100`
* Power Module Relay ON and Module Relay Default ON: `0x01010201`
* Module Relay Default OFF: `0x0200`

### Uplink

Big endian

| Byte    | Name        | Type   | multiple | unit
| ------: | ----------- | ------ | -------- | -------
|       0 | Header      | uint8  |          |
|       1 | Orientation | uint8  |          |
|       2 | State Power Module Relay | uint8   |  0: off, 1: on  |
|       3 | State Module Relay Def | uint8 | 0: off, 1: on, 0xff: unknown |
|       4 | State Module Relay Alt | uint8 |  0: off, 1: on, 0xff: unknown |
|  5 -  6 | Temperature | int16  | 10       | °C

### Header enum

* 0 - boot
* 1 - update
* 2 - button click
* 3 - button hold
* 4 - alarm

## AT interface

* Baud: 115200
* Data: 8
* Stop: 1
* Parity: None
* Send on enter: LF (\n)

Example connect:
```sh
picocom -b 115200 --omap crcrlf --echo /dev/ttyUSB0
```

```
AT$HELP
```
```
AT$DEVEUI
AT$DEVADDR
AT$NWKSKEY
AT$APPSKEY
AT$APPKEY
AT$APPEUI
AT$BAND 0:AS923, 1:AU915, 5:EU868, 6:KR920, 7:IN865, 8:US915
AT$MODE 0:ABP, 1:OTAA
AT$NWK Network type 0:private, 1:public
AT$JOIN Send OTAA Join packet
AT$SEND Immediately send packet
AT$STATUS Show status
AT$BLINK LED blink 3 times
AT$LED LED on/off
AT$REBOOT Reboot
AT$HELP This help
AT+CLAC List all available AT commands
OK
```

```
AT$STATUS
$STATUS: "Relay",0
$STATUS: "Relay Module Def",-1
$STATUS: "Relay Module Alt",-1
$STATUS: "Temperature",23.7
$STATUS: "Acceleration",0.004,0.028,1.040
$STATUS: "Orientation",1
OK
```

### Example OTAA config

Set the public network mode
```
AT$NWK=1
```

Set band EU8686
```
AT$BAND=5
```

Set mode OTAA
```
AT$MODE=1
```

Get DEVEUI
```
AT$DEVEUI?
```

Set APP id
```
AT$APPEUI=69E304FA1E330A1C
```

Set APP key
```
AT$APPKEY=65F717066C1273D4EF4ACAED608E90F4
```

And join
```
AT$JOIN
```

### Hint how generate APPEUI and APPKEY
```
echo -n 'AT$APPEUI=' && hexdump -n 8 -e '4/4 "%08X" 1 "\n"' /dev/random
echo -n 'AT$APPKEY=' &&hexdump -n 16 -e '4/4 "%08X" 1 "\n"' /dev/random
```

## License

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT/) - see the [LICENSE](LICENSE) file for details.

---

Made with &#x2764;&nbsp; by [**HARDWARIO s.r.o.**](https://www.hardwario.com/) in the heart of Europe.
