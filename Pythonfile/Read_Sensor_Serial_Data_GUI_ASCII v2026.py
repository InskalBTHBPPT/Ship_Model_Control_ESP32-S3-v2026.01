import serial
import time
import sys
import os
from datetime import datetime
from serial.tools import list_ports
from PySide6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QTextEdit, QComboBox, QPushButton, QLabel, QSplitter, QLineEdit, QPlainTextEdit, QSpinBox
)
from PySide6.QtCore import Qt, QThread, Signal


class SerialReaderThread(QThread):
    """
    Thread untuk membaca data serial tanpa blocking GUI.
    Mode: Passthrough - Data mentah dari serial langsung disimpan ke CSV dan ditampilkan tanpa parsing.
    """
    data_received = Signal(str)
    error_occurred = Signal(str)
    
    def __init__(self, port, baud_rate, csv_path, csv_header=""):
        super().__init__()
        self.port = port
        self.baud_rate = baud_rate
        self.csv_path = csv_path
        self.csv_header = csv_header  # Header CSV dari input text
        self.ser = None
        self.csv_file = None
        self.running = False
        self.log_buffer = []  # Buffer untuk menyimpan data sebelum ditulis ke CSV
        self.last_flush_time = 0  # Waktu terakhir flush (untuk time-based flush)
        self.text_buffer = ''  # Buffer untuk ASCII text data (untuk menangani data yang terpotong)
        
    def run(self):
        """Main loop untuk membaca serial data"""
        try:
            self.ser = serial.Serial(self.port, self.baud_rate, timeout=1)
            self.data_received.emit(f"Serial port {self.port} dibuka dengan baud rate {self.baud_rate}\n")
            time.sleep(2)  # Tunggu ESP32 siap
            
            # Flush buffer
            self.ser.reset_input_buffer()
            self.data_received.emit("Buffer serial di-clear. Menunggu data baru...\n")
            
            # Print CSV header ke serial monitor
            if self.csv_header:
                self.data_received.emit(f"CSV Header: {self.csv_header}\n")
            
            time.sleep(1)
            
            # Buka file CSV
            self.csv_file = open(self.csv_path, 'w', buffering=1)
            
            # Tulis CSV header jika ada
            if self.csv_header:
                header_line = self.csv_header.strip()
                if not header_line.endswith('\n'):
                    header_line += '\n'
                self.csv_file.write(header_line)
            
            # Reset buffer
            self.log_buffer.clear()
            self.text_buffer = ''
            self.last_flush_time = time.time()  # Inisialisasi waktu flush
            
            self.running = True
            while self.running:
                if not self.ser or not self.ser.is_open:
                    break
                
                # Baca ASCII text data dari serial
                available = self.ser.in_waiting
                if available > 0:
                    chunk = self.ser.read(available).decode('utf-8', errors='ignore')
                    self.text_buffer += chunk
                    
                    # Proses semua baris lengkap yang ada di buffer (dipisahkan newline)
                    while '\n' in self.text_buffer:
                        # Ambil satu baris (sampai newline)
                        line_end = self.text_buffer.index('\n')
                        line = self.text_buffer[:line_end].rstrip('\r\n')  # Hapus trailing newline/carriage return
                        self.text_buffer = self.text_buffer[line_end + 1:]
                        
                        # Skip baris kosong
                        if not line:
                            continue
                        
                        # Passthrough: langsung simpan dan tampilkan data mentah tanpa parsing
                        try:
                            # Tampilkan data mentah ke GUI
                            self.data_received.emit(line + "\n")
                            
                            # Simpan data mentah ke buffer (batch writing untuk mengurangi I/O overhead)
                            if self.csv_file:
                                self.log_buffer.append(line + "\n")
                                
                                # Flush buffer setiap 400ms (time-based, seperti Local Monitor Dashboard)
                                current_time = time.time()
                                if (current_time - self.last_flush_time) >= 0.4:  # 400ms = 0.4 detik
                                    self.flush_log_buffer()
                                    self.last_flush_time = current_time
                                
                        except Exception as e:
                            self.error_occurred.emit(f"[ERR] Error processing data: {e}\n")
                            continue
                else:
                    time.sleep(0.001)
                
        except serial.SerialException as e:
            self.error_occurred.emit(f"Error serial: {e}\n")
        except Exception as e:
            self.error_occurred.emit(f"Error: {e}\n")
        finally:
            self.cleanup()
    
    def stop(self):
        """Stop reading dan cleanup"""
        self.running = False
        # cleanup() akan dipanggil dari finally block di run()
        # Timer akan dihentikan di cleanup() yang dipanggil dari dalam thread
    
    def flush_log_buffer(self):
        """
        Flush log buffer ke file CSV.
        
        Method ini:
        - Menulis semua data dalam buffer ke file
        - Clear buffer setelah menulis
        - Mengurangi I/O overhead dengan batch writing
        """
        if not self.csv_file or not self.log_buffer:
            return
        try:
            # Write and clear in batch to reduce IO overhead
            self.csv_file.writelines(self.log_buffer)
            self.log_buffer.clear()
        except Exception:
            pass
    
    def cleanup(self):
        """Cleanup resources - dipanggil dari dalam thread"""
        # Flush buffer terakhir sebelum menutup file
        try:
            if self.csv_file:
                self.flush_log_buffer()  # Pastikan semua data tersimpan
        except Exception:
            pass
        
        # Close CSV file
        try:
            if self.csv_file:
                self.csv_file.close()
                self.csv_file = None
                self.data_received.emit(f"CSV disimpan ke: {self.csv_path}\n")
        except Exception as e:
            self.error_occurred.emit(f"[ERR] Error menutup CSV: {e}\n")
        
        # Close serial port dengan error handling yang lebih baik
        try:
            if self.ser is not None:
                # Cek apakah port masih terbuka sebelum menutup
                try:
                    if hasattr(self.ser, 'is_open') and self.ser.is_open:
                        self.ser.close()
                except (AttributeError, OSError, ValueError):
                    # Port mungkin sudah ditutup atau error, abaikan
                    pass
                self.ser = None
                self.data_received.emit("Serial port ditutup\n")
        except Exception as e:
            # Jangan emit error jika port sudah None atau sudah ditutup
            if self.ser is not None:
                self.error_occurred.emit(f"[ERR] Error menutup serial: {e}\n")
            self.ser = None


