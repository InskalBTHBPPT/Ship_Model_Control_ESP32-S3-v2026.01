#include <Arduino.h>
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "simulate_nmpc_kapal.h"
#include "simulate_nmpc_kapal_initialize.h"
#include "simulate_nmpc_kapal_terminate.h"

// ============================================================================
// Data Buffers (Ditempatkan di Memory PSRAM untuk Menghindari Stack Overflow)
// ============================================================================
static double *hist_dim = nullptr;
static double *history_input = nullptr;
static double *time_vector = nullptr;
static double *rmse_data = nullptr;

void initBuffers() {
  if (!hist_dim) hist_dim = (double*) heap_caps_malloc(755 * sizeof(double), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!hist_dim) hist_dim = (double*) malloc(755 * sizeof(double));

  if (!history_input) history_input = (double*) heap_caps_malloc(151 * sizeof(double), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!history_input) history_input = (double*) malloc(151 * sizeof(double));

  if (!time_vector) time_vector = (double*) heap_caps_malloc(151 * sizeof(double), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!time_vector) time_vector = (double*) malloc(151 * sizeof(double));

  if (!rmse_data) rmse_data = (double*) heap_caps_malloc(3 * sizeof(double), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!rmse_data) rmse_data = (double*) malloc(3 * sizeof(double));
}



// ============================================================================
// Variabel Benchmark & Statistik Komputasi
// ============================================================================
uint32_t runCount = 0;
static SemaphoreHandle_t benchmarkDoneSem = nullptr;
static volatile bool benchmarkRunning = false;

// Stack solver NMPC besar; dengan PSRAM stack eksternal, 512 KB aman.
static constexpr uint32_t NMPC_TASK_STACK_BYTES = 512 * 1024;

struct BenchmarkStats {
  uint32_t totalDurationUs;
  float totalDurationMs;
  float avgStepMs;
  float minStepMs;
  float maxStepMs;
  uint32_t freeHeapBefore;
  uint32_t freeHeapAfter;
  uint32_t freePsramBefore;
  uint32_t freePsramAfter;
};

void printSystemInfo();
void runNMPCBenchmark();
void nmpcBenchmarkTask(void *param);
static StackType_t *nmpcTaskStack = nullptr;
static StaticTask_t nmpcTaskBuffer;

