#!/usr/bin/env python3
import sys
import __future__

header_lut = {
    0: 'BOOT',
    1: 'BEACON',
    2: 'UPDATE',
    3: 'BUTTON_CLICK',
    4: 'BUTTON_HOLD',
    5: 'ALARM'
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
        "relay": int(data[4:6], 16),
        "relay_0": int(data[6:8], 16) if data[6:8] != 'ff' else None,
        "relay_1": int(data[8:10], 16) if data[8:10] != 'ff' else None,
        "temperature": temperature,
    }


def pprint(data):
    print('Header :', data['header'])
    print('Orientation :', data['orientation'])
    print('Relay :', data['relay'])
    print('Relay 0 :', data['relay_0'])
    print('Relay 1 :', data['relay_1'])
    print('Temperature :', data['temperature'])


if __name__ == '__main__':
    if len(sys.argv) != 2 or sys.argv[1] in ('help', '-h', '--help'):
        print("usage: python3 decode.py [data]")
        print("example: python3 decode.py 040300ffff0100")
        exit(1)

    data = decode(sys.argv[1])
    pprint(data)
