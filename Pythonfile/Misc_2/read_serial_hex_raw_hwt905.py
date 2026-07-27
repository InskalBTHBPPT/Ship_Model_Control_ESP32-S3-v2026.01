
"""
Script untuk membaca data hex dari serial port
Default: COM14, Baudrate: 115200

Magnetometer Resolution:
- Resolusi hardware: 13 nT/LSB (nanoTesla per LSB)
- Konversi unit: 13 nT/LSB = 0.013 µT/LSB (microTesla per LSB)
- Faktor skala Witmotion: 77
- Hubungan: 1/77 ≈ 0.013 µT/LSB = 13 nT/LSB
- Konversi: Witmotion Value (µT) = Raw LSB / 77
           atau: Raw LSB × 0.013 µT/LSB = Witmotion Value (µT)

Catatan: Output Witmotion dan Python menggunakan unit µT (microTesla)
"""

import serial
import sys
import argparse
from datetime import datetime
from collections import deque
import struct

# Konstanta untuk package parsing
PACKAGE_SIZE = 66  # 6 grup × 11 bytes
GROUP_SIZE = 11    # Bytes per grup
START_BYTE = 0x55  # Start byte untuk setiap grup
START_GROUP_BYTE = 0x50  # Byte kedua grup pertama (0x55, 0x50)
END_GROUP_BYTE = 0x59    # Byte kedua grup terakhir (0x55, 0x59)

def parse_package(package_data):
    """
    Parse package data menjadi 6 grup
    
    Args:
        package_data: bytes dengan panjang 66 bytes
        
    Returns:
        list: List of 6 groups, setiap grup adalah list of 11 bytes
    """
    if len(package_data) != PACKAGE_SIZE:
        return None
    
    groups = []
    for i in range(6):
        start_idx = i * GROUP_SIZE
        end_idx = start_idx + GROUP_SIZE
        group = list(package_data[start_idx:end_idx])
        groups.append(group)
    
    return groups

def calculate_checksum(group):
    """
    Menghitung checksum untuk grup berdasarkan format HWT905
    
    Args:
        group: List of 11 bytes
        
    Returns:
        tuple: (calculated_checksum, received_checksum, is_valid)
    """
    if len(group) != 11:
        return (0, 0, False)
    
    group_id = group[1]
    received_sum = group[10]
    
    # Hitung checksum berdasarkan format dokumentasi HWT905
    if group_id == 0x50:  # Time
        # Sum = 0x55 + 0x50 + YY + MM + DD + hh + mm + ss + msL + msH
        calculated_sum = (0x55 + 0x50 + group[2] + group[3] + group[4] + 
                         group[5] + group[6] + group[7] + group[8] + group[9]) & 0xFF
    elif group_id == 0x51:  # Acceleration
        # Sum = 0x55 + 0x51 + AxH + AxL + AyH + AyL + AzH + AzL + TH + TL
        calculated_sum = (0x55 + 0x51 + group[3] + group[2] + group[5] + group[4] + 
                         group[7] + group[6] + group[9] + group[8]) & 0xFF
    elif group_id == 0x52:  # Angular Velocity (Gyro)
        # Sum = 0x55 + 0x52 + wxH + wxL + wyH + wyL + wzH + wzL + TH + TL
        calculated_sum = (0x55 + 0x52 + group[3] + group[2] + group[5] + group[4] + 
                         group[7] + group[6] + group[9] + group[8]) & 0xFF
    elif group_id == 0x53:  # Angle
        # Sum = 0x55 + 0x53 + RollH + RollL + PitchH + PitchL + YawH + YawL + VH + VL
        calculated_sum = (0x55 + 0x53 + group[3] + group[2] + group[5] + group[4] + 
                         group[7] + group[6] + group[9] + group[8]) & 0xFF
    elif group_id == 0x54:  # Magnetic
        # Sum = 0x55 + 0x54 + HxH + HxL + HyH + HyL + HzH + HzL + TH + TL
        calculated_sum = (0x55 + 0x54 + group[3] + group[2] + group[5] + group[4] + 
                         group[7] + group[6] + group[9] + group[8]) & 0xFF
    elif group_id == 0x59:  # Quaternion
        # Sum = 0x55 + 0x59 + Q0L + Q0H + Q1L + Q1H + Q2L + Q2H + Q3L + Q3H
        calculated_sum = (0x55 + 0x59 + group[2] + group[3] + group[4] + group[5] + 
                         group[6] + group[7] + group[8] + group[9]) & 0xFF
    else:
        # Unknown group type
        calculated_sum = 0
    
    is_valid = (calculated_sum == received_sum)
    return (calculated_sum, received_sum, is_valid)

