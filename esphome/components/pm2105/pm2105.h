#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace pm2105 {

class PM2105Component : public PollingComponent, public uart::UARTDevice {
 public:
  PM2105Component() = default;

  void setup() override;
  void dump_config() override;
  void loop() override;
  void update() override;

  float get_setup_priority() const override;

 protected:
  // Returns -1 if an error has been detected, 0 if check has passed, 1 if a packet has been received.
  int8_t check_byte_() const;
  void parse_data_();
  uint32_t get_32_bit_uint_(uint8_t start_index) const;
  uint8_t pm2105_checksum_(const uint8_t *command_data, uint8_t length) const;

  sensor::Sensor *pm_2_5_sensor_{nullptr};

  uint8_t data_[64];
  uint8_t data_index_{0};
};

}  // namespace pm2105
}  // namespace esphome
