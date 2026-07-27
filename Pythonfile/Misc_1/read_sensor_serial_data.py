import serial
import time
import sys
from datetime import datetime

# Konfigurasi serial port (default)
PORT = 'COM14'  # ganti via argumen jika perlu
BAUD_RATE = 115200
TIMEOUT = 1
SAVE_CSV = False
CSV_PATH = f"serial_csv_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

def read_sensor_data():
    try:
        ser = serial.Serial(PORT, BAUD_RATE, timeout=TIMEOUT)
        print(f"Serial port {PORT} dibuka dengan baud rate {BAUD_RATE}")
        time.sleep(2)  # Tunggu ESP32 siap
        
        # Flush buffer untuk menghapus data lama
        ser.reset_input_buffer()
        print("Buffer serial di-clear. Menunggu data baru...")
        time.sleep(1)
        
        csv_file = None
        if SAVE_CSV:
            csv_file = open(CSV_PATH, 'w', buffering=1)
            header = (
                "timestamp,latitude,longitude,speedMps,Calc_deg_servo_1,Calc_deg_servo_2,"
                "roll,pitch,yaw,rpm_prop_1,rpm_prop_2,battery_1,battery_2\n"
            )
            csv_file.write(header)

        while True:
            line = ser.readline()
            if not line:
                time.sleep(0.001)
                continue

            try:
                text = line.decode('utf-8', errors='replace').strip()
                if not text:
                    continue
                parts = [p.strip() for p in text.split(',')]
                if len(parts) != 13:
                    print(f"[WARN] Kolom != 13: '{text}'")
                    continue

                # Cast ke float untuk validasi
                values = [float(p) for p in parts]
                # Cetak ringkas
                print(text)
                # Simpan CSV jika diaktifkan
                if SAVE_CSV and csv_file:
                    csv_file.write(text + "\n")
            except Exception as e:
                print(f"[ERR] Gagal parse: {e}")
                continue
            
            time.sleep(0.001)
    
    except serial.SerialException as e:
        print(f"Error serial: {e}")
    finally:
        try:
            if 'ser' in locals() and ser.is_open:
                ser.close()
        except Exception:
            pass
        try:
            if 'csv_file' in locals() and csv_file:
                csv_file.close()
        except Exception:
            pass
        print("Serial port ditutup")

if __name__ == '__main__':
    # Override PORT/BAUD via argumen opsional: python read_sensor_data.py [PORT] [BAUD]
    if len(sys.argv) >= 2:
        PORT = sys.argv[1]
    if len(sys.argv) >= 3:
        try:
            BAUD_RATE = int(sys.argv[2])
        except ValueError:
            pass
    # Aktifkan simpan CSV dengan argumen ke-4 bernilai 'save'
    if len(sys.argv) >= 4 and sys.argv[3].lower() == 'save':
        SAVE_CSV = True
    read_sensor_data()