def convert_group_data(group):
    """
    Konversi data grup dari hex ke nilai real berdasarkan format HWT905 RS232 Datasheet
    
    Args:
        group: List of 11 bytes (format: 0x55, ID, data[8 bytes], checksum)
        
    Returns:
        dict: Dictionary berisi data yang sudah dikonversi, atau None jika gagal
    """
    if len(group) != 11:
        return None
    
    group_id = group[1]
    result = {'group_id': f'0x{group_id:02X}'}
    
    # Validasi checksum
    calc_sum, recv_sum, is_valid = calculate_checksum(group)
    result['checksum_valid'] = is_valid
    result['checksum'] = {'calculated': calc_sum, 'received': recv_sum}
    
    try:
        if group_id == 0x50:  # Time
            # Format: 0x55 0x50 YY MM DD hh mm ss msL msH SUM
            # ms = (msH << 8) | msL
            result['type'] = 'Time'
            result['year'] = group[2] + 2000
            result['month'] = group[3]
            result['day'] = group[4]
            result['hour'] = group[5]
            result['minute'] = group[6]
            result['second'] = group[7]
            result['millisecond'] = (group[9] << 8) | group[8]
            result['formatted'] = f"{result['year']:04d}-{result['month']:02d}-{result['day']:02d} {result['hour']:02d}:{result['minute']:02d}:{result['second']:02d}.{result['millisecond']:03d}"
            
        elif group_id == 0x51:  # Acceleration
            # Format: 0x55 0x51 AxL AxH AyL AyH AzL AzH TL TH SUM
            # ax = ((AxH << 8) | AxL) / 32768 * 16 (g)
            # T = ((TH << 8) | TL) / 100 (°C)
            result['type'] = 'Acceleration'
            ax_raw = (group[3] << 8) | group[2]
            ay_raw = (group[5] << 8) | group[4]
            az_raw = (group[7] << 8) | group[6]
            temp_raw = (group[9] << 8) | group[8]
            
            # Konversi ke signed int16
            if ax_raw > 32767:
                ax_raw = ax_raw - 65536
            if ay_raw > 32767:
                ay_raw = ay_raw - 65536
            if az_raw > 32767:
                az_raw = az_raw - 65536
            
            result['accel_x'] = ax_raw / 32768.0 * 16.0  # g
            result['accel_y'] = ay_raw / 32768.0 * 16.0  # g
            result['accel_z'] = az_raw / 32768.0 * 16.0  # g
            result['temperature'] = temp_raw / 100.0  # °C
            result['formatted'] = f"X: {result['accel_x']:.3f} g, Y: {result['accel_y']:.3f} g, Z: {result['accel_z']:.3f} g, T: {result['temperature']:.2f}°C"
            
        elif group_id == 0x52:  # Angular Velocity (Gyroscope)
            # Format: 0x55 0x52 wxL wxH wyL wyH wzL wzH TL TH SUM
            # wx = ((wxH << 8) | wxL) / 32768 * 2000 (°/s)
            # T = ((TH << 8) | TL) / 100 (°C)
            result['type'] = 'Angular Velocity (Gyro)'
            wx_raw = (group[3] << 8) | group[2]
            wy_raw = (group[5] << 8) | group[4]
            wz_raw = (group[7] << 8) | group[6]
            temp_raw = (group[9] << 8) | group[8]
            
            # Konversi ke signed int16
            if wx_raw > 32767:
                wx_raw = wx_raw - 65536
            if wy_raw > 32767:
                wy_raw = wy_raw - 65536
            if wz_raw > 32767:
                wz_raw = wz_raw - 65536
            
            result['gyro_x'] = wx_raw / 32768.0 * 2000.0  # dps
            result['gyro_y'] = wy_raw / 32768.0 * 2000.0  # dps
            result['gyro_z'] = wz_raw / 32768.0 * 2000.0  # dps
            result['temperature'] = temp_raw / 100.0  # °C
            result['formatted'] = f"X: {result['gyro_x']:.2f} dps, Y: {result['gyro_y']:.2f} dps, Z: {result['gyro_z']:.2f} dps, T: {result['temperature']:.2f}°C"
            
        elif group_id == 0x53:  # Angle (Roll, Pitch, Yaw)
            # Format: 0x55 0x53 RollL RollH PitchL PitchH YawL YawH VL VH SUM
            # Roll = ((RollH << 8) | RollL) / 32768 * 180 (°)
            # Version = (VH << 8) | VL
            result['type'] = 'Angle (Euler)'
            roll_raw = (group[3] << 8) | group[2]
            pitch_raw = (group[5] << 8) | group[4]
            yaw_raw = (group[7] << 8) | group[6]
            version = (group[9] << 8) | group[8]
            
            # Konversi ke signed int16
            if roll_raw > 32767:
                roll_raw = roll_raw - 65536
            if pitch_raw > 32767:
                pitch_raw = pitch_raw - 65536
            if yaw_raw > 32767:
                yaw_raw = yaw_raw - 65536
            
            # Nilai asli dalam format -180 sampai +180
            result['roll_180'] = roll_raw / 32768.0 * 180.0  # derajat
            result['pitch_180'] = pitch_raw / 32768.0 * 180.0  # derajat
            result['yaw_180'] = yaw_raw / 32768.0 * 180.0  # derajat
            
            # Normalisasi ke 0-360 untuk display
            result['roll'] = result['roll_180']
            if result['roll'] < 0:
                result['roll'] = 360.0 + result['roll']
            
            result['pitch'] = result['pitch_180']
            if result['pitch'] < 0:
                result['pitch'] = 360.0 + result['pitch']
            
            result['yaw'] = result['yaw_180']
            if result['yaw'] < 0:
                result['yaw'] = 360.0 + result['yaw']
            
            result['version'] = version
            result['formatted'] = (f"Roll: {result['roll_180']:.3f}° (-180/+180) / {result['roll']:.2f}° (0-360), "
                                 f"Pitch: {result['pitch_180']:.3f}° (-180/+180) / {result['pitch']:.2f}° (0-360), "
                                 f"Yaw: {result['yaw_180']:.3f}° (-180/+180) / {result['yaw']:.2f}° (0-360), "
                                 f"Version: {version}")
            
        elif group_id == 0x54:  # Magnetic
            # Format: 0x55 0x54 HxL HxH HyL HyH HzL HzH TL TH SUM
            # Hx = ((HxH << 8) | HxL)
            # T = ((TH << 8) | TL) / 100 (°C)
            result['type'] = 'Magnetic'
            hx_raw = (group[3] << 8) | group[2]
            hy_raw = (group[5] << 8) | group[4]
            hz_raw = (group[7] << 8) | group[6]
            temp_raw = (group[9] << 8) | group[8]
            
            # Konversi ke signed int16
            if hx_raw > 32767:
                hx_raw = hx_raw - 65536
            if hy_raw > 32767:
                hy_raw = hy_raw - 65536
            if hz_raw > 32767:
                hz_raw = hz_raw - 65536
            
            # Nilai asli (raw LSB)
            result['mag_x'] = hx_raw
            result['mag_y'] = hy_raw
            result['mag_z'] = hz_raw
            result['temperature'] = temp_raw / 100.0  # °C
            
            # Nilai setelah koreksi dengan faktor skala 77 (seperti Witmotion)
            # Penjelasan faktor skala:
            # - Resolusi magnetometer: 13 nT/LSB = 0.013 µT/LSB (dari datasheet)
            # - Faktor skala 77 berasal dari: 1/77 ≈ 0.013 µT/LSB
            # - Konversi: Raw LSB / 77 = nilai dalam µT (microTesla)
            # - Ini sesuai dengan output Witmotion yang menggunakan resolusi 0.013 µT/LSB
            MAGNETIC_SCALE_FACTOR = 77.0  # Faktor konversi: 1/77 ≈ 0.013 µT/LSB = 13 nT/LSB
            result['mag_x_corrected'] = hx_raw / MAGNETIC_SCALE_FACTOR  # µT
            result['mag_y_corrected'] = hy_raw / MAGNETIC_SCALE_FACTOR  # µT
            result['mag_z_corrected'] = hz_raw / MAGNETIC_SCALE_FACTOR  # µT
            
            result['formatted'] = (f"Raw: X: {result['mag_x']}, Y: {result['mag_y']}, Z: {result['mag_z']}, T: {result['temperature']:.2f}°C | "
                                 f"Corrected (÷77): X: {result['mag_x_corrected']:.3f}, Y: {result['mag_y_corrected']:.3f}, Z: {result['mag_z_corrected']:.3f}")
            
        elif group_id == 0x59:  # Quaternion
            # Format: 0x55 0x59 Q0L Q0H Q1L Q1H Q2L Q2H Q3L Q3H SUM
            # Q0 = ((Q0H << 8) | Q0L) / 32768
            result['type'] = 'Quaternion'
            q0_raw = (group[3] << 8) | group[2]
            q1_raw = (group[5] << 8) | group[4]
            q2_raw = (group[7] << 8) | group[6]
            q3_raw = (group[9] << 8) | group[8]
            
            # Konversi ke signed int16
            if q0_raw > 32767:
                q0_raw = q0_raw - 65536
            if q1_raw > 32767:
                q1_raw = q1_raw - 65536
            if q2_raw > 32767:
                q2_raw = q2_raw - 65536
            if q3_raw > 32767:
                q3_raw = q3_raw - 65536
            
            result['q0'] = q0_raw / 32768.0
            result['q1'] = q1_raw / 32768.0
            result['q2'] = q2_raw / 32768.0
            result['q3'] = q3_raw / 32768.0
            result['formatted'] = f"Q0: {result['q0']:.4f}, Q1: {result['q1']:.4f}, Q2: {result['q2']:.4f}, Q3: {result['q3']:.4f}"
            
        else:
            result['type'] = f'Unknown (0x{group_id:02X})'
            result['raw_data'] = ' '.join([f'{b:02X}' for b in group[2:10]])
            result['formatted'] = f"Raw: {result['raw_data']}"
            
    except Exception as e:
        result['error'] = str(e)
        result['formatted'] = f"Error parsing: {e}"
    
    return result

