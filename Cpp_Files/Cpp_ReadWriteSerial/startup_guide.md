# Panduan Auto-Start Windows — `read_write_serial.exe`

Cara menjalankan `read_write_serial.exe` otomatis saat login Windows di mini PC.

## Analogi dengan skrip Python

**Python (lama):**
```batch
@echo off
cd /d "D:\path\to\folder"
"C:\Python311\python.exe" "D:\path\to\apm_udp_server_realtime.py"
```

**C++ exe (ini):** tidak perlu path Python — langsung panggil `.exe` + argumen CLI.
```batch
@echo off
cd /d "D:\path\to\Cpp_ReadWriteSerial"
read_write_serial.exe --port COM16 --baud 115200 --rudder-mode yawrate2
```

## File batch yang dipakai

Gunakan `start_read_write_serial.bat` di folder yang sama dengan `read_write_serial.exe` (dan DLL jika build dynamic).

Isi penting (sesuaikan sebelum deploy):

```batch
@echo off
set COM_PORT=COM16
set BAUD=115200
set RUDDER_MODE=yawrate2

cd /d "%~dp0"
read_write_serial.exe --port %COM_PORT% --baud %BAUD% --rudder-mode %RUDDER_MODE%
```

| Variabel | Arti | Contoh |
|----------|------|--------|
| `COM_PORT` | Port serial USB | `COM16` |
| `BAUD` | Baud rate | `115200` |
| `RUDDER_MODE` | Mode rudder | `zero`, `yawrate2`, `demo` |

`%~dp0` = folder tempat file `.bat` berada. Karena itu **biarkan `.bat` tetap di folder program**, jangan dipindah ke Startup tanpa mengubah path.

### Sintaks CLI lengkap (uji manual)

```powershell
.\read_write_serial.exe --port COM16 --baud 115200 --rudder-mode yawrate2
```

Bantuan: `.\read_write_serial.exe --help`

---

## Apakah `.bat` harus di folder Startup?

**Tidak.** Yang harus ada di Startup adalah **pemicu** (shortcut atau bat dengan path absolut). File program tetap di folder aslinya.

### Opsi A — Shortcut di Startup (disarankan)

1. Pastikan `start_read_write_serial.bat`, `read_write_serial.exe`, dan DLL (jika ada) satu folder.
2. Edit `COM_PORT` / `BAUD` / `RUDDER_MODE` di dalam `.bat`.
3. `Win+R` → ketik `shell:startup` → Enter.
4. Buat **shortcut** ke `start_read_write_serial.bat` (klik kanan bat → Send to → Desktop, lalu pindahkan shortcut ke folder Startup; atau drag dengan Alt).
5. Log off/on atau reboot untuk uji.

| Di folder Startup | Di folder program |
|-------------------|-------------------|
| Shortcut `.lnk` | `start_read_write_serial.bat` + `.exe` (+ DLL) |

Keuntungan: `%~dp0` tetap benar, mudah diedit, tidak dobel file.

### Opsi B — Salin `.bat` langsung ke Startup

Boleh, tapi **wajib** path absolut ke folder exe (karena `%~dp0` akan jadi folder Startup):

```batch
@echo off
cd /d "D:\Pengujian\Ship_Model_Control_ESP32-S3 v2026.01\Cpp_Files\Cpp_ReadWriteSerial"
read_write_serial.exe --port COM16 --baud 115200 --rudder-mode yawrate2
```

Sesuaikan drive/path di mini PC.

### Opsi C — Task Scheduler (alternatif)

1. Buka `taskschd.msc`.
2. Create Task → trigger **At log on**.
3. Action: Start a program → `start_read_write_serial.bat`.
4. **Start in**: folder `Cpp_ReadWriteSerial` (penting).

Atau PowerShell (Administrator), sesuaikan `$dir`:

```powershell
$dir = "D:\Pengujian\Ship_Model_Control_ESP32-S3 v2026.01\Cpp_Files\Cpp_ReadWriteSerial"
$action = New-ScheduledTaskAction -Execute "$dir\start_read_write_serial.bat" -WorkingDirectory $dir
$trigger = New-ScheduledTaskTrigger -AtLogOn -User $env:USERNAME
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -RestartCount 999 -RestartInterval (New-TimeSpan -Minutes 1)
Register-ScheduledTask -TaskName "ShipModel_ReadWriteSerial" -Action $action -Trigger $trigger -Settings $settings -Force
```

---

## Cara cek di mini PC (bukan di laptop pengembang)

### Startup folder

```text
Win+R → shell:startup
Win+R → shell:common startup
```

Cek ada `.bat` atau shortcut `.lnk`. Kalau `.lnk`: Properties → **Target** harus mengarah ke bat/exe yang benar.

### Task Scheduler

```text
Win+R → taskschd.msc
```

Cari task terkait (`ShipModel`, `read_write`, dll.) → tab **Triggers** (At log on / At startup) dan **Actions**.

| Lokasi | Artinya |
|--------|---------|
| `shell:startup` / common startup | Jalan otomatis saat user login |
| Task Scheduler (At log on / At startup) | Jalan otomatis sesuai trigger |
| Hanya `.bat` di folder project, tanpa shortcut/task | **Tidak** otomatis |

---

## Tips deploy mini PC

1. Tetapkan **nomor COM tetap** di Device Manager (USB serial → Port Settings → Advanced) agar `COM_PORT` tidak berubah setelah reboot.
2. Batch default menunggu ~15 detik setelah start supaya USB serial sempat terdeteksi.
3. Jika program crash, `start_read_write_serial.bat` otomatis restart setelah beberapa detik.
4. Tutup Serial Monitor PlatformIO / aplikasi lain yang memakai port COM yang sama.
5. Jika build dynamic (bukan static), salin DLL ke folder yang sama dengan `.exe`:
   - `libgcc_s_seh-1.dll`
   - `libgomp-1.dll`
   - `libstdc++-6.dll`
   - `libwinpthread-1.dll`

---

## Ringkas langkah cepat (Opsi A)

1. Edit `COM_PORT` di `start_read_write_serial.bat`.
2. Uji manual: dobel-klik bat, pastikan port terbuka dan data masuk.
3. `shell:startup` → buat shortcut ke bat.
4. Reboot / log off-on → konfirmasi jendela/console program muncul sendiri.
