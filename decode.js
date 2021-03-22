function Decoder(bytes, port) {
    var lut = {
        0: 'BOOT',
        1: 'BEACON',
        2: 'UPDATE',
        3: 'BUTTON_CLICK',
        4: 'BUTTON_HOLD',
        5: 'ALARM'
    };

    var temperature = ((bytes[5] << 8) | bytes[6]);

    if (temperature != 65535) {
        if (temperature > 32768) temperature -= 65536
        temperature /= 10;
    } else {
        temperature = null;
    }

    return {
        header: lut[bytes[0]],
        orientation: bytes[1],
        relay: bytes[2],
        relay_0: bytes[3] != 255 ? bytes[3] : null,
        relay_1: bytes[4] != 255 ? bytes[3] : null,
        "temperature": temperature,
    };
  }

  const buf = [...Buffer.from(process.argv[2], 'hex')];

  console.log(Decoder(buf, 1));
