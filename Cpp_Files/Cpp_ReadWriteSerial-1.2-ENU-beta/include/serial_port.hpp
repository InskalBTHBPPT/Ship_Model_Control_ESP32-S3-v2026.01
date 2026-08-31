#pragma once

#include <cstdint>
#include <string>

class SerialPort {
public:
  SerialPort() = default;
  ~SerialPort();

  SerialPort(const SerialPort &) = delete;
  SerialPort &operator=(const SerialPort &) = delete;

  bool open(const std::string &port_name, uint32_t baud_rate);
  void close();
  bool is_open() const;

  bool read_line(std::string &line, uint32_t timeout_ms = 1000);
  bool write_line(const std::string &line);
  std::string last_error() const { return last_error_; }

private:
  bool configure(uint32_t baud_rate);
  bool wait_for_data(uint32_t timeout_ms);

#ifdef _WIN32
  void *handle_ = reinterpret_cast<void *>(-1);
#else
  int fd_ = -1;
#endif

  std::string read_buffer_;
  std::string last_error_;
};
