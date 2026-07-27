# Analisis Keaslian Data End-to-End (ESP-NOW -> Serial -> Python)

Pertanyaan:

> jika data asli dikirim kemudian dikirim melalui `PlatformIO/ESP_Now_Send_Ver2025_revJan2026` kemudian diterima oleh `PlatformIO/ESP_Now_Receive_Ver2025` kemudian di kirim ke PC melalui serial untuk dibaca melalui `Pythonfile/Local Monitor Dashboard 1.0.py` apakah data yang ditampilkan di python adalah data asli

## Jawaban Singkat

Data di Python **bukan raw data mentah 1:1**, tetapi **representasi data asli yang sudah melalui beberapa transformasi terkontrol** (scaling fixed-point, normalisasi, koreksi, dan formatting).

Artinya:

- Untuk mayoritas field, nilainya masih merepresentasikan kondisi asli sistem.
- Namun ada perbedaan numerik kecil/semantik karena proses konversi di sender, receiver, dan parser Python.

## Alur Data yang Terjadi

1. Sensor/aktuator dibaca di sender (`ESP_Now_Send_Ver2025_revJan2026`).
2. Sender menyusun payload struct `DatatoSend`.
3. Banyak field dikonversi ke format fixed-point (`x100`) lalu dikirim via ESP-NOW.
4. Receiver (`ESP_Now_Receive_Ver2025`) menerima payload, melakukan konversi balik (`/100.0`) untuk field tertentu, lalu mencetak CSV ke serial.
5. Python (`Local Monitor Dashboard 1.0.py`) membaca CSV serial 15 kolom, parsing ke numerik, lalu menampilkan indikator/plot.

## Kenapa Tidak 100% Raw

### 1) Fixed-point scaling di sender

Di sender, beberapa nilai float diubah jadi integer dengan skala `x100` (contoh: sudut, speed, battery). Saat ini dilakukan cast ke tipe integer (`int16_t`/`uint16_t`), sehingga:

- presisi dibatasi sekitar 0.01 unit,
- ada pembulatan/truncation.

Jadi nilai di Python adalah hasil rekonstruksi dari fixed-point, bukan float mentah awal.

### 2) Konversi balik di receiver

Receiver membagi nilai fixed-point dengan `100.0` sebelum print serial CSV. Ini membuat nilai kembali ke satuan manusiawi, tapi tetap dalam batas presisi fixed-point.

### 3) Transformasi yaw di sender

Yaw dari IMU yang negatif dinormalisasi menjadi rentang `0..360` sebelum dikirim. Jadi ini bukan nilai raw IMU mentah lagi, tapi nilai yang sudah dinormalisasi.

### 4) Transformasi zigzag di Python

Di `Local Monitor Dashboard 1.0.py`, nilai `zigzag_yaw` dikalikan `-1` saat parsing agar searah konvensi sudut rudder. Ini mengubah tanda nilai relatif terhadap data serial dari receiver.

### 5) Koreksi sudut rudder di Python

`Calc_deg_servo_1` dan `Calc_deg_servo_2` di Python dikurangi offset koreksi (`correction_deg_servo_1/2`). Jadi yang tampil adalah nilai terkalibrasi, bukan raw feedback murni.

### 6) Fallback koordinat 0,0

Jika latitude/longitude terbaca `0.0, 0.0`, Python mengganti ke default location. Pada kondisi ini, tampilan bukan data GPS mentah.

### 7) Perbedaan semantik mode

Sender bisa menghasilkan `mode_auto = 5` pada kondisi tertentu (misalnya netral/konflik mode), sementara deskripsi mode umum sering hanya 0..4. Ini bukan mismatch format, tapi bisa memengaruhi interpretasi di UI.

## Kesimpulan Praktis

Data yang ditampilkan di Python adalah:

- **YA**: representasi yang valid dari data sistem untuk monitoring dan analisis operasional.
- **TIDAK 100%**: data mentah bit-perfect dari sensor, karena ada tahapan normalisasi, scaling, konversi, dan kalibrasi.

Kalau tujuan Anda adalah audit absolut "raw sensor truth", maka perlu logging tambahan yang menyimpan:

- nilai raw sebelum scaling di sender,
- payload integer exact yang dikirim via ESP-NOW,
- CSV serial output receiver,
- hasil parse akhir di Python.

Dengan empat titik logging ini, Anda bisa verifikasi deviasi di setiap tahap secara kuantitatif.

