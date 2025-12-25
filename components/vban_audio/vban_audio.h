#pragma once

#include "esphome/core/component.h"
#include "esphome/components/microphone/microphone.h"
#include "esphome/core/log.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <lwip/sockets.h>
#include <lwip/inet.h>

#include <string>
#include <vector>

namespace esphome {
namespace vban_audio {

static const char *const TAG = "vban_audio";

/* Tunables */
#define VBAN_SAMPLES_PER_PACKET 128
#define VBAN_RING_PACKETS       32
#define VBAN_TASK_STACK         4096
#define VBAN_TASK_PRIORITY      4

/* VBAN header (packed, compliant, with frame counter) */
struct __attribute__((packed)) VBANHeader {
  char     vban[4];          // "VBAN"
  uint8_t  format_sr;        // sample-rate index
  uint8_t  format_nbs;       // samples per frame - 1
  uint8_t  format_nbc;       // channels - 1
  uint8_t  format_format;    // format / codec
  char     streamname[16];   // null padded
  uint32_t frame_counter;    // REQUIRED
};

struct AudioPacket {
  int16_t samples[VBAN_SAMPLES_PER_PACKET];
};

class VBANAudio : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  /* API used by codegen */
  void set_microphone(microphone::Microphone *mic);
  void set_target_ip(const std::string &ip);
  void set_target_port(uint16_t port);
  void set_sample_rate(uint32_t rate);
  void set_stream_name(const std::string &name);
  void set_gain(float gain);

 protected:
  static void tx_task_(void *arg);
  void tx_loop_();

  void microphone_bytes_callback_(const std::vector<uint8_t> &data);
  void push_samples_(const int16_t *samples, size_t count);

  uint8_t vban_sr_index_(uint32_t rate);

  microphone::Microphone *microphone_{nullptr};

  QueueHandle_t sample_queue_{nullptr};
  TaskHandle_t tx_task_handle_{nullptr};
  bool task_started_{false};

  int sock_{-1};
  sockaddr_in dest_addr_{};

  std::string target_ip_str_;
  uint16_t port_{6980};
  uint32_t sample_rate_{48000};
  std::string stream_name_{"ESP32"};

  float gain_{1.0f};  // digital mic gain
};

}  // namespace vban_audio
}  // namespace esphome
