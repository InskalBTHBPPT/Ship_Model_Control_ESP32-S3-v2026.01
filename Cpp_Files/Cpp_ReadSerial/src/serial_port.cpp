#include "serial_port.hpp"

#include <chrono>
#include <cstring>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

SerialPort::~SerialPort() { close(); }

bool SerialPort::is_open() const {
#ifdef _WIN32
  return handle_ != reinterpret_cast<void *>(-1);
#else
  return fd_ >= 0;
#endif
}

void SerialPort::close() {
#ifdef _WIN32
  if (is_open()) {
    CloseHandle(static_cast<HANDLE>(handle_));
    handle_ = reinterpret_cast<void *>(-1);
  }
#else
  if (is_open()) {
    ::close(fd_);
    fd_ = -1;
  }
#endif
  read_buffer_.clear();
}

bool SerialPort::open(const std::string &port_name, uint32_t baud_rate) {
  close();
  last_error_.clear();

#ifdef _WIN32
  std::string device = port_name;
  if (device.rfind("\\\\.\\", 0) != 0) {
    device = "\\\\.\\" + device;
  }

  HANDLE handle = CreateFileA(
      device.c_str(),
      GENERIC_READ | GENERIC_WRITE,
      0,
      nullptr,
      OPEN_EXISTING,
      0,
      nullptr);

  if (handle == INVALID_HANDLE_VALUE) {
    last_error_ = "CreateFile gagal untuk " + port_name;
    return false;
  }

  handle_ = handle;
  if (!configure(baud_rate)) {
    close();
    return false;
  }
  return true;
#else
  fd_ = ::open(port_name.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd_ < 0) {
    last_error_ = "open gagal untuk " + port_name + ": " + std::strerror(errno);
    return false;
  }

  if (!configure(baud_rate)) {
    close();
    return false;
  }
  return true;
#endif
}

bool SerialPort::configure(uint32_t baud_rate) {
#ifdef _WIN32
  DCB dcb{};
  dcb.DCBlength = sizeof(DCB);
  HANDLE handle = static_cast<HANDLE>(handle_);

  if (!GetCommState(handle, &dcb)) {
    last_error_ = "GetCommState gagal";
    return false;
  }

  dcb.BaudRate = baud_rate;
  dcb.ByteSize = 8;
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT;
  dcb.fBinary = TRUE;
  dcb.fParity = FALSE;
  dcb.fOutxCtsFlow = FALSE;
  dcb.fOutxDsrFlow = FALSE;
  dcb.fDtrControl = DTR_CONTROL_ENABLE;
  dcb.fRtsControl = RTS_CONTROL_ENABLE;
  dcb.fOutX = FALSE;
  dcb.fInX = FALSE;

  if (!SetCommState(handle, &dcb)) {
    last_error_ = "SetCommState gagal";
    return false;
  }

  COMMTIMEOUTS timeouts{};
  timeouts.ReadIntervalTimeout = MAXDWORD;
  timeouts.ReadTotalTimeoutMultiplier = 0;
  timeouts.ReadTotalTimeoutConstant = 0;
  timeouts.WriteTotalTimeoutMultiplier = 0;
  timeouts.WriteTotalTimeoutConstant = 0;

  if (!SetCommTimeouts(handle, &timeouts)) {
    last_error_ = "SetCommTimeouts gagal";
    return false;
  }

  PurgeComm(handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
  return true;
#else
  termios tty{};
  if (tcgetattr(fd_, &tty) != 0) {
    last_error_ = "tcgetattr gagal: " + std::string(std::strerror(errno));
    return false;
  }

  speed_t speed = B115200;
  switch (baud_rate) {
    case 9600: speed = B9600; break;
    case 19200: speed = B19200; break;
    case 38400: speed = B38400; break;
    case 57600: speed = B57600; break;
    case 115200: speed = B115200; break;
    case 230400: speed = B230400; break;
    case 460800: speed = B460800; break;
    case 921600: speed = B921600; break;
    default:
      last_error_ = "Baud rate tidak didukung";
      return false;
  }

  cfsetispeed(&tty, speed);
  cfsetospeed(&tty, speed);

  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;
  tty.c_cflag |= CREAD | CLOCAL;
  tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
  tty.c_oflag &= ~OPOST;
  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 0;

  if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
    last_error_ = "tcsetattr gagal: " + std::string(std::strerror(errno));
    return false;
  }

  tcflush(fd_, TCIOFLUSH);
  return true;
#endif
}

bool SerialPort::wait_for_data(uint32_t timeout_ms) {
#ifdef _WIN32
  (void)timeout_ms;
  return true;
#else
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(fd_, &readfds);

  timeval tv{};
  tv.tv_sec = static_cast<time_t>(timeout_ms / 1000);
  tv.tv_usec = static_cast<suseconds_t>((timeout_ms % 1000) * 1000);

  int result = select(fd_ + 1, &readfds, nullptr, nullptr, &tv);
  if (result < 0) {
    last_error_ = "select gagal: " + std::string(std::strerror(errno));
    return false;
  }
  return result > 0;
#endif
}

bool SerialPort::read_line(std::string &line, uint32_t timeout_ms) {
  line.clear();
  if (!is_open()) {
    last_error_ = "Port serial belum dibuka";
    return false;
  }

  const auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(timeout_ms);

  while (true) {
    const auto newline_pos = read_buffer_.find('\n');
    if (newline_pos != std::string::npos) {
      line = read_buffer_.substr(0, newline_pos);
      read_buffer_.erase(0, newline_pos + 1);
      return true;
    }

    if (std::chrono::steady_clock::now() >= deadline) {
      return false;
    }

#ifdef _WIN32
    char buffer[256];
    DWORD bytes_read = 0;
    HANDLE handle = static_cast<HANDLE>(handle_);

    if (!ReadFile(handle, buffer, sizeof(buffer), &bytes_read, nullptr)) {
      last_error_ = "ReadFile gagal";
      return false;
    }

    if (bytes_read == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      continue;
    }

    read_buffer_.append(buffer, bytes_read);
#else
    if (!wait_for_data(50)) {
      continue;
    }

    char buffer[256];
    const ssize_t bytes_read = ::read(fd_, buffer, sizeof(buffer));
    if (bytes_read < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }
      last_error_ = "read gagal: " + std::string(std::strerror(errno));
      return false;
    }
    if (bytes_read == 0) {
      continue;
    }

    read_buffer_.append(buffer, static_cast<size_t>(bytes_read));
#endif
  }
}

bool SerialPort::write_line(const std::string &line) {
  if (!is_open()) {
    last_error_ = "Port serial belum dibuka";
    return false;
  }

  const std::string payload = line + "\n";

#ifdef _WIN32
  DWORD bytes_written = 0;
  HANDLE handle = static_cast<HANDLE>(handle_);
  if (!WriteFile(
          handle,
          payload.data(),
          static_cast<DWORD>(payload.size()),
          &bytes_written,
          nullptr) ||
      bytes_written != payload.size()) {
    last_error_ = "WriteFile gagal";
    return false;
  }
  return true;
#else
  const ssize_t bytes_written =
      ::write(fd_, payload.data(), payload.size());
  if (bytes_written < 0) {
    last_error_ = "write gagal: " + std::string(std::strerror(errno));
    return false;
  }
  if (static_cast<size_t>(bytes_written) != payload.size()) {
    last_error_ = "write tidak lengkap";
    return false;
  }
  return true;
#endif
}
