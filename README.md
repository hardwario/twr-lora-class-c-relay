<a href="https://www.hardwario.com/"><img src="https://www.hardwario.com/ci/assets/hw-logo.svg" width="200" alt="HARDWARIO Logo" align="right"></a>

# Firmware for HARDWARIO Lora CLASS-C Relay

[![Travis](https://img.shields.io/travis/hardwario/twr-lora-class-c-relay/master.svg)](https://travis-ci.org/hardwario/twr-lora-class-c-relay)
[![Release](https://img.shields.io/github/release/hardwario/twr-lora-class-c-relay.svg)](https://github.com/hardwario/twr-lora-class-c-relay/releases)
[![License](https://img.shields.io/github/license/hardwario/twr-lora-class-c-relay.svg)](https://github.com/hardwario/twr-lora-class-c-relay/blob/master/LICENSE)
[![Twitter](https://img.shields.io/twitter/follow/hardwario_en.svg?style=social&label=Follow)](https://twitter.com/hardwario_en)

## Description

Control Relay via LoRaWan, you can set On, Off, Toggle or Pulse for between 0.5s to 120s. Measure temperature and react on accelerometer event.

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
* 0x04 CHESTER A Relay 1
* 0x05 CHESTER A Relay 2

Change relay enum:
* 0x00 Off
* 0x01 On
* 0x02 Toggle
* 0x03 Pulse 500 ms
* 0x04 Pulse 1000 ms
* 0x05 Pulse 1.5s
* 0x06 Pulse 2.0s
* ...
* 0xf2 Pulse 120.0s


Examples:

* Power Module Relay ON: `0x0101`
* Power Module Relay OFF: `0x0100`
* Power Module Relay Pulse 3s: `0x0108`
* Power Module Relay ON and Module Relay Default ON: `0x01010201`
* Module Relay Default OFF: `0x0200`

Compute pulse:

hex(floor((delay / 500) + 2))

### Uplink

Big endian

| Byte    | Name        | Type   |  Value / State | note
| ------: | ----------- | ------ | -------- | -------
|       0 | Header      | uint8  |          |
|       1 | Orientation | uint8  |          |
|       2 | Relay | uint8   |  0: Off, 1: On  | Power Module Relay
|       3 | Relay_0 | uint8 | 0: Off, 1: On, 0xff: Unknown | Module Relay default i2c address
|       4 | Relay_1 | uint8 |  0: Off, 1: On, 0xff: Unknown | Module Relay alternate i2c address
|  5 -  6 | Temperature | int16  | multiple 10, unit °C |
|       7 | CHESTER A Relay 1 | uint8   |  0: Off, 1: On  | Power Module Relay
|       8 | CHESTER A Relay 2 | uint8   |  0: Off, 1: On  | Power Module Relay
#### Example decoders:

```
python3 decode.py 040300ffff0100
node decode.js 040300ffff0100
```

### Header enum

* 0 - boot
* 1 - beacon
* 2 - update
* 3 - button click
* 4 - button hold
* 5 - alarm

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

### Generate DEVEUI + APPEUI + APPKEY

```
echo -n 'AT$DEVEUI=' && hexdump -n  8 -e '4/4 "%08X" 1 "\n"' /dev/random
echo -n 'AT$APPEUI=' && hexdump -n  8 -e '4/4 "%08X" 1 "\n"' /dev/random
echo -n 'AT$APPKEY=' && hexdump -n 16 -e '4/4 "%08X" 1 "\n"' /dev/random
```

## License

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT/) - see the [LICENSE](LICENSE) file for details.

---

Made with &#x2764;&nbsp; by [**HARDWARIO s.r.o.**](https://www.hardwario.com/) in the heart of Europe.
