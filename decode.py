#!/usr/bin/env python3
import sys
import __future__

HEADER_BOOT =  0x00
HEADER_UPDATE = 0x01
HEADER_BUTTON_CLICK = 0x02
HEADER_BUTTON_HOLD  = 0x03
HEADER_ALARM = 0x04

header_lut = {
    HEADER_BOOT: 'BOOT',
    HEADER_UPDATE: 'UPDATE',
    HEADER_BUTTON_CLICK: 'BUTTON_CLICK',
    HEADER_BUTTON_HOLD: 'BUTTON_HOLD',
    HEADER_ALARM: 'ALARM'
}


def decode(data):
    if len(data) != 14:
        raise Exception("Bad data length, 18 characters expected")

    header = int(data[0:2], 16)

    temperature = int(data[10:14], 16) if data[10:14] != 'ffff' else None

    if temperature:
        if temperature > 32768:
            temperature -= 65536
        temperature /= 10.0

    return {
        "header": header_lut[header],
        "orientation": int(data[2:4], 16),
        "state Power Module Relay": int(data[4:6], 16),
        "state Module Relay Def": int(data[6:8], 16) if data[6:8] != 'ff' else None,
        "state Module Relay Alt": int(data[8:10], 16) if data[8:10] != 'ff' else None,
        "temperature": temperature,
    }


def pprint(data):
    print('Header :', data['header'])
    print('Orientation :', data['orientation'])
    print('State Power Module Relay :', data['state Power Module Relay'])
    print('State Module Relay Def :', data['state Module Relay Def'])
    print('State Module Relay Alt :', data['state Module Relay Alt'])
    print('Temperature :', data['temperature'])


if __name__ == '__main__':
    if len(sys.argv) != 2 or sys.argv[1] in ('help', '-h', '--help'):
        print("usage: python3 decode.py [data]")
        print("example: python3 decode.py 040300ffff0100")
        exit(1)

    data = decode(sys.argv[1])
    pprint(data)
