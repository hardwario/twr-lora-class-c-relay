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
    print('len(data)', len(data))
    if len(data) != 14 and len(data) != 18:
        raise Exception("Bad data length, 14 or 18 characters expected")

    header = int(data[0:2], 16)

    temperature = int(data[10:14], 16) if data[10:14] != 'ffff' else None

    if temperature:
        if temperature > 32768:
            temperature -= 65536
        temperature /= 10.0

    payload = {
        "header": header_lut[header],
        "orientation": int(data[2:4], 16),
        "relay": int(data[4:6], 16),
        "relay_0": int(data[6:8], 16) if data[6:8] != 'ff' else None,
        "relay_1": int(data[8:10], 16) if data[8:10] != 'ff' else None,
        "temperature": temperature,
    }

    if len(data) == 18:
        payload['chester_a_relay_1'] = int(data[6:8], 16) if data[14:16] != 'ff' else None
        payload['chester_a_relay_2'] = int(data[6:8], 16) if data[16:18] != 'ff' else None

    return payload


def pprint(data):
    print('Header :', data['header'])
    print('Orientation :', data['orientation'])
    print('Relay :', data['relay'])
    print('Relay 0 :', data['relay_0'])
    print('Relay 1 :', data['relay_1'])
    print('Temperature :', data['temperature'])
    if 'chester_a_relay_1' in data:
        print('CHESTER A Relay 1', data['chester_a_relay_1'])
        print('CHESTER A Relay 2', data['chester_a_relay_2'])


if __name__ == '__main__':
    if len(sys.argv) != 2 or sys.argv[1] in ('help', '-h', '--help'):
        print("usage: python3 decode.py [data]")
        print("example: python3 decode.py 040300ffff0100")
        exit(1)

    data = decode(sys.argv[1])
    pprint(data)
