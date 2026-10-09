# Local Monitor Dashboard 1.7

File: `Local Monitor Dashboard-1.7.py`

Dashboard PySide6 di laptop. Membaca telemetry **24 kolom** dari User-Side-05. Tidak terhubung ke mini PC. Tidak menambah kolom CSV.

| Pasangan | Path |
|----------|------|
| User-Side | `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_User-Side-05` |
| Remote-Side | `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-05.2` |
| Mini PC | `Cpp_Files/Cpp_ReadWriteSerial-2.3-ENU-NMPC` (USB Remote) |

---

## Alarm baterai

`battery_1` (kontrol) dan `battery_2` (motor) adalah tegangan paket LiPo 3S. Jika salah satu di bawah **10,8 V** (3,6 V/sel), label berkedip merah dan bunyi berulang. Tombol **Diamkan alarm** mematikan bunyi; kedip tetap. Alarm mengikuti nilai yang tampil, bukan menunggu 10 detik.

Pulang ke Home diputuskan di Remote-05.2 (10 detik terus-menerus saat mode auto) lalu dikirim sebagai `$RTL` ke mini PC 2.3. Dashboard tidak mengirim perintah itu.

---

## Yaw dan heading setpoint

`yaw` di panel Live adalah haluan kapal kompas CW (0 = Utara, 90 = Timur). Dashboard hanya `kolom / 100`.

| `mode_auto` | Isi setpoint yang ditampilkan |
|-------------|-------------------------------|
| 0 manual | sama dengan yaw |
| 2 auto mini PC | bearing dari 2.3 (waypoint, atau Home setelah `$RTL`) |
| 1 auto PD | bearing ke waypoint aktif |

---

## `u` dan `v` (lokal)

Tidak datang dari firmware. Dihitung di PC dari `lat`, `lon`, `yaw`, `timestamp`:

```text
u = ẋ sinψ + ẏ cosψ
v = ẋ cosψ − ẏ sinψ
```

Tampil di Live dan masuk log sebagai `u (m/s)`, `v (m/s)`. Mini PC 2.3 tidak memakai angka ini (`v = 0`, `u0 = 0.6114`).

---

## Analyze

**Load Log CSV** menggambar jejak kapal. **Load Waypoints** membaca CSV `No,Lat,Long` (jumlah titik mengikuti isi file). Home berupa kotak hijau. Setiap waypoint berupa bintang bernomor, lingkaran radius 3 m, dan garis putus-putus antar waypoint.

Pada jejak log, pindah mode dari kolom `mode_auto` ditandai terpisah dari waypoint:

| Tanda | Arti |
|-------|------|
| Segitiga hijau ke atas | Auto mulai (`mode_auto` 1 atau 2) |
| Segitiga merah ke bawah | Auto selesai, kembali manual |
| Belah ketupat oranye | Log berakhir saat masih auto |

Setiap sesi punya nomor di tooltip, plus waktu dalam detik. Log yang dibuka sudah dalam mode auto menandai baris pertama sebagai mulai.

---

## Perintah yang keluar dari dashboard

| Tombol | Serial ke User-Side | Lanjut ke |
|--------|---------------------|-----------|
| Send Way Points | `$WPSET,...` | ESP-NOW `0xA1` → Remote cetak `[WP]` ke mini PC |
| Shutdown | `$SHUTDOWN` | ESP-NOW `0xA2` → Remote cetak `$SHUTDOWN` ke 2.3 |

Balasan: `$WACK,...` dan `$SACK,...`. `$SACK,OK` berarti paket terkirim, bukan OS sudah mati.

---

## Jalankan

```powershell
cd Pythonfile\Way_Points_Tracking
python "Local Monitor Dashboard-1.7.py"
```