class SerialMonitorGUI(QMainWindow):
    """GUI untuk monitoring serial data dengan PySide6"""
    
    def __init__(self):
        super().__init__()
        self.serial_thread = None
        self.csv_path = None
        self.setup_ui()
        self.auto_detect_ports()
        
    def setup_ui(self):
        """Setup user interface"""
        self.setWindowTitle("Serial Sensor Data Monitor")
        self.setGeometry(100, 100, 1200, 700)
        
        # Central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Main horizontal layout
        main_layout = QHBoxLayout(central_widget)
        main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.setSpacing(5)
        
        # Splitter untuk membagi window dengan rasio 5:1
        splitter = QSplitter(Qt.Horizontal)
        
        # ========== LEFT PANEL (5 bagian) ==========
        left_panel = QWidget()
        left_layout = QVBoxLayout(left_panel)
        left_layout.setContentsMargins(5, 5, 5, 5)
        
        # Label untuk data display
        data_label = QLabel("Data Serial:")
        data_label.setStyleSheet("font-weight: bold; font-size: 12pt;")
        left_layout.addWidget(data_label)
        
        # QTextEdit untuk menampilkan data (bisa scroll)
        self.data_display = QTextEdit()
        self.data_display.setReadOnly(True)
        self.data_display.setFontFamily("Courier")
        self.data_display.setFontPointSize(9)
        left_layout.addWidget(self.data_display)
        
        splitter.addWidget(left_panel)
        
        # ========== RIGHT PANEL (1 bagian) ==========
        right_panel = QWidget()
        right_panel.setMaximumWidth(250)
        right_layout = QVBoxLayout(right_panel)
        right_layout.setContentsMargins(5, 5, 5, 5)
        right_layout.setSpacing(10)
        
        # Title
        title_label = QLabel("Kontrol Serial")
        title_label.setStyleSheet("font-weight: bold; font-size: 11pt;")
        right_layout.addWidget(title_label)
        
        # COM Port section
        com_label = QLabel("COM Port:")
        right_layout.addWidget(com_label)
        
        self.com_port_combo = QComboBox()
        self.com_port_combo.setEditable(True)
        right_layout.addWidget(self.com_port_combo)
        
        # Baudrate section
        baud_label = QLabel("Baudrate:")
        right_layout.addWidget(baud_label)
        
        self.baudrate_combo = QComboBox()
        self.baudrate_combo.addItems(["9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"])
        self.baudrate_combo.setCurrentText("230400")  # Default increased to support 200 data/sec
        right_layout.addWidget(self.baudrate_combo)
        
        # Auto Detect button
        self.auto_detect_btn = QPushButton("Auto Detect")
        self.auto_detect_btn.clicked.connect(self.auto_detect_ports)
        right_layout.addWidget(self.auto_detect_btn)
        
        # Jumlah Kolom input section
        num_columns_label = QLabel("Jumlah Kolom:")
        right_layout.addWidget(num_columns_label)
        
        self.num_columns_spinbox = QSpinBox()
        self.num_columns_spinbox.setMinimum(1)
        self.num_columns_spinbox.setMaximum(100)
        self.num_columns_spinbox.setValue(15)  # Default value
        right_layout.addWidget(self.num_columns_spinbox)
        
        # CSV Header input section
        csv_header_label = QLabel("CSV Header:")
        right_layout.addWidget(csv_header_label)
        
        # Gunakan QPlainTextEdit untuk support text wrapping (bisa multi-line)
        self.csv_header_input = QPlainTextEdit()
        # Default value dari comment di line 52-53
        default_header = "timestamp,latitude,longitude,speedMps,Calc_deg_servo_1,Calc_deg_servo_2,roll,pitch,yaw,zigzag_yaw,rpm_prop_1,rpm_prop_2,battery_1,battery_2,mode_auto"
        self.csv_header_input.setPlainText(default_header)
        self.csv_header_input.setPlaceholderText("Masukkan header CSV (dipisahkan koma)")
        self.csv_header_input.setFixedHeight(80)  # Set tinggi input text dengan wrapping
        self.csv_header_input.setLineWrapMode(QPlainTextEdit.WidgetWidth)  # Enable word wrap
        right_layout.addWidget(self.csv_header_input)
        
        # Connect/Disconnect button
        self.connect_btn = QPushButton("Connect")
        self.connect_btn.setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;")
        self.connect_btn.clicked.connect(self.toggle_connection)
        right_layout.addWidget(self.connect_btn)
        
        # Spacer untuk push semua ke atas
        right_layout.addStretch()
        
        splitter.addWidget(right_panel)
        
        # Set splitter ratio 5:1
        splitter.setSizes([1000, 200])
        
        main_layout.addWidget(splitter)
        
    def auto_detect_ports(self):
        """Auto detect COM ports yang tersedia"""
        self.com_port_combo.clear()
        ports = list_ports.comports()
        
        if ports:
            for port in ports:
                port_str = f"{port.device} - {port.description}"
                self.com_port_combo.addItem(port_str, port.device)
        else:
            self.com_port_combo.addItem("Tidak ada port ditemukan")
        
        # Set default ke item pertama jika ada
        if self.com_port_combo.count() > 0:
            self.com_port_combo.setCurrentIndex(0)
    
    def get_selected_port(self):
        """Ambil port yang dipilih dari combo box"""
        if self.com_port_combo.count() == 0:
            return None
        
        current_data = self.com_port_combo.currentData()
        if current_data:
            return current_data
        
        # Jika tidak ada data, coba parse dari text
        current_text = self.com_port_combo.currentText()
        if " - " in current_text:
            return current_text.split(" - ")[0]
        return current_text
    
    def toggle_connection(self):
        """Toggle connect/disconnect"""
        if self.serial_thread and self.serial_thread.isRunning():
            # Disconnect
            self.disconnect_serial()
        else:
            # Connect
            self.connect_serial()
    
    def connect_serial(self):
        """Connect ke serial port dan mulai membaca data"""
        # Clear display sebelum koneksi baru
        self.data_display.clear()
        
        port = self.get_selected_port()
        if not port:
            self.data_display.append("[ERROR] Port tidak dipilih!\n")
            return
        
        try:
            baud_rate = int(self.baudrate_combo.currentText())
        except ValueError:
            self.data_display.append("[ERROR] Baudrate tidak valid!\n")
            return
        
        # Generate CSV path di folder LogData
        script_dir = os.path.dirname(os.path.abspath(__file__))
        log_dir = os.path.join(script_dir, "LogData")
        
        # Buat folder LogData jika belum ada
        if not os.path.exists(log_dir):
            os.makedirs(log_dir)
        
        csv_filename = f"serial_csv_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        self.csv_path = os.path.join(log_dir, csv_filename)
        
        # Ambil CSV header dari input text (QPlainTextEdit menggunakan toPlainText())
        csv_header = self.csv_header_input.toPlainText().strip()
        
        # Buat thread untuk membaca serial (passthrough mode - tidak perlu num_columns)
        self.serial_thread = SerialReaderThread(port, baud_rate, self.csv_path, csv_header)
        self.serial_thread.data_received.connect(self.append_data)
        self.serial_thread.error_occurred.connect(self.append_error)
        self.serial_thread.finished.connect(self.on_thread_finished)
        
        # Update UI
        self.connect_btn.setText("Disconnect")
        self.connect_btn.setStyleSheet("background-color: #f44336; color: white; font-weight: bold;")
        self.com_port_combo.setEnabled(False)
        self.baudrate_combo.setEnabled(False)
        self.auto_detect_btn.setEnabled(False)
        self.num_columns_spinbox.setEnabled(False)  # Disable jumlah kolom input saat connect
        self.csv_header_input.setEnabled(False)  # Disable CSV header input saat connect
        
        # Start thread
        self.serial_thread.start()
        self.data_display.append(f"[INFO] Menghubungkan ke {port} dengan baudrate {baud_rate}...\n")
    
    def disconnect_serial(self):
        """Disconnect dari serial port"""
        if self.serial_thread and self.serial_thread.isRunning():
            self.serial_thread.stop()
            self.serial_thread.wait(3000)  # Wait max 3 seconds
        
        # Update UI
        self.connect_btn.setText("Connect")
        self.connect_btn.setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;")
        self.com_port_combo.setEnabled(True)
        self.baudrate_combo.setEnabled(True)
        self.auto_detect_btn.setEnabled(True)
        self.num_columns_spinbox.setEnabled(True)  # Enable jumlah kolom input saat disconnect
        self.csv_header_input.setEnabled(True)  # Enable CSV header input saat disconnect
        
        self.data_display.append("[INFO] Disconnected\n")
    
    def append_data(self, text):
        """Append data ke text display"""
        self.data_display.append(text)
        # Auto scroll ke bawah
        scrollbar = self.data_display.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())
    
    def append_error(self, text):
        """Append error ke text display"""
        self.data_display.append(f"<span style='color: red;'>{text}</span>")
        scrollbar = self.data_display.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())
    
    def on_thread_finished(self):
        """Callback ketika thread selesai"""
        # Hanya update UI, jangan panggil disconnect_serial lagi karena cleanup sudah dilakukan di thread
        self.connect_btn.setText("Connect")
        self.connect_btn.setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;")
        self.com_port_combo.setEnabled(True)
        self.baudrate_combo.setEnabled(True)
        self.auto_detect_btn.setEnabled(True)
        self.num_columns_spinbox.setEnabled(True)
    
    def closeEvent(self, event):
        """Handle window close event"""
        if self.serial_thread and self.serial_thread.isRunning():
            self.disconnect_serial()
        event.accept()


if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = SerialMonitorGUI()
    window.show()
    sys.exit(app.exec())
