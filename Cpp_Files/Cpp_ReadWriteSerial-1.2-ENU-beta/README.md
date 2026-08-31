# Cpp_ReadWriteSerial-1.2-ENU-beta

Versi **1.2-ENU-beta** — clone dari `Cpp_Files/Cpp_ReadWriteSerial-1.1-ENU-beta` dengan hitungan **posisi ENU** dan **kecepatan surge/sway** (internal).

Program C++ mini-PC: baca telemetry USB dari **ESP-Now_ESP32-S3_Remote-Side-05**, kirim `$HB`, tulis balik `timestamp,result` (rudder deg).

| Pasangan | Path |
|----------|------|
| Firmware Remote | `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-05` |
| Firmware User | `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_User-Side-05` |
| Dashboard | `Pythonfile/Way_Points_Tracking/Local Monitor Dashboard-beta1.5.py` |
| Versi sebelumnya | `Cpp_Files/Cpp_ReadWriteSerial-1.1-ENU-beta` |
| Varian NED | `Cpp_Files/Cpp_ReadWriteSerial-1.2-NED-beta` |

**Firmware Remote tidak diubah.** CSV stdout dan serial TX sama seperti 1.1.

Kolom `calc_deg_servo_1/2` dari Remote-Side-05 sudah lewat `RUDDER_DEG_FILTER` (default oversample). Bridge 1.2 tidak memfilter ulang sudut rudder.

---

## `u` dan `v` itu kecepatan, bukan perpindahan

| Simbol | Jenis | Satuan | Arti |
|--------|--------|--------|------|
| `x`, `y` | **posisi / perpindahan** dari home | **m** | di mana kapal di peta |
| `ẋ`, `ẏ` | **kecepatan** di peta | **m/s** | seberapa cepat `x`,`y` berubah |
| `\|V\|` | **kecepatan** ground | **m/s** | `√(ẋ² + ẏ²)` |
| `u` surge | **kecepatan** badan | **m/s** | maju (+) / mundur (−) |
| `v` sway | **kecepatan** badan | **m/s** | kanan (+) / kiri (−) |
| `ψ` | sudut | **rad** | heading dari Utara |
| `r` | **kecepatan sudut** | **rad/s** | dari `gyro_z` |

`u` bukan “sudah maju berapa meter”, melainkan “sedang maju berapa m/s”.

---

## Dua kerangka

### Global ENU (tetap di bumi, origin = home)

| Simbol | Nama | Arti |
|--------|------|------|
| `x` | East | meter ke **timur** dari home |
| `y` | North | meter ke **utara** dari home |
| `ẋ` (`dx/dt`) | kecepatan timur | m/s |
| `ẏ` (`dy/dt`) | kecepatan utara | m/s |

`ẋ`, `ẏ` = ke mana kapal bergerak **di peta**.

### Badan kapal (ikut haluan)

| Simbol | Nama | Arti |
|--------|------|------|
| `u` | surge | m/s, maju (+) / mundur (−) |
| `v` | sway | m/s, ke **kanan** (+) / kiri (−) |
| `ψ` (psi) | yaw / heading | rad, dari **Utara**, naik ke **timur** (kompas) |
| `r` | yaw rate | rad/s, dari `gyro_z` — **bukan** untuk rumus `u, v` |

`u`, `v` = maju/mundur dan kiri/kanan relatif **badan kapal**.

---

## Langkah hitungan (internal)

Kode: `include/local_frame.hpp` + `include/enu_velocity.hpp`.

### Langkah 1 — `lat, lon` → `x, y` (posisi, meter)

| Simbol | Arti |
|--------|------|
| `lat`, `lon` | GPS (derajat) |
| `lat0`, `lon0` | origin = home, atau default `-7.2867106, 112.7958064` |
| `R` | jari-jari bumi `6 371 000` m |

```text
x = (lon − lon0) × (π/180) × R × cos(lat0)        // East, m
y = (lat − lat0) × (π/180) × R                    // North, m
```

Home sendiri: `(x, y) = (0, 0)`.

Origin:

| Kondisi | Origin |
|---------|--------|
| Belum ada home / `[WP] Home: <none>` | `-7.2867106, 112.7958064` |
| `[WP] Home: lat, lon` | koordinat itu; tracker `ẋ,ẏ` di-reset |

`lat ≈ 0` dan `lon ≈ 0` dianggap GPS belum fix → `u, v` tidak dihitung (dianggap 0).

### Langkah 2 — `ẋ, ẏ` dari perubahan posisi (kecepatan peta)

`Δt = timestamp_k − timestamp_{k-1}` (CSV ≈ 0.1 s).

```text
ẋ_raw ≈ (x_k − x_{k-1}) / Δt
ẏ_raw ≈ (y_k − y_{k-1}) / Δt
```

Low-pass (α = 0.70):

```text
ẋ = α · ẋ_lama + (1 − α) · ẋ_raw
ẏ = α · ẏ_lama + (1 − α) · ẏ_raw
|V| = √(ẋ² + ẏ²)
```

`ẏ > 0` → ke utara. `ẋ > 0` → ke timur.

