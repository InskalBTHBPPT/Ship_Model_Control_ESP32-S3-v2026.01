# Panduan Auto-Start Windows — `read_write_serial.exe` (2.0 NMPC)

## File batch

`start_read_write_serial.bat` satu folder dengan exe (+ DLL jika build dynamic).

```batch
set COM_PORT=COM16
set BAUD=115200
set RUDDER_MODE=nmpc
set PRINT_MODE=none
```

`PRINT_MODE`: `all` | `csv` | `wp` | `none`

### Uji manual

```powershell
.\read_write_serial.exe --port COM16 --baud 115200 --rudder-mode nmpc --print all
```

## Opsi A — Shortcut Startup (disarankan)

1. Edit `COM_PORT` di `.bat`.
2. `Win+R` → `shell:startup`.
3. Shortcut ke `start_read_write_serial.bat` (biarkan bat di folder program).
4. Log off/on untuk uji.

## Opsi B — Path absolut di Startup

```batch
cd /d "D:\Pengujian\Ship_Model_Control_ESP32-S3 v2026.01\Cpp_Files\Cpp_ReadWriteSerial-2.0-ENU-NMPC-beta"
read_write_serial.exe --port COM16 --baud 115200 --rudder-mode nmpc
```

## Tips

1. Kunci nomor COM di Device Manager.
2. Batch menunggu ~15 detik agar USB siap.
3. Tutup Serial Monitor lain pada port yang sama.
4. Build dynamic: simpan DLL MinGW di folder exe.
