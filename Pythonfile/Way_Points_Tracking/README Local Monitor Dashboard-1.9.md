# Local Monitor Dashboard 1.9

File: `Local Monitor Dashboard-1.9.py`

Dari 1.8. Dashboard PySide6 di laptop. Membaca telemetry **24 kolom** dari User-Side-06. Tidak terhubung ke mini PC. Tidak menambah kolom CSV.

| Pasangan | Path |
|----------|------|
| User-Side | `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_User-Side-06` |
| Remote-Side | `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-05.3` |
| Mini PC | `Cpp_Files/Cpp_ReadWriteSerial-2.4-ENU-NMPC` (USB Remote) |

Tab: Map Points, Live Data, Analize Data, 3D.

---

## Simbol peta

Home di ketiga peta adalah kotak hijau, tanpa lingkaran. Waypoint adalah bintang bernomor plus lingkaran radius 3 m. Jumlah titik mengikuti isi file `No,Lat,Long`. Garis rencana oranye putus-putus hanya antar waypoint, tidak dari Home.

| Peta | Kapal | Heading | Jejak |
|------|--------|---------|-------|
| Live Data | belah ketupat hijau pada sampel jejak (default sekitar 1 per detik) | merah putus-putus, 5 m | biru |
| Map Points | satu belah ketupat hijau di posisi live | oranye solid, 5 m | — |
| Analyze | belah ketupat hijau mengikuti slider | merah putus-putus 5 m pada slider. Checkbox **Heading Line** menambah garis merah solid di sepanjang jejak | biru |

---

## Analyze

**Load Log CSV** menggambar jejak kapal. **Load Waypoints** menaruh Home, bintang, dan lingkaran 3 m di atas jejak. Overlay waypoint tetap ada setelah log dimuat lagi.

Pindah mode dari kolom `mode_auto` ditandai terpisah dari waypoint:

| Tanda | Arti |
|-------|------|
| Segitiga hijau ke atas | Auto mulai (`mode_auto` 1 atau 2) |
| Segitiga merah ke bawah | Auto selesai, kembali manual |
| Belah ketupat oranye | Log berakhir saat masih auto |

Setiap sesi punya nomor di tooltip, plus waktu dalam detik. Log yang dibuka sudah dalam mode auto menandai baris pertama sebagai mulai.

---

## Tab 3D

Bingkai ENU lokal (Three.js di folder `vendor/three/`), tanpa petak satelit. Sumbu: timur +X, utara −Z, atas +Y.

| Kontrol | Fungsi |
|---------|--------|
| Load Log CSV | jejak uji |
| Load Waypoints | Home, bintang, lingkaran 3 m, garis rencana |
| Prediksi | lintasan model, garis merah |
| Slider, Play, 0.5×–8× | memutar **log uji** saja. 1× = satu detik log per detik dinding |
| POV kapal / Orbit | kamera. Menggeser slider menghentikan Play. Di ujung log, Play mengulang dari awal |

| Gambar | Warna |
|--------|--------|
| Jejak log | hitam |
| Lintasan prediksi | merah |
| Rencana waypoint | oranye putus-putus |
| Mulai auto | kerucut hijau |
| Selesai auto | kerucut merah |
| Log berakhir masih auto | bentuk berlian oranye |

---

## Prediksi

Tombol **Prediksi** di tab 3D memakai file yang baru dimuat **Load Waypoints** di tab yang sama. Aktif bila file punya baris Home dan minimal satu waypoint. Hasilnya satu garis merah di scene yang sama. Tidak ada Play kedua.

`predict_track.exe` harus berada di folder yang sama dengan dashboard. Program itu menutup loop model NMPC 2.3: posisi awal = Home, yaw awal 90° (timur), surge `u0 = 0.6114` m/s, sway `v = 0`, kemudi awal 0, air tenang. Saat berjalan, tombol bertuliskan **Menghitung…**. Memuat file waypoint lain menghapus garis merah.

Bangun dari root repo:

```text
g++ -std=c++17 -O2 -ICpp_Files/Cpp_ReadWriteSerial-2.3-ENU-NMPC/nmpc
  "Pythonfile/Way_Points_Tracking/predict_track.cpp"
  Cpp_Files/Cpp_ReadWriteSerial-2.3-ENU-NMPC/nmpc/nmpc_kapal_waypoint.c
  Cpp_Files/Cpp_ReadWriteSerial-2.3-ENU-NMPC/nmpc/waypoint_manager.c
  Cpp_Files/Cpp_ReadWriteSerial-2.3-ENU-NMPC/nmpc/geo_enu.c
  -o "Pythonfile/Way_Points_Tracking/predict_track.exe"
```