`|V|` secara fisis sama dengan `speedMps` GPS, tetapi dihitung dari Δposisi. Nilai per sampel sering beda (noise). `speedMps` **tidak** ada di CSV 8 kolom, jadi tidak dipakai di 1.2.

Sampel pertama, `Δt` aneh (`< 1 ms` atau `> 1 s`), atau origin baru → `ẋ, ẏ, u, v = 0`.

### Langkah 3 — `ψ` dari IMU

Kolom CSV `yaw` dalam **derajat** (0–360). Internal pakai **radian**:

```text
ψ = yaw × π / 180
r = gyro_z × π / 180     // rad/s, untuk state NMPC, bukan rumus u,v
```

| `yaw` | `ψ` | Haluan |
|------:|----:|--------|
| 0° | 0 | Utara |
| 90° | π/2 | Timur |
| 180° | π | Selatan |
| 270° | 3π/2 | Barat |

### Langkah 4 — rotasi ke badan: `u, v` (kecepatan, m/s)

```text
u =  ẋ sinψ + ẏ cosψ     // surge
v =  ẋ cosψ − ẏ sinψ     // sway
```

Arah maju di peta ENU = `(sinψ, cosψ)` (East, North).  
Arah kanan = `(cosψ, −sinψ)`.

Cek `ψ = 0` (haluan utara): `u = ẏ`, `v = ẋ`.

---

## Contoh angka

Origin default. Titik GPS ≈ 20 m utara, 10 m timur:

```text
lat ≈ -7.2865307    lon ≈ 112.7958971
x ≈ 10 m            y ≈ 20 m
```

0.1 s kemudian `x = 10.2`, `y = 20.5` (sebelum filter):

```text
ẋ ≈ 2 m/s     ẏ ≈ 5 m/s     |V| ≈ 5.39 m/s
```

Heading `yaw = 0°` (`ψ = 0`):

```text
u ≈ 5 m/s     v ≈ 2 m/s
```

Heading `yaw = 90°` (haluan timur), `ẋ, ẏ` sama:

```text
u ≈ 2 m/s     v ≈ −5 m/s
```

---

## Apa yang tampil di terminal

Hitungan **tidak** masuk stdout CSV dan **tidak** dikirim ke serial.

| Data | Tujuan |
|------|--------|
| CSV 8 kolom asli | stdout (`--print`) |
| `[WP] ...` | stdout |
| `$SHUTDOWN` | stdout + matikan OS |
| `timestamp,result` | serial TX saja |
| origin, `[KIN] x y ẋ ẏ \|V\| u v psi r` | **stderr**, ringkasan ~1 Hz |

Contoh stderr:

```text
[KIN] x=10.05 y=20.12 m | ẋ=0.08 ẏ=0.31 |V|=0.32 m/s | u=0.30 v=0.09 m/s | psi=12.0 deg r=0.021 rad/s
```

---

## Format input (dari ESP32 Remote)

Sama seperti 1.1: CSV 8 kolom saat RC auto, `[WP] ...` saat waypoint, `$SHUTDOWN`.

```text
timestamp,lat,lon,calc_deg_servo_1,calc_deg_servo_2,yaw,gyro_z,yaw_rate
```

**Baud:** `115200`

`--print` hanya memfilter stdout. Hitung ENU/`u,v` + rudder + `$HB` tetap jalan.

Mode default: `--rudder-mode zero`. Uji: `--rudder-mode yawrate2`.

---

## Build

```powershell
cd "Cpp_Files\Cpp_ReadWriteSerial-1.2-ENU-beta"
g++ -std=c++17 -Iinclude src/main.cpp src/serial_port.cpp -o read_write_serial.exe
```

Static (tanpa DLL):

```powershell
g++ -std=c++17 -static -static-libgcc -static-libstdc++ -Iinclude src/main.cpp src/serial_port.cpp -o read_write_serial.exe
```

---

## Penggunaan

```powershell
cd "Cpp_Files\Cpp_ReadWriteSerial-1.2-ENU-beta"
.\read_write_serial.exe --port COM16 --baud 115200 --rudder-mode yawrate2 --print all
.\read_write_serial.exe --port COM16 --print none
```

| Opsi | Default | Keterangan |
|------|---------|------------|
| `--port` | `COM16` | Port serial |
| `--baud` | `115200` | Baud rate |
| `--timeout` | `1000` | Timeout baca (ms) |
| `--print` | `all` | `all` / `csv` / `wp` / `none` |
| `--rudder-mode` | `zero` | `zero`, `yawrate2`, `demo` |
| `--op` / `--field-a` / `--field-b` | — | Hanya mode `demo` |
| `--help` / `-h` | — | Bantuan |

Auto-start: [`startup_guide.md`](startup_guide.md). DLL MinGW satu folder dengan exe jika build dynamic.

---

## Catatan

1. Port COM hanya satu aplikasi.
2. `u, v` untuk bekal NMPC (`s = [v, r, X, Y, ψ]` dengan state peta sesuai kerangka yang dipilih); rudder 1.2 belum memakai NMPC.
3. Model NMPC memakai surge konstan `u_0`; `u` hasil GPS berguna untuk cek / ganti `u_0` nanti.
4. User Windows perlu hak `shutdown`. Setelah mati, mini PC tidak bisa dihidupkan dari dashboard.
