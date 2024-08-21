#include "pm1006.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pm1006 {

static const char *const TAG = "pm1006";

static const uint8_t PM2105_RESPONSE_HEADER[] = {0x16, 0x35, 0x0B};
static const uint8_t PM2105_DATA_LENGTH = 52;
static const uint8_t PM2105_REQUEST[] = {0x11, 0x02, 0x0b, 0x07, 0xdb};

void PM2105Component::setup() {
  // because this implementation is currently rx-only, there is nothing to setup
}

void PM2105Component::dump_config() {
  ESP_LOGCONFIG(TAG, "PM2105:");
  LOG_SENSOR("  ", "PM2.5", this->pm_2_5_sensor_);
  LOG_UPDATE_INTERVAL(this);
  this->check_uart_settings(9600);
}

void PM2105Component::update() {
  ESP_LOGV(TAG, "sending measurement request");
  this->write_array(PM2105_REQUEST, sizeof(PM2105_REQUEST));
}

void PM2105Component::loop() {
  while (this->available() != 0) {
    this->read_byte(&this->data_[this->data_index_]);
    uint8_t check = this->check_byte_();
    if (check == -1) {
      // Error in data.
      ESP_LOGV(TAG, "Byte %i of received data frame is invalid.", this->data_index_);
      this->data_index_ = 0;
    } else if (check == 0) {
      // Continue reading bytes.
      data_index_++;
    } else if (check == 1) {
      this->parse_data_();
      this->data_index_ = 0;
    }
  }
}

float PM2105Component::get_setup_priority() const { return setup_priority::DATA; }

uint8_t PM2105Component::pm2105_checksum_(const uint8_t *command_data, uint8_t length) const {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < length; i++) {
    sum += command_data[i];
  }
  return sum;
}

int8_t PM2105Component::check_byte_() const {
  const uint8_t index = this->data_index_;
  const uint8_t byte = this->data_[index];

  // Fixed header bytes.
  if (index < sizeof(PM2105_RESPONSE_HEADER)) {
    if (byte != PM2105_RESPONSE_HEADER[index]) {
      return -1;
    }
  }

  // Data within expected length.
  if (index < (sizeof(PM2105_RESPONSE_HEADER) + PM2105_DATA_LENGTH)) {
    return 0;
  }

  // Verify checksum byte.
  if (index == (sizeof(PM2105_RESPONSE_HEADER) + PM2105_DATA_LENGTH)) {
    uint8_t checksum = (pm2105_checksum_(this->data_, this->data_index_ + 1));
    if (checksum != 0) {
      return -1;
    }
    return 1;
  }

  // Message has exceeded expected size.
  return -1;
}

void PM2105Component::parse_data_() {
  const int pm_2_5_concentration = this->get_16_bit_uint_(5);

  ESP_LOGD(TAG, "Got PM2.5 Concentration: %d µg/m³", pm_2_5_concentration);

  if (this->pm_2_5_sensor_ != nullptr) {
    this->pm_2_5_sensor_->publish_state(pm_2_5_concentration);
  }
}

uint32_t PM2105Component::get_32_bit_uint_(uint8_t start_index) const {
  return encode_uint32(this->data_[start_index], this->data_[start_index + 1], this->data_[start_index + 2],
                       this->data_[start_index + 3]);
}

}  // namespace pm1006
}  // namespace esphome
