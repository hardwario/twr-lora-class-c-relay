function Decoder(bytes, port) {
    const headers = {
        0: 'BOOT',
        1: 'UPDATE',
        2: 'BUTTON_CLICK',
        3: 'BUTTON_HOLD',
        4: 'ALARM'
    }
    var temperature = ((bytes[5] << 8) | bytes[6]);

    if (temperature != 65535) {
        if (temperature > 32768) temperature -= 65536
        temperature /= 10;
    } else {
        temperature = null;
    }

    return {
        header: headers[bytes[0]],
        orientation: bytes[1],
        "state Power Module Relay": bytes[2],
        "state Module Relay Def": bytes[3] != 255 ? bytes[3] : null,
        "state Module Relay Alt": bytes[4] != 255 ? bytes[3] : null,
        "temperature": temperature,
    };
  }

  const buf = [...Buffer.from(process.argv[2], 'hex')];

  console.log(Decoder(buf, 1));