static bool createNmpcBenchmarkTask() {
  const uint32_t stackWords = NMPC_TASK_STACK_BYTES / sizeof(StackType_t);

  if (!nmpcTaskStack) {
    nmpcTaskStack = static_cast<StackType_t *>(
      heap_caps_malloc(stackWords * sizeof(StackType_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!nmpcTaskStack) {
      Serial.printf("[ERROR] Alokasi stack PSRAM %u byte gagal.\n", NMPC_TASK_STACK_BYTES);
      return false;
    }
  }

  if (xTaskCreateStaticPinnedToCore(
        nmpcBenchmarkTask,
        "nmpc_bench",
        stackWords,
        nullptr,
        1,
        nmpcTaskStack,
        &nmpcTaskBuffer,
        1) == nullptr) {
    Serial.println("[ERROR] Gagal membuat task benchmark.");
    return false;
  }

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(3000); // Waktu tunggu pembukaan Serial Monitor

  Serial.println("\n=======================================================");
  Serial.println("   PENGUJAN PERFORMA KOMPUTASI REAL-TIME NMPC ESP32-S3  ");
  Serial.println("=======================================================");

  // 1. Cek Informasi Sistem dan Memori
  printSystemInfo();

  // 2. Inisialisasi Buffers Memori PSRAM & Solver NMPC
  initBuffers();
  benchmarkDoneSem = xSemaphoreCreateBinary();
  Serial.println("\n[INIT] Menginisialisasi Solver NMPC...");
  simulate_nmpc_kapal_initialize();
  Serial.println("[INIT] Solver NMPC Berhasil Diinisialisasi.");
  
  Serial.println("\nMemulai pengujian komputasi real-time...");
  Serial.println("-------------------------------------------------------");
}

void loop() {
  if (benchmarkRunning) {
    return;
  }

  runCount++;
  Serial.printf("\n>>> MEMULAI RUN BENCHMARK NMPC #%u <<<\n", runCount);

  benchmarkRunning = true;
  if (!createNmpcBenchmarkTask()) {
    benchmarkRunning = false;
    delay(5000);
    return;
  }

  xSemaphoreTake(benchmarkDoneSem, portMAX_DELAY);

  Serial.println("\n[WAIT] Menunggu 5 detik sebelum siklus pengujian berikutnya...");
  delay(5000);
}

void nmpcBenchmarkTask(void *param) {
  (void)param;
  runNMPCBenchmark();
  benchmarkRunning = false;
  xSemaphoreGive(benchmarkDoneSem);
  vTaskDelete(nullptr);
}

void printSystemInfo() {
  Serial.printf("CPU Frequency       : %u MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("SDK Version         : %s\n", ESP.getSdkVersion());
  Serial.printf("Total Heap          : %u bytes\n", ESP.getHeapSize());
  Serial.printf("Free Heap           : %u bytes\n", ESP.getFreeHeap());

  if (psramInit()) {
    Serial.printf("Status PSRAM        : TERDETEKSI DAN AKTIF\n");
    Serial.printf("Total PSRAM Size    : %u bytes (%.2f MB)\n", ESP.getPsramSize(), ESP.getPsramSize() / 1048576.0);
    Serial.printf("Free PSRAM          : %u bytes (%.2f MB)\n", ESP.getFreePsram(), ESP.getFreePsram() / 1048576.0);
  } else {
    Serial.println("Status PSRAM        : TIDAK TERDETEKSI / TIDAK AKTIF");
  }
}

void runNMPCBenchmark() {
  BenchmarkStats stats;
  stats.freeHeapBefore = ESP.getFreeHeap();
  stats.freePsramBefore = ESP.getFreePsram();

  // 1. Catat waktu mulai (Presisi tinggi mikrodetik)
  uint64_t startTimeUs = esp_timer_get_time();

  // 2. Eksekusi Simulasi NMPC (150 Langkah Horizon Optimasi)
  simulate_nmpc_kapal(hist_dim, history_input, time_vector, rmse_data);

  // 3. Catat waktu selesai
  uint64_t endTimeUs = esp_timer_get_time();

  stats.freeHeapAfter = ESP.getFreeHeap();
  stats.freePsramAfter = ESP.getFreePsram();

  // 4. Kalkulasi Statistik Performa Waktu
  stats.totalDurationUs = (uint32_t)(endTimeUs - startTimeUs);
  stats.totalDurationMs = stats.totalDurationUs / 1000.0f;
  
  // Total steps = 150
  const int TOTAL_STEPS = 150;
  stats.avgStepMs = stats.totalDurationMs / TOTAL_STEPS;

  // 5. Cetak Laporan Performa Komputasi Real-time
  Serial.println("\n----------------- HASIL BENCHMARK NMPC -----------------");
  Serial.printf("Total Waktu Simulasi (150 Langkah) : %.2f ms (%.3f detik)\n", 
                stats.totalDurationMs, stats.totalDurationMs / 1000.0f);
  Serial.printf("Rata-rata Waktu per Step NMPC       : %.2f ms (%.2f Hz)\n", 
                stats.avgStepMs, 1000.0f / stats.avgStepMs);
  Serial.printf("Total Waktu Komputasi (us)          : %u us\n", stats.totalDurationUs);

  Serial.println("\n----------------- AKURASI KONTROL (RMSE) ----------------");
  Serial.printf("RMSE X   (Posisi X)  : %.4f meter\n", rmse_data[0]);
  Serial.printf("RMSE Y   (Posisi Y)  : %.4f meter\n", rmse_data[1]);
  Serial.printf("RMSE Psi (Heading)   : %.4f rad (%.2f deg)\n", rmse_data[2], rmse_data[2] * RAD_TO_DEG);

  Serial.println("\n----------------- ANALISIS MEMORI (BYTES) ---------------");
  Serial.printf("Free Heap (Sebelum -> Sesudah) : %u -> %u bytes\n", 
                stats.freeHeapBefore, stats.freeHeapAfter);
  if (ESP.getPsramSize() > 0) {
    Serial.printf("Free PSRAM (Sebelum -> Sesudah): %u -> %u bytes\n", 
                  stats.freePsramBefore, stats.freePsramAfter);
  }

  Serial.println("\n----------------- SAMPEL OUTPUT KONTROL ----------------");
  Serial.printf("Input Kontrol Rudder Pertama (Step 1) : %.2f deg\n", history_input[0] * RAD_TO_DEG);
  Serial.printf("Input Kontrol Rudder Akhir (Step 150): %.2f deg\n", history_input[149] * RAD_TO_DEG);
  Serial.printf("Posisi Akhir Kapal (X, Y, Psi)        : X=%.2f m, Y=%.2f m, Psi=%.2f deg\n", 
                hist_dim[2 * 151 + 150], hist_dim[3 * 151 + 150], hist_dim[4 * 151 + 150] * RAD_TO_DEG);
  Serial.println("-------------------------------------------------------");
}