---

## Alarm baterai

`battery_1` (kontrol) dan `battery_2` (motor) adalah tegangan paket LiPo 3S. Jika salah satu di bawah **10,8 V** (3,6 V/sel), label berkedip merah dan bunyi berulang. Tombol **Diamkan alarm** mematikan bunyi; kedip tetap. Alarm mengikuti nilai yang tampil, bukan menunggu 10 detik.

Pulang ke Home karena baterai diputuskan di Remote-05.3 (10 detik terus-menerus saat mode auto) lalu dikirim sebagai `$RTL` ke mini PC 2.4. Dashboard tidak mengirim perintah itu.

Pulang ke Home karena operator memilih **Auto** pada dialog misi selesai dikirim sebagai `$GOHOME` (lihat bagian berikut).

---

## Yaw dan heading setpoint

`yaw` di panel Live adalah haluan kapal kompas CW (0 = Utara, 90 = Timur). Dashboard hanya `kolom / 100`.

| `mode_auto` | Isi setpoint yang ditampilkan |
|-------------|-------------------------------|
| 0 manual | sama dengan yaw |
| 2 auto mini PC | bearing dari 2.4 (waypoint, atau Home setelah `$RTL` / `$GOHOME`) |
| 1 auto PD | bearing ke waypoint aktif |

---

## `u` dan `v` (lokal)

Tidak datang dari firmware. Dihitung di PC dari `lat`, `lon`, `yaw`, `timestamp`:

```text
u = ẋ sinψ + ẏ cosψ
v = ẋ cosψ − ẏ sinψ
```

Tampil di Live dan masuk log sebagai `u (m/s)`, `v (m/s)`. Mini PC 2.4 tidak memakai angka ini (`v = 0`, `u0 = 0.6114`).

---

## Perintah yang keluar dari dashboard

| Tombol | Serial ke User-Side | Lanjut ke |
|--------|---------------------|-----------|
| Send Way Points | `$WPSET,...` | ESP-NOW `0xA1` → Remote cetak `[WP]` ke mini PC |
| Shutdown | `$SHUTDOWN` | ESP-NOW `0xA2` cmd=1 → Remote cetak `$SHUTDOWN` ke 2.4 |
| Dialog misi, tombol Auto | `$GOHOME` | ESP-NOW `0xA2` cmd=2 → Remote cetak `$GOHOME` ke 2.4 |

Balasan: `$WACK,...`, `$SACK,...`, dan `$HACK,...`. `$SACK,OK` dan `$HACK,OK` berarti paket terkirim, bukan OS sudah mati dan bukan kapal sudah di Home.

Indikator waypoint pada Live memakai `track_wp_index`: `254` tampil **Selesai**, `255` tampil **Home**.

---

## Dialog misi selesai

Saat `track_wp_index` = 254 dan mode masih auto, jendela non-modal **Misi selesai** muncul. Artinya kapal sudah masuk radius titik terakhir: kemudi 0° dan propeller netral, kapal boleh hanyut.

| Tombol | Perilaku |
|--------|----------|
| Manual | Dialog tertutup. Operator memindahkan CH6 ke manual. Stik lalu menguasai kemudi dan propeller. Dashboard tidak mengirim perintah |
| Auto | Dashboard mengirim `$GOHOME`. Remote melepas kunci propeller netral (kembali ke kecepatan CH3/CH5 yang tersimpan) dan 2.4 mengarahkan NMPC ke Home |

Jika operator memindahkan CH6 ke manual sebelum memilih tombol, dialog tertutup dan stik langsung berlaku. Kunci misi di mini PC tidak dilepas. Masuk auto lagi tanpa mengirim waypoint baru menahan propeller netral dan kemudi 0, lalu dialog muncul kembali.

Sampai Home (`track_wp_index` 255) tidak membuka dialog kedua. `$RTL` baterai tetap jalur terpisah dan tidak lewat dialog ini.

---

## Jalankan

```powershell
cd Pythonfile\Way_Points_Tracking
python "Local Monitor Dashboard-1.9.py"
```