def find_package_start(buffer):
    """
    Mencari package yang dimulai dengan grup 0x55, 0x50 dan diakhiri dengan grup 0x55, 0x59
    Memverifikasi bahwa setiap grup (setiap 11 bytes) dimulai dengan 0x55
    
    Args:
        buffer: deque berisi bytes
        
    Returns:
        int: Index dari start byte, atau -1 jika tidak ditemukan
    """
    for i in range(len(buffer) - PACKAGE_SIZE + 1):
        # Cari grup pertama yang dimulai dengan 0x55, 0x50
        if buffer[i] == START_BYTE and i + 1 < len(buffer) and buffer[i + 1] == START_GROUP_BYTE:
            # Pastikan ada cukup data untuk package lengkap
            if len(buffer) - i < PACKAGE_SIZE:
                continue
            
            # Verifikasi bahwa setiap grup dimulai dengan 0x55
            is_valid = True
            for group_idx in range(6):
                group_start = i + (group_idx * GROUP_SIZE)
                if group_start >= len(buffer) or buffer[group_start] != START_BYTE:
                    is_valid = False
                    break
            
            # Verifikasi grup terakhir dimulai dengan 0x55, 0x59
            if is_valid:
                last_group_start = i + (5 * GROUP_SIZE)  # Grup ke-6 (index 5)
                if last_group_start + 1 < len(buffer) and buffer[last_group_start + 1] == END_GROUP_BYTE:
                    return i
            
    return -1

