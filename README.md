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

dont judge my code too hard, ive had a lot of help from chatgtp, but it works and thats amazing
