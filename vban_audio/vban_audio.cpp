#include "vban_audio.h"

namespace esphome {
namespace vban_audio {

void VBANAudio::setup() {
  ESP_LOGI(TAG, "Setting up VBAN Audio");

  sample_queue_ = xQueueCreate(
      VBAN_RING_PACKETS,
      sizeof(AudioPacket));

  if (!sample_queue_) {
    ESP_LOGE(TAG, "Failed to create audio queue");
    return;
  }

  if (microphone_) {
    microphone_->add_data_callback(
        [this](const std::vector<uint8_t> &data) {
          this->microphone_bytes_callback_(data);
        });
  }
}

void VBANAudio::loop() {
  if (task_started_)
    return;

  task_started_ = true;

  xTaskCreatePinnedToCore(
      tx_task_,
      "vban_tx",
      VBAN_TASK_STACK,
      this,
      VBAN_TASK_PRIORITY,
      &tx_task_handle_,
      0);

  ESP_LOGI(TAG, "VBAN TX task started");
}

void VBANAudio::dump_config() {
  ESP_LOGCONFIG(TAG, "VBAN Audio:");
  ESP_LOGCONFIG(TAG, "  Stream name: %s", stream_name_.c_str());
  ESP_LOGCONFIG(TAG, "  Target IP: %s", target_ip_str_.c_str());
  ESP_LOGCONFIG(TAG, "  Port: %u", port_);
  ESP_LOGCONFIG(TAG, "  Sample rate: %u", sample_rate_);
  ESP_LOGCONFIG(TAG, "  Gain: %.2f", gain_);
}

void VBANAudio::set_microphone(microphone::Microphone *mic) {
  microphone_ = mic;
}

void VBANAudio::set_target_ip(const std::string &ip) {
  target_ip_str_ = ip;
}

void VBANAudio::set_target_port(uint16_t port) {
  port_ = port;
}

void VBANAudio::set_sample_rate(uint32_t rate) {
  sample_rate_ = rate;
}

void VBANAudio::set_stream_name(const std::string &name) {
  stream_name_ = name.substr(0, 16);
}

void VBANAudio::set_gain(float gain) {
  if (gain < 0.0f) gain = 0.0f;
  if (gain > 10.0f) gain = 10.0f;
  gain_ = gain;
}

/* VBAN sample-rate index mapping (16 kHz = index 8) */
uint8_t VBANAudio::vban_sr_index_(uint32_t rate) {
  switch (rate) {
    case 6000:   return 0;
    case 12000:  return 1;
    case 24000:  return 2;
    case 48000:  return 3;
    case 96000:  return 4;
    case 192000: return 5;
    case 8000:   return 7;
    case 16000:  return 8;
    case 32000:  return 9;
    case 64000:  return 10;
    default:
      ESP_LOGW(TAG, "Unsupported sample rate %u, defaulting to 48000", rate);
      return 3;
  }
}

void VBANAudio::microphone_bytes_callback_(const std::vector<uint8_t> &data) {
  if (data.size() < 2)
    return;

  const size_t sample_count = data.size() / 2;
  const int16_t *samples =
      reinterpret_cast<const int16_t *>(data.data());

  push_samples_(samples, sample_count);
}

void VBANAudio::push_samples_(const int16_t *samples, size_t count) {
  AudioPacket pkt;

  while (count >= VBAN_SAMPLES_PER_PACKET) {
    for (size_t i = 0; i < VBAN_SAMPLES_PER_PACKET; i++) {
      int32_t s = static_cast<int32_t>(samples[i] * gain_);
      if (s > 32767) s = 32767;
      if (s < -32768) s = -32768;
      pkt.samples[i] = static_cast<int16_t>(s);
    }

    if (xQueueSend(sample_queue_, &pkt, 0) != pdTRUE)
      return;

    samples += VBAN_SAMPLES_PER_PACKET;
    count   -= VBAN_SAMPLES_PER_PACKET;
  }
}

void VBANAudio::tx_task_(void *arg) {
  static_cast<VBANAudio *>(arg)->tx_loop_();
}

void VBANAudio::tx_loop_() {
  AudioPacket pkt;
  uint8_t packet[sizeof(VBANHeader) + sizeof(pkt.samples)];
  uint32_t frame_counter = 0;

  sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock_ < 0) {
    ESP_LOGE(TAG, "Failed to create UDP socket");
    vTaskDelete(nullptr);
    return;
  }

  memset(&dest_addr_, 0, sizeof(dest_addr_));
  dest_addr_.sin_family = AF_INET;
  dest_addr_.sin_addr.s_addr = inet_addr(target_ip_str_.c_str());
  dest_addr_.sin_port = htons(port_);

  ESP_LOGI(TAG, "VBAN dest %s:%u",
           target_ip_str_.c_str(), port_);

  while (true) {
    if (xQueueReceive(sample_queue_, &pkt, portMAX_DELAY) != pdTRUE)
      continue;

    VBANHeader *hdr = reinterpret_cast<VBANHeader *>(packet);
    memcpy(hdr->vban, "VBAN", 4);

    hdr->format_sr      = vban_sr_index_(sample_rate_);
    hdr->format_nbs     = VBAN_SAMPLES_PER_PACKET - 1;
    hdr->format_nbc     = 0;
    hdr->format_format = 0x01;

    memset(hdr->streamname, 0, sizeof(hdr->streamname));
    memcpy(hdr->streamname,
           stream_name_.c_str(),
           stream_name_.size());

    hdr->frame_counter = frame_counter++;

    memcpy(packet + sizeof(VBANHeader),
           pkt.samples,
           sizeof(pkt.samples));

    sendto(sock_,
           packet,
           sizeof(packet),
           0,
           reinterpret_cast<sockaddr *>(&dest_addr_),
           sizeof(dest_addr_));
  }
}

}  // namespace vban_audio
}  // namespace esphome
