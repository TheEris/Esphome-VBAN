# VBAN audio streaming protocol component for ESPHome

Sample setup
```yaml
vban_audio:
  id: vban_tx
  microphone: echo_microphone
  ip_address: 10.0.0.99
  port: 6980
  stream_name: ESP32MIC
  sample_rate: 32000
  gain: 3.0
```

 `microphone`     Microphone ID  
 `ip_address`     IP Adress to send the packet to. Dont forget to set the incoming IP in Voicemeeter  
 `port`           Port to sedn the UDP packet to. Default is 6980  
 `stream_name`    Name of the audio stream, max 16 characters, needs to match the one set in Voicemeeter  
 `sample_rate`    Sample rate of the audio stream, must match the sample rate of microphone  
 `gain`           Gain of the microphone, 1.0 to 10.0. 1.0 is no gain.  

to use this component, add this to your YAML  
```yaml
external_components:
  - source:
      type: git
      url: https://github.com/TheEris/Esphome-VBAN
      ref: main
    components: [ vban_audio ]
```

You can have dynamic gain via `number` component, like so
```yaml
number:
  - platform: template
    id: setGain
    name: "Mic Gain"
    optimistic: True
    min_value: 1
    max_value: 10
    step: 0.1
    initial_value: 1
    on_value: 
      then:
        - lambda: 'id(vban_tx).set_gain(float(x));'
```

and control if youre streaming the audio or not with simple `switch`  
```yaml
switch:
  - platform: template
    name: "Stream"
    id: micSwitch
    optimistic: True
    restore_mode: ALWAYS_OFF
    on_turn_on: 
      then:
        - microphone.capture:
    on_turn_off: 
      then:
        - microphone.stop_capture:
```

## Bonus

Here is a dead simple implemention of VBAN TEXT packet that you can use to controll the Voicemeeter
```yaml
script:
  - id: sendVBANcmd
    parameters:
      msg: string
      stream_name: string
      host: string
      port: int
    then:
      - lambda: |-
            int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
            struct sockaddr_in destination, source;

            static uint32_t frame_counter = 0;

            destination.sin_family = AF_INET;
            destination.sin_port = htons(port);
            destination.sin_addr.s_addr = inet_addr(host.c_str());

            std::vector<uint8_t> packet;
            packet.reserve(28 + msg.size());

            packet.push_back('V');    //VBAN Header
            packet.push_back('B');
            packet.push_back('A');
            packet.push_back('N');

            packet.push_back(0x52);   // SR - 256000baud = 0x40 mask + 0x12 (index 18) = 0x52
            packet.push_back(0x00);   // channel ident
            packet.push_back(0x00);   // nbs
            packet.push_back(0x00);   // Data Format

            // stream name
            {
              char namebuf[16] = {0};
              strncpy(namebuf, stream_name.c_str(), sizeof(namebuf)-1);
              for (size_t i = 0; i < 16; i++) {
                packet.push_back((uint8_t)namebuf[i]);
              }
            }

            // Frame counter uint32_t
            packet.push_back((uint8_t)(frame_counter & 0xFF));
            packet.push_back((uint8_t)((frame_counter >> 8) & 0xFF));
            packet.push_back((uint8_t)((frame_counter >> 16) & 0xFF));
            packet.push_back((uint8_t)((frame_counter >> 24) & 0xFF));
            frame_counter++;

            // text
            packet.insert(packet.end(), msg.begin(), msg.end());

            // send it off
            int n_bytes = ::sendto(sock, packet.data(), packet.size(), 0, reinterpret_cast<sockaddr*>(&destination), sizeof(destination));
            ::close(sock);

            ESP_LOGI("VBAN", "Sent \"%s\" frame %i to %s:%d in %d bytes", msg.c_str(), frame_counter, host.c_str(), port, n_bytes);
```

then simply call this script like so

```yaml
text:
  - platform: template
    mode: TEXT
    id: VBANcmd
    name: "VBAN Command"
    optimistic: True
    on_value: 
      then:
        - script.execute:
            id: sendVBANcmd
            msg: !lambda 'return x.c_str();'
            stream_name: "ESP32CMD"
            host: "10.0.0.99"
            port: 6980
```
You can then put `vban.instream[1].ip=10.0.0.10` to change second VBAN input chanel `from IP` to `10.0.0.10`
All the commands you can use are in the [Voicemeeter UserManual](https://vb-audio.com/Voicemeeter/Voicemeeter_UserManual.pdf)




> dont judge my code too hard, ive had a lot of help from chatgtp, but it works and thats amazing
