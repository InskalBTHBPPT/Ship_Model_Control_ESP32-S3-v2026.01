# Local Monitor Dashboard beta 1.6

File: `Local Monitor Dashboard-beta1.6.py`

Dashboard PySide6 di laptop. Membaca telemetry **24 kolom** dari User-Side-05. Tidak terhubung ke mini PC.

| Pasangan | Path |
|----------|------|
| User-Side | `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_User-Side-05` |
| Remote-Side | `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-05` |
| Mini PC | `Cpp_Files/Cpp_ReadWriteSerial-2.0-ENU-NMPC-beta` (USB Remote) |

---

## Yaw dan heading setpoint

`yaw` di panel Live adalah haluan kapal kompas CW (0 = Utara, 90 = Timur), setelah Remote membagi rantai wrap → +90° pasang → `360 − yaw`. Dashboard hanya `kolom / 100`.

`heading_setpoint` **bukan** zigzag. Kolom lama `zigzag_yaw` hanya dikenali saat membuka log firmware lama.

| `mode_auto` | Isi setpoint di dashboard |
|-------------|---------------------------|
| 0 manual | sama dengan yaw |
| 2 auto mini PC (default) | sama dengan yaw, error = 0. Alg 2 tidak mengirim bearing |
| 1 auto PD | bearing ke waypoint aktif |

Plot "Heading Setpoint" pada alg 2 menempel pada garis yaw.

---

## `u` dan `v` (lokal)

Tidak datang dari firmware. Dihitung di PC dari `lat`, `lon`, `yaw`, `timestamp`:

```text
u = ẋ sinψ + ẏ cosψ
v = ẋ cosψ − ẏ sinψ
```

Tampil di Live dan masuk log sebagai `u (m/s)`, `v (m/s)`. Mini PC 2.0 tidak memakai angka ini (`v = 0`, `u0 = 0.6114`).

---

## Perintah yang keluar dari dashboard

| Tombol | Serial ke User-Side | Lanjut ke |
|--------|---------------------|-----------|
| Send Way Points | `$WPSET,...` | ESP-NOW `0xA1` → Remote cetak `[WP]` ke mini PC |
| Shutdown | `$SHUTDOWN` | ESP-NOW `0xA2` → Remote cetak `$SHUTDOWN` ke 2.0 |

Balasan: `$WACK,...` dan `$SACK,...`. `$SACK,OK` berarti paket terkirim, bukan OS sudah mati.

---

## Jalankan

```powershell
cd Pythonfile\Way_Points_Tracking
python "Local Monitor Dashboard-beta1.6.py"
```