def print_package(groups, timestamp, show_raw=True):
    """
    Menampilkan package yang sudah di-parse
    
    Args:
        groups: List of 6 groups
        timestamp: Timestamp string
        show_raw: Tampilkan raw hex dari seluruh package
    """
    print(f"\n[{timestamp}] ========== PACKAGE DITEMUKAN ==========")
    
    # Tampilkan raw hex dari seluruh package jika diminta
    if show_raw:
        all_bytes = []
        for group in groups:
            all_bytes.extend(group)
        raw_hex = ' '.join([f'{b:02X}' for b in all_bytes])
        raw_array = ', '.join([f'0x{b:02X}' for b in all_bytes])
        print(f"  Raw Package ({len(all_bytes)} bytes):")
        print(f"    Hex: {raw_hex}")
        print(f"    Array: [{raw_array}]")
        print()
    
    # Tampilkan setiap grup secara detail dengan konversi nilai real
    for group_idx, group in enumerate(groups, 1):
        # Format hex untuk grup
        hex_str = ' '.join([f'{b:02X}' for b in group])
        
        # Format array untuk grup
        array_str = ', '.join([f'0x{b:02X}' for b in group])
        
        # Identifikasi grup berdasarkan byte kedua
        group_id = f"0x{group[1]:02X}" if len(group) > 1 else "Unknown"
        
        # Konversi ke nilai real
        converted = convert_group_data(group)
        
        print(f"  Grup {group_idx} (ID: {group_id}):")
        print(f"    Hex: {hex_str}")
        print(f"    Array: [{array_str}]")
        
        if converted:
            # Tampilkan type dan nilai real
            checksum_status = "✓" if converted.get('checksum_valid', False) else "✗"
            print(f"    Type: {converted.get('type', 'Unknown')} [Checksum: {checksum_status}]")
            print(f"    Real Value: {converted.get('formatted', 'N/A')}")
            
            # Tampilkan warning jika checksum tidak valid
            if not converted.get('checksum_valid', True):
                print(f"    ⚠ Warning: Checksum mismatch! Calc: 0x{converted['checksum']['calculated']:02X}, Recv: 0x{converted['checksum']['received']:02X}")
            
            if 'error' in converted:
                print(f"    ⚠ Error: {converted['error']}")
        
        print()  # Baris kosong antar grup
    
    print(f"[{timestamp}] =========================================\n")

