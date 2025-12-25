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


dont judge my code too hard, ive had a lot of help from chatgtp, but it works and thats amazing