def read_serial_hex(port='COM14', baudrate=115200, timeout=1):
    """
    Membaca data hex dari serial port
    
    Args:
        port: Port serial (default: COM14)
        baudrate: Baudrate (default: 115200)
        timeout: Timeout dalam detik (default: 1)
    """
    try:
        # Membuka koneksi serial
        ser = serial.Serial(
            port=port,
            baudrate=baudrate,
            timeout=timeout,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE
        )
        
        print(f"Terhubung ke {port} dengan baudrate {baudrate}")
        print("Menunggu data... (Tekan Ctrl+C untuk keluar)\n")
        print("Format package: 6 grup × 11 bytes = 66 bytes total")
        print("Package dimulai dengan grup 0x55, 0x50 dan diakhiri dengan grup 0x55, 0x59\n")
        
        # Buffer untuk menyimpan data yang belum lengkap
        buffer = deque(maxlen=200)  # Max buffer size untuk menghindari memory overflow
        
        try:
            while True:
                # Membaca data dari serial port
                if ser.in_waiting > 0:
                    data = ser.read(ser.in_waiting)
                    
                    if data:
                        # Tambahkan data ke buffer
                        buffer.extend(data)
                        
                        # Cari dan parse package
                        while len(buffer) >= PACKAGE_SIZE:
                            start_idx = find_package_start(buffer)
                            
                            if start_idx == -1:
                                # Tidak ada start byte yang valid, buang byte pertama
                                if len(buffer) > 0:
                                    buffer.popleft()
                                break
                            
                            # Buang bytes sebelum start byte
                            for _ in range(start_idx):
                                buffer.popleft()
                            
                            # Ambil package lengkap (66 bytes)
                            if len(buffer) >= PACKAGE_SIZE:
                                package_bytes = bytes([buffer[i] for i in range(PACKAGE_SIZE)])
                                
                                # Parse package
                                groups = parse_package(package_bytes)
                                
                                if groups:
                                    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                                    print_package(groups, timestamp)
                                    
                                    # Hapus package yang sudah di-parse dari buffer
                                    for _ in range(PACKAGE_SIZE):
                                        buffer.popleft()
                                else:
                                    # Jika parsing gagal, buang byte pertama dan coba lagi
                                    buffer.popleft()
                            else:
                                break
                        
                        # Tampilkan info buffer jika masih ada data yang belum lengkap
                        if len(buffer) > 0 and len(buffer) < PACKAGE_SIZE:
                            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                            hex_buffer = ' '.join([f'{b:02X}' for b in buffer])
                            print(f"[{timestamp}] Buffer: {len(buffer)}/{PACKAGE_SIZE} bytes - {hex_buffer}")
                
        except KeyboardInterrupt:
            print("\n\nMenghentikan pembacaan data...")
            
    except serial.SerialException as e:
        print(f"Error: Tidak dapat membuka port {port}")
        print(f"Detail: {e}")
        sys.exit(1)
        
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
        
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print(f"Port {port} ditutup.")

def main():
    parser = argparse.ArgumentParser(
        description='Membaca data hex dari serial port',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Contoh penggunaan:
  python read_serial_hex.py                    # Menggunakan default COM14, 115200
  python read_serial_hex.py -p COM3            # Menggunakan COM3 dengan baudrate default
  python read_serial_hex.py -p COM5 -b 9600     # Menggunakan COM5 dengan baudrate 9600
        """
    )
    
    parser.add_argument(
        '-p', '--port',
        type=str,
        default='COM14',
        help='Port serial (default: COM14)'
    )
    
    parser.add_argument(
        '-b', '--baudrate',
        type=int,
        default=115200,
        help='Baudrate (default: 115200)'
    )
    
    parser.add_argument(
        '-t', '--timeout',
        type=float,
        default=1.0,
        help='Timeout dalam detik (default: 1.0)'
    )
    
    args = parser.parse_args()
    
    read_serial_hex(
        port=args.port,
        baudrate=args.baudrate,
        timeout=args.timeout
    )

if __name__ == '__main__':
    main()
