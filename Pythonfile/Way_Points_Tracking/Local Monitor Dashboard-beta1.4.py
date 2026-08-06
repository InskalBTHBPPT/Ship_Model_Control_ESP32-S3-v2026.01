"""
Local Monitor Dashboard beta 1.4

Ringkasan:
- Dashboard PySide6 untuk monitoring telemetry kapal secara real-time.
- Input data berasal dari serial CSV text (UTF-8) lewat User-Side-04.
- Mendukung map tracking, indikator live, plotting time-series, logging CSV,
  analyze, Home Points, serta Send Way Points ke Remote via User-Side.

Format data serial yang dibaca (24 kolom, raw fixed-point dari User-Side-04):
1) timestamp  2) latitude  3) longitude
4) speedMps (x100)  5-6) Calc_deg_servo_1/2 (x100, °)
7) yaw (x100)  8) heading_setpoint (x100)  9) heading_error (x100)
10) rudder_cmd (x100)  11) track_wp_index  12) distance_to_wp (x10, m)
13-18) accel_x/y/z, gyro_x/y/z (x100) — di-log, tidak di panel live
19-20) rpm_prop_1/2 (integer RPM langsung, bukan x100)
21-22) battery_1/2 (x100, V)  23) mode_auto  24) mini_pc_link (0/1)

Send Way Points (tab Map Points):
- Tombol mengirim ASCII:
  $WPSET,<home_lat>,<home_lon>,<wp_count>,<lat1>,<lon1>,...,<latN>,<lonN>
- User-Side membalas $WACK,OK / $WACK,ERR,...
- User-Side meneruskan ke Remote sebagai ESP-NOW waypoints_payload (0xA1).
- Remote mencetak [WP] ... ke USB Serial mini PC; Cpp_ReadWriteSerial dapat
  menampilkan baris itu (--print all|wp). Dashboard tidak bicara langsung ke mini PC.

Catatan pengolahan:
- Parser memproses baris utuh yang diakhiri newline dan memvalidasi
  jumlah kolom = TELEMETRY_COL_COUNT (24).
- RPM: nilai mentah dari Remote-Side = putaran/menit langsung.
- Data lat/lon tervalidasi range; nilai 0,0 dapat diganti default location.
- Nilai terbaru lat/lon disimpan untuk fitur Home Points.
"""

import csv
import io
import os
import sys
from bisect import bisect_left
from datetime import datetime

TELEMETRY_COL_COUNT = 24

TELEMETRY_LOG_HEADER = (
    "timestamp (s),latitude (°),longitude (°),speedMps (m/s),Calc_deg_servo_1 (°),Calc_deg_servo_2 (°),"
    "yaw (°),heading_setpoint (°),heading_error (°),rudder_cmd (°),track_wp_index,distance_to_wp (m),"
    "accel_x (g),accel_y (g),accel_z (g),gyro_x (deg/s),gyro_y (deg/s),gyro_z (deg/s),"
    "rpm_prop_1 (rpm),rpm_prop_2 (rpm),battery_1 (V),battery_2 (V),mode_auto,mini_pc_link\n"
)


def _build_telemetry_log_line(
    timestamp: float,
    lat: float,
    lon: float,
    speed: float,
    rud1_sensor: float,
    rud2_sensor: float,
    heading: float,
    heading_setpoint: float,
    heading_error: float,
    rudder_cmd: float,
    track_wp_index: int,
    distance_to_wp: float,
    accel_x: float,
    accel_y: float,
    accel_z: float,
    gyro_x: float,
    gyro_y: float,
    gyro_z: float,
    rpm1: float,
    rpm2: float,
    bat1: float,
    bat2: float,
    mode_auto: int,
    mini_pc_link: int = 0,
) -> str:
    """Baris CSV log — nilai tampilan (sama seperti panel Live)."""
    return (
        f"{timestamp:.3f},{lat:.6f},{lon:.6f},"
        f"{speed:.2f},"
        f"{rud1_sensor:.2f},{rud2_sensor:.2f},"
        f"{heading:.2f},{heading_setpoint:.2f},{heading_error:.2f},{rudder_cmd:.2f},"
        f"{track_wp_index},{distance_to_wp:.1f},"
        f"{accel_x:.2f},{accel_y:.2f},{accel_z:.2f},"
        f"{gyro_x:.2f},{gyro_y:.2f},{gyro_z:.2f},"
        f"{rpm1:.0f},{rpm2:.0f},"
        f"{bat1:.2f},{bat2:.2f},"
        f"{mode_auto},{mini_pc_link}\n"
    )


def _telemetry_scale(parts: list[str], idx: int, divisor: float = 100.0) -> float:
    return float(parts[idx]) / divisor


def _telemetry_rpm(parts: list[str], idx: int) -> int:
    """RPM dari User-Side: integer langsung (bukan fixed-point x100)."""
    return int(float(parts[idx]))


def _format_track_wp_index(raw: int) -> str:
    if raw == 0:
        return "—"
    if raw == 255:
        return "Home"
    return f"WP{raw}"


def _csv_row_value(row: dict, *keys: str, default: str = "0") -> str:
    for key in keys:
        if key in row and row[key] is not None and str(row[key]).strip() != "":
            return str(row[key]).strip()
    return default


def _detect_analyze_csv_format(fields: set[str]) -> str:
    if "heading_setpoint (raw)" in fields:
        return "raw_v23"
    if "speedMps (m/s)" in fields:
        return "display_v23"
    if "heading_setpoint (°)" in fields and "rudder_cmd (°)" in fields:
        return "display_v23"
    return "legacy"


def _parse_analyze_csv_row(row: dict, fmt: str) -> dict | None:
    """Normalisasi satu baris CSV Analyze ke dict nilai tampilan."""
    try:
        timestamp = float(_csv_row_value(row, "timestamp (s)", "timestamp"))
        lat = float(_csv_row_value(row, "latitude (°)", "latitude"))
        lon = float(_csv_row_value(row, "longitude (°)", "longitude"))
        if fmt == "raw_v23":
            speed = float(_csv_row_value(row, "speedMps (raw)", "speedMps")) / 100.0
            rud1 = float(_csv_row_value(row, "Calc_deg_servo_1 (raw)", "Calc_deg_servo_1")) / 100.0
            rud2 = float(_csv_row_value(row, "Calc_deg_servo_2 (raw)", "Calc_deg_servo_2")) / 100.0
            yaw = float(_csv_row_value(row, "yaw (raw)", "yaw")) / 100.0
            heading_setpoint = float(_csv_row_value(row, "heading_setpoint (raw)", "heading_setpoint")) / 100.0
            heading_error = float(_csv_row_value(row, "heading_error (raw)", "heading_error")) / 100.0
            rudder_cmd = float(_csv_row_value(row, "rudder_cmd (raw)", "rudder_cmd")) / 100.0
            track_wp_index = int(float(_csv_row_value(row, "track_wp_index", default="0")))
            distance_to_wp = float(_csv_row_value(row, "distance_to_wp (raw)", "distance_to_wp")) / 10.0
            rpm1 = int(float(_csv_row_value(row, "rpm_prop_1 (raw)", "rpm_prop_1")))
            rpm2 = int(float(_csv_row_value(row, "rpm_prop_2 (raw)", "rpm_prop_2")))
            bat1 = float(_csv_row_value(row, "battery_1 (raw)", "battery_1")) / 100.0
            bat2 = float(_csv_row_value(row, "battery_2 (raw)", "battery_2")) / 100.0
        elif fmt == "display_v23":
            speed = float(_csv_row_value(row, "speedMps (m/s)", "speedMps"))
            rud1 = float(_csv_row_value(row, "Calc_deg_servo_1 (°)", "Calc_deg_servo_1"))
            rud2 = float(_csv_row_value(row, "Calc_deg_servo_2 (°)", "Calc_deg_servo_2"))
            yaw = float(_csv_row_value(row, "yaw (°)", "yaw"))
            heading_setpoint = float(_csv_row_value(row, "heading_setpoint (°)", "heading_setpoint"))
            heading_error = float(_csv_row_value(row, "heading_error (°)", "heading_error"))
            rudder_cmd = float(_csv_row_value(row, "rudder_cmd (°)", "rudder_cmd"))
            track_wp_index = int(float(_csv_row_value(row, "track_wp_index", default="0")))
            distance_to_wp = float(_csv_row_value(row, "distance_to_wp (m)", "distance_to_wp"))
            rpm1 = float(_csv_row_value(row, "rpm_prop_1 (rpm)", "rpm_prop_1"))
            rpm2 = float(_csv_row_value(row, "rpm_prop_2 (rpm)", "rpm_prop_2"))
            bat1 = float(_csv_row_value(row, "battery_1 (V)", "battery_1"))
            bat2 = float(_csv_row_value(row, "battery_2 (V)", "battery_2"))
        else:
            speed = float(_csv_row_value(row, "speedMps"))
            rud1 = float(_csv_row_value(row, "Calc_deg_servo_1 (°)", "Calc_deg_servo_1"))
            rud2 = float(_csv_row_value(row, "Calc_deg_servo_2 (°)", "Calc_deg_servo_2"))
            yaw = float(_csv_row_value(row, "yaw (°)", "yaw"))
            heading_setpoint = float(_csv_row_value(row, "zigzag_yaw (°)", "zigzag_yaw"))
            heading_error = 0.0
            rudder_cmd = 0.0
            track_wp_index = 0
            distance_to_wp = 0.0
            rpm1 = float(_csv_row_value(row, "rpm_prop_1 (rpm)", "rpm_prop_1"))
            rpm2 = float(_csv_row_value(row, "rpm_prop_2 (rpm)", "rpm_prop_2"))
            bat1 = float(_csv_row_value(row, "battery_1 (V)", "battery_1"))
            bat2 = float(_csv_row_value(row, "battery_2 (V)", "battery_2"))
        mode_auto = int(float(_csv_row_value(row, "mode_auto", default="0")))
        return {
            "timestamp": timestamp,
            "lat": lat,
            "lon": lon,
            "speed": speed,
            "rud1_sensor": rud1,
            "rud2_sensor": rud2,
            "yaw": yaw,
            "heading_setpoint": heading_setpoint,
            "heading_error": heading_error,
            "rudder_cmd": rudder_cmd,
            "track_wp_index": track_wp_index,
            "distance_to_wp": distance_to_wp,
            "rpm1": rpm1,
            "rpm2": rpm2,
            "bat1": bat1,
            "bat2": bat2,
            "mode_auto": mode_auto,
        }
    except (ValueError, TypeError):
        return None

import folium
import serial
from serial.tools import list_ports
from PySide6.QtCore import Qt, QTimer, QUrl
from PySide6.QtWebEngineWidgets import QWebEngineView
from PySide6.QtWebEngineCore import QWebEnginePage
from PySide6.QtWidgets import (
    QApplication,
    QMainWindow,
    QVBoxLayout,
    QHBoxLayout,
    QWidget,
    QPushButton,
    QComboBox,
    QLabel,
    QGridLayout,
    QGroupBox,
    QFileDialog,
    QMessageBox,
    QProgressBar,
    QTabWidget,
    QCheckBox,
    QSlider,
    QTableWidget,
    QTableWidgetItem,
    QHeaderView,
    QLineEdit,
    QFormLayout,
    QSplitter,
    QDialog,
    QDialogButtonBox,
    QDoubleSpinBox,
)
import pyqtgraph as pg
from time import time, strftime


def _make_live_stat_cell(parent: QWidget, title: str, value_label: QLabel) -> QWidget:
    """Satu sel indikator live: judul di atas, nilai di bawah."""
    cell = QWidget(parent)
    layout = QVBoxLayout(cell)
    layout.setContentsMargins(4, 2, 4, 2)
    layout.setSpacing(2)
    title_lbl = QLabel(title, cell)
    title_lbl.setStyleSheet("color: #9ca3af; font-size: 10pt;")
    title_lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
    value_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
    layout.addWidget(title_lbl)
    layout.addWidget(value_label)
    return cell


class LiveRudderSetupDialog(QDialog):
  """Dialog koreksi rudder untuk tab Live Data (display + log saja)."""

  CORRECTION_TOOLTIP = (
      "Rentang input ±5°.\n"
      "Koreksi hanya berlaku untuk nilai di dashboard dan file log CSV "
      "setelah data diterima via serial.\n"
      "Tidak mengubah data yang dikirim Remote-Side / User-Side."
  )

  def __init__(self, parent: "MainWindow"):
      super().__init__(parent)
      self.setWindowTitle("Live Data Setup — Rudder Correction")
      self.setMinimumWidth(420)

      layout = QVBoxLayout(self)

      info = QLabel(self.CORRECTION_TOOLTIP, self)
      info.setWordWrap(True)
      info.setStyleSheet("color: #000000; font-size: 10pt; padding: 4px 0 8px 0;")
      layout.addWidget(info)

      form = QFormLayout()
      form.setSpacing(10)

      self.cb_rud1 = QCheckBox("Rudder 1 correction", self)
      self.sp_rud1 = QDoubleSpinBox(self)
      self.sp_rud1.setRange(-5.0, 5.0)
      self.sp_rud1.setDecimals(2)
      self.sp_rud1.setSingleStep(0.1)
      self.sp_rud1.setToolTip(self.CORRECTION_TOOLTIP)

      self.cb_rud2 = QCheckBox("Rudder 2 correction", self)
      self.sp_rud2 = QDoubleSpinBox(self)
      self.sp_rud2.setRange(-5.0, 5.0)
      self.sp_rud2.setDecimals(2)
      self.sp_rud2.setSingleStep(0.1)
      self.sp_rud2.setToolTip(self.CORRECTION_TOOLTIP)

      self.cb_rud_cmd = QCheckBox("Rudder cmd correction", self)
      self.sp_rud_cmd = QDoubleSpinBox(self)
      self.sp_rud_cmd.setRange(-5.0, 5.0)
      self.sp_rud_cmd.setDecimals(2)
      self.sp_rud_cmd.setSingleStep(0.1)
      self.sp_rud_cmd.setToolTip(self.CORRECTION_TOOLTIP)

      for cb, sp in (
          (self.cb_rud1, self.sp_rud1),
          (self.cb_rud2, self.sp_rud2),
          (self.cb_rud_cmd, self.sp_rud_cmd),
      ):
          row = QWidget(self)
          row_layout = QHBoxLayout(row)
          row_layout.setContentsMargins(0, 0, 0, 0)
          row_layout.addWidget(cb)
          row_layout.addWidget(sp)
          form.addRow(row)
          sp.setEnabled(cb.isChecked())
          cb.toggled.connect(sp.setEnabled)

      layout.addLayout(form)

      buttons = QDialogButtonBox(
          QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel,
          self,
      )
      buttons.accepted.connect(self.accept)
      buttons.rejected.connect(self.reject)
      layout.addWidget(buttons)

      self._load_from_parent(parent)

  def _load_from_parent(self, parent: "MainWindow") -> None:
      self.cb_rud1.setChecked(parent.rudder1_correction_enabled)
      self.sp_rud1.setValue(parent.rudder1_correction_value)
      self.cb_rud2.setChecked(parent.rudder2_correction_enabled)
      self.sp_rud2.setValue(parent.rudder2_correction_value)
      self.cb_rud_cmd.setChecked(parent.rudder_cmd_correction_enabled)
      self.sp_rud_cmd.setValue(parent.rudder_cmd_correction_value)
      self.sp_rud1.setEnabled(self.cb_rud1.isChecked())
      self.sp_rud2.setEnabled(self.cb_rud2.isChecked())
      self.sp_rud_cmd.setEnabled(self.cb_rud_cmd.isChecked())

  def apply_to_parent(self, parent: "MainWindow") -> None:
      parent.rudder1_correction_enabled = self.cb_rud1.isChecked()
      parent.rudder1_correction_value = self.sp_rud1.value()
      parent.rudder2_correction_enabled = self.cb_rud2.isChecked()
      parent.rudder2_correction_value = self.sp_rud2.value()
      parent.rudder_cmd_correction_enabled = self.cb_rud_cmd.isChecked()
      parent.rudder_cmd_correction_value = self.sp_rud_cmd.value()


class ClickableMapPage(QWebEnginePage):
    """
    Custom QWebEnginePage untuk menangani klik pada peta.
    """
    def __init__(self, parent=None, click_callback=None):
        super().__init__(parent)
        self.click_callback = click_callback
    
    def acceptNavigationRequest(self, url, nav_type, is_main_frame):
        """
        Intercept navigation request untuk menangani custom URL scheme.
        """
        if url.scheme() == 'python' and self.click_callback:
            # Format: python://click?lat=xxx&lon=xxx
            if url.host() == 'click':
                query = url.query()
                params = {}
                for param in query.split('&'):
                    if '=' in param:
                        key, value = param.split('=', 1)
                        params[key] = value
                if 'lat' in params and 'lon' in params:
                    try:
                        lat = float(params['lat'])
                        lon = float(params['lon'])
                        self.click_callback(lat, lon)
                    except ValueError:
                        pass
            return False
        return super().acceptNavigationRequest(url, nav_type, is_main_frame)


class MapWebView(QWebEngineView):
    """
    WebView widget untuk menampilkan peta interaktif menggunakan Folium.
    
    Class ini menangani:
    - Pembuatan peta Folium dengan Google Hybrid tile
    - Penambahan marker dan trail untuk tracking posisi kapal
    - Update heading indicator untuk menunjukkan arah heading
    - Clear markers dan trail
    
    Attributes:
        folium_map: Objek Folium Map
        trail_coords: List koordinat untuk trail
        marker_count: Counter untuk jumlah marker
        data: BytesIO buffer untuk menyimpan HTML peta
    """
    def __init__(self, initial_coordinates: tuple[float, float], custom_page=None):
        """
        Inisialisasi MapWebView dengan koordinat awal.
        
        Args:
            initial_coordinates: Tuple (latitude, longitude) untuk posisi awal peta
            custom_page: Optional QWebEnginePage untuk custom page handling
        """
        super().__init__()
        
        # Set custom page jika diberikan (sebelum membuat folium map)
        if custom_page:
            self.setPage(custom_page)
        
        self.folium_map = folium.Map(
            location=initial_coordinates,
            zoom_start=18,
            zoom_control=True,
            attribution_control=True,
            tiles=None  # no default OSM; we'll add Google Hybrid as default
        )
        
        print(f"[MAP] Zoom Level: 18 | Coverage: ~200-500 m | Details: High Detail")
        print(f"[MAP] Max Zoom: 21 | Leaflet.js native support")
        
        # Add satellite tile layers
        self.add_tile_layers()
        
        # Track coordinates for trail
        self.trail_coords = [initial_coordinates]
        self.marker_count = 0
        
        self.data = io.BytesIO()
        self.folium_map.save(self.data, close_file=False)
        self.setHtml(self.data.getvalue().decode())
        
        # Add initial marker after HTML is loaded
        from PySide6.QtCore import QTimer
        QTimer.singleShot(1000, lambda: self.add_initial_marker(initial_coordinates))
    
    def add_tile_layers(self):
        """
        Menambahkan tile layers ke peta.
        
        Saat ini hanya menggunakan Google Hybrid (satelit + label) sebagai base layer.
        Tile layers lain (Google Satellite, Bing Satellite, dll) di-comment out.
        """
        # # Google Satellite
        # folium.TileLayer(
        #     tiles='https://mt1.google.com/vt/lyrs=s&x={x}&y={y}&z={z}',
        #     attr='Google',
        #     name='🌍 Google Satellite',
        #     overlay=False,
        #     control=True,
        #     max_zoom=21,
        #     maxNativeZoom=21
        # ).add_to(self.folium_map)
        
        # # Bing Satellite (High Resolution)
        # folium.TileLayer(
        #     tiles='http://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}',
        #     attr='Bing',
        #     name='🛰️ Bing Satellite',
        #     overlay=False,
        #     control=True,
        #     max_zoom=20,
        #     maxNativeZoom=20
        # ).add_to(self.folium_map)
        
        # # CartoDB Dark Matter
        # folium.TileLayer(
        #     tiles='https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png',
        #     attr='CartoDB',
        #     name='🌙 Dark Mode',
        #     overlay=False,
        #     control=True,
        #     max_zoom=19,
        #     maxNativeZoom=19
        # ).add_to(self.folium_map)
        
        # # Esri Satellite
        # folium.TileLayer(
        #     tiles='https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}',
        #     attr='Esri',
        #     name='🛰️ Esri Satellite',
        #     overlay=False,
        #     control=True,
        #     max_zoom=20,
        #     maxNativeZoom=20
        # ).add_to(self.folium_map)
        
        # Google Hybrid (Satellite + Labels) - Default and only visible base layer
        folium.TileLayer(
            tiles='https://mt1.google.com/vt/lyrs=y&x={x}&y={y}&z={z}',
            attr='Google',
            name='🗺️ Google Hybrid',
            overlay=False,
            control=True,
            max_zoom=21,
            maxNativeZoom=21
        ).add_to(self.folium_map)
        
        # Add layer control
        folium.LayerControl(position='topright').add_to(self.folium_map)
        
        # quiet: removed verbose print
    
    def add_initial_marker(self, coords: tuple[float, float], heading: float | None = None):
        """Add initial marker using JavaScript"""
        self.marker_count += 1
        map_name = self.folium_map.get_name()
        
        # Build popup content
        if heading is not None:
            popup_content = f'🚢 Start Position\\nLat: {coords[0]:.6f}\\nLon: {coords[1]:.6f}\\nHeading: {heading:.1f}°'
        else:
            popup_content = f'🚢 Start Position\\nLat: {coords[0]:.6f}\\nLon: {coords[1]:.6f}'
        
        js_code = f"""
        // Add initial marker
        var startMarker = L.marker({list(coords)})
            .addTo({map_name})
            .bindPopup('{popup_content}')
            .bindTooltip('Start Point');
        window.startMarker = startMarker;
        
        // Create marker group for trail markers
        window.trailMarkers = L.layerGroup().addTo({map_name});
        
        // Create polyline for trail (menghubungkan semua points)
        window.trailLine = L.polyline([{list(coords)}], {{
            color: '#3b82f6',
            weight: 3,
            opacity: 0.8,
            lineCap: 'round',
            lineJoin: 'round'
        }}).addTo({map_name});
        window.trailLine.bringToFront();
        
        console.log('✅ Initial marker and trail line created');
        """
        
        self.page().runJavaScript(js_code)
        # quiet
    
    def add_marker_js(self, coords: tuple[float, float], heading: float | None = None):
        """Add marker using JavaScript without regenerating HTML"""
        self.marker_count += 1
        self.trail_coords.append(coords)
        
        map_name = self.folium_map.get_name()
        
        # Build popup content
        if heading is not None:
            popup_content = f'📍 Point {self.marker_count}\\nLat: {coords[0]:.6f}\\nLon: {coords[1]:.6f}\\nHeading: {heading:.1f}°'
        else:
            popup_content = f'📍 Point {self.marker_count}\\nLat: {coords[0]:.6f}\\nLon: {coords[1]:.6f}'
        
        # Step 1 (self-heal): bungkus dengan IIFE + guard. Kalau Leaflet/L, peta,
        # atau window.trailMarkers belum siap (mis. add_initial_marker gagal di
        # awal karena race condition QTimer.singleShot vs page load), kita
        # buat sendiri di sini agar marker tetap tampil dan tidak melempar error.
        js_code = f"""
        (function() {{
            if (typeof L === 'undefined') {{
                console.warn('Leaflet (L) not ready, skip add_marker_js');
                return;
            }}
            if (typeof {map_name} === 'undefined' || !{map_name}) {{
                console.warn('Map {map_name} not ready, skip add_marker_js');
                return;
            }}
            if (!window.trailMarkers) {{
                window.trailMarkers = L.layerGroup().addTo({map_name});
            }}

            var newMarker = L.marker({list(coords)})
                .bindPopup('{popup_content}')
                .bindTooltip('Point {self.marker_count}');
            window.trailMarkers.addLayer(newMarker);

            if (!window.trailLine) {{
                window.trailLine = L.polyline([{list(coords)}], {{
                    color: '#3b82f6', weight: 3, opacity: 0.8,
                    lineCap: 'round', lineJoin: 'round'
                }}).addTo({map_name});
            }}

            var allCoords = {[list(coord) for coord in self.trail_coords]};
            window.trailLine.setLatLngs(allCoords);
            window.trailLine.bringToFront();

            console.log('Marker {self.marker_count} added at {list(coords)} | Trail points: {len(self.trail_coords)}');
        }})();
        """
        
        self.page().runJavaScript(js_code)
        # quiet

    def update_heading_line(self, origin: tuple[float, float], heading_deg: float, length_m: float = 5.0):
        """
        Create atau update garis heading dari posisi saat ini.
        
        Args:
            origin: Tuple (latitude, longitude) untuk posisi awal
            heading_deg: Heading dalam derajat (0-360°)
            length_m: Panjang garis heading dalam meter (default: 20.0 m)
            
        Method ini:
        - Menghitung koordinat tujuan berdasarkan heading dan panjang
        - Membuat atau update polyline untuk heading indicator
        - Menggunakan warna merah untuk heading line
        """
        import math
        lat1 = math.radians(origin[0])
        lon1 = math.radians(origin[1])
        brng = math.radians(heading_deg % 360.0)
        R = 6371000.0
        dR = length_m / R
        lat2 = math.asin(math.sin(lat1) * math.cos(dR) + math.cos(lat1) * math.sin(dR) * math.cos(brng))
        lon2 = lon1 + math.atan2(math.sin(brng) * math.sin(dR) * math.cos(lat1), math.cos(dR) - math.sin(lat1) * math.sin(lat2))
        dest = (math.degrees(lat2), math.degrees(lon2))

        map_name = self.folium_map.get_name()
        js_code = f"""
        (function() {{
          var pts = [{list(origin)}, {list(dest)}];
          if (!window.headingLine) {{
            window.headingLine = L.polyline(pts, {{ color: 'red', weight: 4, opacity: 0.9 }}).addTo({map_name});
          }} else {{
            window.headingLine.setLatLngs(pts);
          }}
        }})();
        """
        self.page().runJavaScript(js_code)

    def add_heading_line_segment(self, origin: tuple[float, float], heading_deg: float, length_m: float = 5.0):
        """
        Add heading line segment (used for Analyze tab to draw multiple headings).

        Args:
            origin: Tuple (lat, lon)
            heading_deg: heading in degrees
            length_m: length of line
        """
        import math
        lat1 = math.radians(origin[0])
        lon1 = math.radians(origin[1])
        brng = math.radians(heading_deg % 360.0)
        R = 6371000.0
        dR = length_m / R
        lat2 = math.asin(math.sin(lat1) * math.cos(dR) + math.cos(lat1) * math.sin(dR) * math.cos(brng))
        lon2 = lon1 + math.atan2(math.sin(brng) * math.sin(dR) * math.cos(lat1), math.cos(dR) - math.sin(lat1) * math.sin(lat2))
        dest = (math.degrees(lat2), math.degrees(lon2))

        map_name = self.folium_map.get_name()
        js_code = f"""
        (function() {{
          var mapRef = {map_name};
          if (!window.headingLinesGroup) {{
            window.headingLinesGroup = L.layerGroup().addTo(mapRef);
            window.headingLineLayers = [];
          }}
          var pts = [{list(origin)}, {list(dest)}];
          var seg = L.polyline(pts, {{ color: 'red', weight: 3, opacity: 0.9 }}).addTo(window.headingLinesGroup);
          window.headingLineLayers.push(seg);
        }})();
        """
        self.page().runJavaScript(js_code)
    
    def move_start_marker(self, coords: tuple[float, float], heading: float | None = None, timestamp: float | None = None):
        """
        Pindahkan marker awal (start marker) ke koordinat baru dan update popup/tooltip.
        """
        map_name = self.folium_map.get_name()
        popup_lines = [
            f"t = {timestamp:.3f} s" if timestamp is not None else None,
            f"Lat: {coords[0]:.6f}",
            f"Lon: {coords[1]:.6f}",
            f"Heading: {heading:.1f}°" if heading is not None else None
        ]
        popup_content = "\\n".join([line for line in popup_lines if line])
        tooltip_content = popup_content.replace("\\n", ", ")
        popup_js = popup_content.replace("\\", "\\\\").replace("'", "\\'")
        tooltip_js = tooltip_content.replace("\\", "\\\\").replace("'", "\\'")
        js_code = f"""
        (function() {{
          var coords = {list(coords)};
          var popupContent = '{popup_js}';
          var tooltipContent = '{tooltip_js}';
          if (window.startMarker) {{
            window.startMarker.setLatLng(coords);
            var popup = window.startMarker.getPopup();
            if (popup) {{
              popup.setContent(popupContent);
            }} else {{
              window.startMarker.bindPopup(popupContent);
            }}
            window.startMarker.unbindTooltip();
            window.startMarker.bindTooltip(tooltipContent, {{
              direction: 'top',
              opacity: 0.95,
              sticky: true,
              className: 'active-marker-tooltip',
              offset: [0, -30]
            }});
          }} else {{
            window.startMarker = L.marker(coords)
              .addTo({map_name})
              .bindPopup(popupContent)
              .bindTooltip(tooltipContent, {{
                direction: 'top',
                opacity: 0.95,
                sticky: true,
                className: 'active-marker-tooltip',
                offset: [0, -30]
              }});
          }}
          if (window.startMarker && window.startMarker.setOpacity) {{
            window.startMarker.setOpacity(1);
          }}
          if (window.startMarker && window.startMarker.openTooltip) {{
            window.startMarker.openTooltip();
          }}
        }})();
        """
        self.page().runJavaScript(js_code)

    def update_slider_heading_line(self, origin: tuple[float, float] | None, heading_deg: float | None, length_m: float = 5.0):
        """
        Update garis heading khusus pointer slider Analyze.
        """
        map_name = self.folium_map.get_name()
        if origin is None or heading_deg is None:
            js_code = f"""
            (function() {{
              if (window.sliderHeadingLine) {{
                {map_name}.removeLayer(window.sliderHeadingLine);
                window.sliderHeadingLine = null;
              }}
            }})();
            """
            self.page().runJavaScript(js_code)
            return

        import math
        lat1 = math.radians(origin[0])
        lon1 = math.radians(origin[1])
        brng = math.radians(heading_deg % 360.0)
        R = 6371000.0
        dR = length_m / R
        lat2 = math.asin(math.sin(lat1) * math.cos(dR) + math.cos(lat1) * math.sin(dR) * math.cos(brng))
        lon2 = lon1 + math.atan2(math.sin(brng) * math.sin(dR) * math.cos(lat1), math.cos(dR) - math.sin(lat1) * math.sin(lat2))
        dest = (math.degrees(lat2), math.degrees(lon2))

        js_code = f"""
        (function() {{
          var pts = [{list(origin)}, {list(dest)}];
          if (window.sliderHeadingLine) {{
            window.sliderHeadingLine.setLatLngs(pts);
          }} else {{
            window.sliderHeadingLine = L.polyline(pts, {{
              color: 'red',
              weight: 4,
              opacity: 0.95,
              dashArray: '6, 6'
            }}).addTo({map_name});
          }}
          window.sliderHeadingLine.bringToFront();
        }})();
        """
        self.page().runJavaScript(js_code)

    def update_map(self, new_coords: tuple[float, float], heading_deg: float | None = None):
        """
        Update peta dengan koordinat baru dan heading.
        
        Args:
            new_coords: Tuple (latitude, longitude) untuk posisi baru
            heading_deg: Heading dalam derajat (optional, default: None)
            
        Method ini:
        - Update posisi peta ke koordinat baru
        - Menambahkan marker untuk posisi baru
        - Update heading indicator jika heading disediakan
        - Memindahkan view peta ke posisi baru
        """
        # Update Python object location
        self.folium_map.location = new_coords
        
        # Add marker for new position (with heading if available)
        self.add_marker_js(new_coords, heading_deg)
        
        # Update heading line if provided
        if heading_deg is not None:
            try:
                self.update_heading_line(new_coords, float(heading_deg))
            except Exception:
                pass
        
        # Move map view to new position
        map_name = self.folium_map.get_name()
        js_code = f'{map_name}.setView({list(new_coords)})'
        self.page().runJavaScript(js_code)
        
        # quiet

    def clear_markers(self):
        """
        Clear semua marker dan trail dari peta.
        
        Method ini:
        - Clear semua trail markers
        - Clear trail line
        - Clear heading line
        - Reset marker counter
        - Menyimpan koordinat awal (jika ada)
        """
        # Reset Python data
        self.trail_coords = [self.trail_coords[0]] if self.trail_coords else []  # Keep initial coord
        self.marker_count = 0
        
        # Clear markers via JavaScript
        map_name = self.folium_map.get_name()
        js_code = f"""
        (function() {{
          // Clear all trail markers
          if (window.trailMarkers) {{
            window.trailMarkers.clearLayers();
          }}
          
          // Clear trail line
          if (window.trailLine) {{
            {map_name}.removeLayer(window.trailLine);
            window.trailLine = null;  // null-kan agar self-heal di add_marker_js bisa create ulang
          }}
          
          // Clear heading line
          if (window.headingLine) {{
            {map_name}.removeLayer(window.headingLine);
            window.headingLine = null;  // null-kan agar update_heading_line bisa create ulang dengan bersih
          }}

          // Clear multiple heading lines (Analyze tab)
          if (window.headingLinesGroup) {{
            window.headingLinesGroup.clearLayers();
            {map_name}.removeLayer(window.headingLinesGroup);
            window.headingLinesGroup = null;
          }}
          window.headingLineLayers = [];
          
          if (window.sliderHeadingLine) {{
            {map_name}.removeLayer(window.sliderHeadingLine);
            window.sliderHeadingLine = null;
          }}

          if (window.startMarker) {{
            {map_name}.removeLayer(window.startMarker);
            window.startMarker = null;
          }}

          if (window.liveWpHomeMarker) {{
            {map_name}.removeLayer(window.liveWpHomeMarker);
            window.liveWpHomeMarker = null;
          }}
          if (window.liveWpMarkers) {{
            window.liveWpMarkers.clearLayers();
            {map_name}.removeLayer(window.liveWpMarkers);
            window.liveWpMarkers = null;
          }}
          if (window.liveWpRouteLine) {{
            {map_name}.removeLayer(window.liveWpRouteLine);
            window.liveWpRouteLine = null;
          }}
          
          console.log('✅ All map markers cleared');
        }})();
        """
        self.page().runJavaScript(js_code)
        print("[MAP] All markers and trails cleared")

    def clear_waypoint_route(self):
        """Hapus layer Home/WP route dari peta (tab Live)."""
        map_name = self.folium_map.get_name()
        js_code = f"""
        (function() {{
          if (window.liveWpHomeMarker) {{
            {map_name}.removeLayer(window.liveWpHomeMarker);
            window.liveWpHomeMarker = null;
          }}
          if (window.liveWpMarkers) {{
            window.liveWpMarkers.clearLayers();
            {map_name}.removeLayer(window.liveWpMarkers);
            window.liveWpMarkers = null;
          }}
          if (window.liveWpRouteLine) {{
            {map_name}.removeLayer(window.liveWpRouteLine);
            window.liveWpRouteLine = null;
          }}
        }})();
        """
        self.page().runJavaScript(js_code)

    def show_waypoint_route(
        self,
        home: tuple[float, float] | None,
        waypoints: list[tuple[float, float]],
    ) -> None:
        """
        Tampilkan Home + waypoint navigasi dan garis rute di peta Live.
        Layer terpisah dari trail posisi kapal (trailLine).
        """
        if home is None and not waypoints:
            self.clear_waypoint_route()
            return

        map_name = self.folium_map.get_name()
        line_coords: list[list[float]] = []
        if home is not None:
            line_coords.append([home[0], home[1]])
        for wp in waypoints:
            line_coords.append([wp[0], wp[1]])

        home_js = "null"
        if home is not None:
            home_js = str([home[0], home[1]])
        wp_coords_js = [[c[0], c[1]] for c in waypoints]

        js_code = f"""
        (function() {{
            if (typeof L === 'undefined' || typeof {map_name} === 'undefined') return;

            if (window.liveWpHomeMarker) {{
                {map_name}.removeLayer(window.liveWpHomeMarker);
                window.liveWpHomeMarker = null;
            }}
            if (window.liveWpMarkers) {{
                window.liveWpMarkers.clearLayers();
            }} else {{
                window.liveWpMarkers = L.layerGroup().addTo({map_name});
            }}
            if (window.liveWpRouteLine) {{
                {map_name}.removeLayer(window.liveWpRouteLine);
                window.liveWpRouteLine = null;
            }}

            var homeCoord = {home_js};
            if (homeCoord !== null) {{
                window.liveWpHomeMarker = L.marker(homeCoord, {{
                    icon: L.icon({{
                        iconUrl: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-green.png',
                        shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/0.7.7/images/marker-shadow.png',
                        iconSize: [25, 41],
                        iconAnchor: [12, 41],
                        popupAnchor: [1, -34],
                        shadowSize: [41, 41]
                    }})
                }})
                    .addTo({map_name})
                    .bindPopup('🏠 Home Point (route)')
                    .bindTooltip('Home');
            }}

            var wpCoords = {wp_coords_js};
            wpCoords.forEach(function(coord, idx) {{
                L.marker(coord, {{
                    icon: L.icon({{
                        iconUrl: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-blue.png',
                        shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/0.7.7/images/marker-shadow.png',
                        iconSize: [25, 41],
                        iconAnchor: [12, 41],
                        popupAnchor: [1, -34],
                        shadowSize: [41, 41]
                    }})
                }})
                    .addTo(window.liveWpMarkers)
                    .bindPopup('📍 WP ' + (idx + 1))
                    .bindTooltip('WP ' + (idx + 1));
            }});

            var lineCoords = {line_coords};
            if (lineCoords.length > 0) {{
                window.liveWpRouteLine = L.polyline(lineCoords, {{
                    color: '#3b82f6',
                    weight: 3,
                    opacity: 0.85,
                    lineCap: 'round',
                    lineJoin: 'round'
                }}).addTo({map_name});
                window.liveWpRouteLine.bringToFront();
            }}
        }})();
        """
        self.page().runJavaScript(js_code)
    
    def enable_click_handler(self, click_callback=None):
        """
        Enable click handler untuk peta yang akan print koordinat ke terminal.
        
        Args:
            click_callback: Function yang akan dipanggil saat peta diklik (lat, lon)
        """
        map_name = self.folium_map.get_name()
        js_code = f"""
        (function() {{
            {map_name}.on('click', function(e) {{
                var lat = e.latlng.lat;
                var lng = e.latlng.lng;
                console.log('Map clicked at: Lat=' + lat + ', Lon=' + lng);
                // Navigate to custom URL scheme untuk komunikasi dengan Python
                window.location.href = 'python://click?lat=' + lat + '&lon=' + lng;
            }});
            console.log('✅ Map click handler enabled');
        }})();
        """
        self.page().runJavaScript(js_code)


class MapPointsWebView(MapWebView):
    """
    Subclass MapWebView khusus untuk tab Map Points dengan click handler.
    """
    def __init__(self, initial_coordinates: tuple[float, float]):
        # Inisialisasi marker counter untuk Map Points
        self.click_marker_count = 0
        # Simpan koordinat marker sebelumnya untuk membuat garis penghubung
        self.previous_marker_coords = None
        # Simpan semua koordinat marker untuk polyline
        self.click_marker_coords = []
        # Reference ke table widget untuk menampilkan data marker
        self.table_widget = None
        # Optional callback untuk diberitahu saat jumlah marker berubah
        # (dipakai MainWindow untuk update label "Points: N").
        self._on_change_callback = None
        # Maksimum jumlah marker yang diperbolehkan
        self.max_markers = 10
        # Home untuk polyline rute (Home → WP1 → WP2 …); di-set dari MainWindow
        self._route_home_coords: tuple[float, float] | None = None

        # Setup click callback
        def on_map_click(lat: float, lon: float):
            """Handler untuk klik peta - akan dipanggil dari JavaScript."""
            # print(f"[MAP POINTS] Clicked at: Lat={lat:.6f}, Lon={lon:.6f}")  # Commented out - tidak perlu printout
            # Cek apakah sudah mencapai maksimum marker
            if self.click_marker_count >= self.max_markers:
                # Tampilkan dialog window sebagai ganti printout
                from PySide6.QtWidgets import QMessageBox
                msg = QMessageBox(self)
                msg.setIcon(QMessageBox.Information)
                msg.setWindowTitle("Maximum Markers Reached")
                msg.setText(f"Maximum {self.max_markers} markers reached.")
                msg.setInformativeText("Cannot add more markers.")
                msg.exec()
                return
            # Tambahkan marker di peta
            self.add_click_marker((lat, lon))
        
        # Simpan callback untuk digunakan nanti
        self.click_callback = on_map_click
        
        # Buat custom page dengan callback (parent akan di-set setelah super().__init__())
        custom_page = ClickableMapPage(None, click_callback=on_map_click)
        
        # Panggil __init__ parent dengan custom_page
        super().__init__(initial_coordinates, custom_page=custom_page)
        
        # Set parent untuk custom page setelah super().__init__() selesai
        # (meskipun sebenarnya tidak wajib untuk QWebEnginePage)
        if custom_page.parent() != self:
            custom_page.setParent(self)
        
        # Setup click handler setelah peta dimuat
        # Gunakan loadFinished signal untuk memastikan HTML benar-benar ter-load
        def setup_click_handler_after_load(success):
            if success:
                from PySide6.QtCore import QTimer
                QTimer.singleShot(500, lambda: self.enable_click_handler(on_map_click))
                # Setup marker group untuk click markers
                QTimer.singleShot(600, self.setup_click_marker_group)
        
        # Connect signal untuk setup click handler setelah HTML ter-load
        if hasattr(self.page(), 'loadFinished'):
            self.page().loadFinished.connect(setup_click_handler_after_load)
        else:
            # Fallback: gunakan timer jika signal tidak tersedia
            from PySide6.QtCore import QTimer
            QTimer.singleShot(2000, lambda: self.enable_click_handler(on_map_click))
            QTimer.singleShot(2100, self.setup_click_marker_group)
    
    def setup_click_marker_group(self):
        """
        Setup marker group dan polyline untuk menyimpan semua marker yang dibuat dari klik.
        """
        map_name = self.folium_map.get_name()
        js_code = f"""
        (function() {{
            // Create marker group untuk click markers jika belum ada
            if (!window.clickMarkers) {{
                window.clickMarkers = L.layerGroup().addTo({map_name});
                console.log('✅ Click markers group created');
            }}
            
            // Create polyline untuk menghubungkan click markers jika belum ada
            if (!window.clickMarkerLine) {{
                window.clickMarkerLine = L.polyline([], {{
                    color: '#3b82f6',
                    weight: 3,
                    opacity: 0.8,
                    lineCap: 'round',
                    lineJoin: 'round'
                }}).addTo({map_name});
                window.clickMarkerLine.bringToFront();
                console.log('✅ Click marker line created');
            }}
        }})();
        """
        self.page().runJavaScript(js_code)
    
    def set_table_widget(self, table_widget):
        """
        Set reference ke table widget untuk update data marker.
        
        Args:
            table_widget: QTableWidget untuk menampilkan data marker
        """
        self.table_widget = table_widget

    def set_change_callback(self, callback):
        """
        Set callback yang akan dipanggil setiap kali click_marker_coords
        berubah (add / delete). Dipakai MainWindow untuk meng-update
        label info jumlah waypoint di group Send Way Points.
        """
        self._on_change_callback = callback

    def set_route_home_coords(self, coords: tuple[float, float] | None) -> None:
        """Set koordinat Home untuk polyline rute biru (Home → waypoint)."""
        self._route_home_coords = coords
        self._refresh_click_marker_line()

    def _route_polyline_coords(self) -> list[list[float]]:
        """Titik polyline: Home (jika ada) lalu semua waypoint klik."""
        pts: list[list[float]] = []
        if self._route_home_coords is not None:
            pts.append([self._route_home_coords[0], self._route_home_coords[1]])
        for c in self.click_marker_coords:
            pts.append([c[0], c[1]])
        return pts

    def _refresh_click_marker_line(self) -> None:
        """Perbarui garis biru waypoint termasuk segmen Home → WP1."""
        map_name = self.folium_map.get_name()
        all_coords = self._route_polyline_coords()
        if not all_coords:
            js_code = f"""
            (function() {{
                if (window.clickMarkerLine) {{
                    {map_name}.removeLayer(window.clickMarkerLine);
                    window.clickMarkerLine = null;
                }}
            }})();
            """
        else:
            js_code = f"""
            (function() {{
                var allCoords = {all_coords};
                if (!window.clickMarkerLine) {{
                    window.clickMarkerLine = L.polyline(allCoords, {{
                        color: '#3b82f6',
                        weight: 3,
                        opacity: 0.8,
                        lineCap: 'round',
                        lineJoin: 'round'
                    }}).addTo({map_name});
                }} else {{
                    window.clickMarkerLine.setLatLngs(allCoords);
                }}
                window.clickMarkerLine.bringToFront();
            }})();
            """
        self.page().runJavaScript(js_code)

    def _notify_change(self):
        """Panggil callback perubahan jika sudah di-set."""
        if self._on_change_callback:
            try:
                self._on_change_callback()
            except Exception:
                pass

    def update_table(self):
        """
        Update table dengan data marker yang sudah ditambahkan.
        """
        if not self.table_widget:
            self._notify_change()
            return
        
        # Clear semua baris terlebih dahulu (termasuk widget tombol)
        # Mulai dari baris 1 (skip baris 0 untuk Home point)
        for row in range(1, self.table_widget.rowCount()):
            # Hapus cell widget (tombol) di kolom Action
            widget = self.table_widget.cellWidget(row, 3)
            if widget:
                self.table_widget.removeCellWidget(row, 3)
            # Clear item text
            for col in range(self.table_widget.columnCount()):
                item = self.table_widget.item(row, col)
                if item:
                    item.setText("")
        
        # Baris 0 (Home) tidak di-clear, akan di-update oleh update_home_point_table jika perlu
        
        # Isi table dengan data marker (mulai dari baris kedua, index 1)
        for idx, coords in enumerate(self.click_marker_coords):
            if idx >= self.max_markers:
                break
            
            # Baris table dimulai dari index 1 (baris kedua)
            table_row = idx + 1
            
            # Kolom No
            item_no = QTableWidgetItem(str(idx + 1))
            item_no.setTextAlignment(Qt.AlignCenter)
            self.table_widget.setItem(table_row, 0, item_no)
            
            # Kolom Lat
            item_lat = QTableWidgetItem(f"{coords[0]:.6f}")
            item_lat.setTextAlignment(Qt.AlignRight | Qt.AlignVCenter)
            self.table_widget.setItem(table_row, 1, item_lat)
            
            # Kolom Long
            item_lon = QTableWidgetItem(f"{coords[1]:.6f}")
            item_lon.setTextAlignment(Qt.AlignRight | Qt.AlignVCenter)
            self.table_widget.setItem(table_row, 2, item_lon)
            
            # Kolom Action - tombol Hapus
            delete_btn = QPushButton("Hapus", self)
            delete_btn.setMaximumWidth(60)
            delete_btn.setMaximumHeight(25)
            delete_btn.clicked.connect(lambda checked, marker_idx=idx: self.delete_marker(marker_idx))
            self.table_widget.setCellWidget(table_row, 3, delete_btn)

        # Notifikasi MainWindow agar label info waypoint ikut ter-update.
        self._notify_change()

    def delete_marker(self, marker_index: int):
        """
        Hapus marker berdasarkan index dan update peta serta tabel.
        
        Args:
            marker_index: Index marker yang akan dihapus (0-based dari click_marker_coords)
        """
        # Validasi index
        if marker_index < 0 or marker_index >= len(self.click_marker_coords):
            return
        
        # Hapus koordinat dari list
        deleted_coords = self.click_marker_coords.pop(marker_index)
        
        # Update marker count
        self.click_marker_count = len(self.click_marker_coords)
        
        # Update previous_marker_coords ke marker terakhir (jika masih ada)
        if self.click_marker_coords:
            self.previous_marker_coords = self.click_marker_coords[-1]
        else:
            self.previous_marker_coords = None
        
        # Hapus semua marker dan polyline dari peta, lalu redraw
        map_name = self.folium_map.get_name()
        js_code = f"""
        (function() {{
            // Clear semua marker dan polyline
            if (window.clickMarkers) {{
                window.clickMarkers.clearLayers();
            }}
            if (window.clickMarkerLine) {{
                {map_name}.removeLayer(window.clickMarkerLine);
                window.clickMarkerLine = null;
            }}
            
            // Redraw semua marker yang tersisa
            var wpCoords = {[list(c) for c in self.click_marker_coords]};
            var lineCoords = {self._route_polyline_coords()};
            
            if (wpCoords.length > 0) {{
                // Recreate marker group dan polyline
                if (!window.clickMarkers) {{
                    window.clickMarkers = L.layerGroup().addTo({map_name});
                }}
                
                // Add markers untuk setiap koordinat yang tersisa
                wpCoords.forEach(function(coord, idx) {{
                    var markerNum = idx + 1;
                    var popupContent = '📍 Point ' + markerNum + '\\\\nLat: ' + coord[0].toFixed(6) + '\\\\nLon: ' + coord[1].toFixed(6);
                    
                    var marker = L.marker(coord, {{
                        icon: L.icon({{
                            iconUrl: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-blue.png',
                            shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/0.7.7/images/marker-shadow.png',
                            iconSize: [25, 41],
                            iconAnchor: [12, 41],
                            popupAnchor: [1, -34],
                            shadowSize: [41, 41]
                        }})
                    }})
                        .addTo(window.clickMarkers)
                        .bindPopup(popupContent)
                        .bindTooltip('Point ' + markerNum);
                }});
                
                // Recreate polyline (Home → waypoint jika Home sudah di-set)
                window.clickMarkerLine = L.polyline(lineCoords, {{
                    color: '#3b82f6',
                    weight: 3,
                    opacity: 0.8,
                    lineCap: 'round',
                    lineJoin: 'round'
                }}).addTo({map_name});
                window.clickMarkerLine.bringToFront();
            }} else if (lineCoords.length > 0) {{
                window.clickMarkerLine = L.polyline(lineCoords, {{
                    color: '#3b82f6',
                    weight: 3,
                    opacity: 0.8,
                    lineCap: 'round',
                    lineJoin: 'round'
                }}).addTo({map_name});
                window.clickMarkerLine.bringToFront();
            }}
            
            console.log('✅ Marker ' + ({marker_index} + 1) + ' deleted. Remaining markers: ' + wpCoords.length);
        }})();
        """
        self.page().runJavaScript(js_code)
        
        # Update tabel dengan data yang sudah di-renumber
        self.update_table()
    
    def add_home_marker(self, coords: tuple[float, float]):
        """
        Tambahkan marker Home di peta.
        
        Args:
            coords: Tuple (latitude, longitude) untuk posisi Home marker
        """
        map_name = self.folium_map.get_name()
        popup_content = f'🏠 Home Point\\nLat: {coords[0]:.6f}\\nLon: {coords[1]:.6f}'
        
        js_code = f"""
        (function() {{
            // Hapus marker Home yang lama jika ada
            if (window.homeMarker) {{
                {map_name}.removeLayer(window.homeMarker);
                window.homeMarker = null;
            }}
            
            // Add Home marker dengan icon hijau
            window.homeMarker = L.marker({list(coords)}, {{
                icon: L.icon({{
                    iconUrl: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-green.png',
                    shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/0.7.7/images/marker-shadow.png',
                    iconSize: [25, 41],
                    iconAnchor: [12, 41],
                    popupAnchor: [1, -34],
                    shadowSize: [41, 41]
                }})
            }})
                .addTo({map_name})
                .bindPopup('{popup_content}')
                .bindTooltip('Home Point');
            
            // Open popup secara otomatis
            window.homeMarker.openPopup();
            
            console.log('✅ Home marker added at {list(coords)}');
        }})();
        """
        self.page().runJavaScript(js_code)
    
    def remove_home_marker(self):
        """
        Hapus marker Home dari peta.
        """
        map_name = self.folium_map.get_name()
        js_code = f"""
        (function() {{
            if (window.homeMarker) {{
                {map_name}.removeLayer(window.homeMarker);
                window.homeMarker = null;
                console.log('✅ Home marker removed');
            }}
        }})();
        """
        self.page().runJavaScript(js_code)

    def update_live_position(self, coords: tuple[float, float], heading_deg: float | None = None):
        """
        Update marker posisi live (data serial terbaru) di peta Map Points.

        Menggunakan window.livePositionMarker (lingkaran oranye) dan
        window.liveHeadingLine (oranye) terpisah dari homeMarker dan click markers.
        """
        import math
        map_name = self.folium_map.get_name()

        heading_line_js = "null"
        if heading_deg is not None:
            try:
                lat1 = math.radians(coords[0])
                lon1 = math.radians(coords[1])
                brng = math.radians(float(heading_deg) % 360.0)
                R = 6371000.0
                dR = 5.0 / R
                lat2 = math.asin(
                    math.sin(lat1) * math.cos(dR)
                    + math.cos(lat1) * math.sin(dR) * math.cos(brng)
                )
                lon2 = lon1 + math.atan2(
                    math.sin(brng) * math.sin(dR) * math.cos(lat1),
                    math.cos(dR) - math.sin(lat1) * math.sin(lat2),
                )
                dest = [math.degrees(lat2), math.degrees(lon2)]
                heading_line_js = str(dest)
            except Exception:
                pass

        heading_str = f"{heading_deg:.1f}" if heading_deg is not None else "N/A"
        popup_content = (
            f"📡 Live Position\\n"
            f"Lat: {coords[0]:.6f}\\n"
            f"Lon: {coords[1]:.6f}\\n"
            f"Hdg: {heading_str}°"
        )

        js_code = f"""
        (function() {{
            if (typeof L === 'undefined' || typeof {map_name} === 'undefined') return;

            if (!window.livePositionMarker) {{
                window.livePositionMarker = L.circleMarker({list(coords)}, {{
                    radius: 8, color: '#ea580c', weight: 2,
                    fillColor: '#f97316', fillOpacity: 0.9
                }}).addTo({map_name})
                  .bindPopup('{popup_content}')
                  .bindTooltip('📡 Live', {{permanent: true, direction: 'top', offset: [0, -10]}});
            }} else {{
                window.livePositionMarker.setLatLng({list(coords)});
                window.livePositionMarker.setPopupContent('{popup_content}');
            }}

            var destPt = {heading_line_js};
            if (destPt !== null) {{
                var pts = [{list(coords)}, destPt];
                if (!window.liveHeadingLine) {{
                    window.liveHeadingLine = L.polyline(pts, {{
                        color: '#ea580c', weight: 3, opacity: 0.9
                    }}).addTo({map_name});
                }} else {{
                    window.liveHeadingLine.setLatLngs(pts);
                }}
            }} else {{
                if (window.liveHeadingLine) {{
                    {map_name}.removeLayer(window.liveHeadingLine);
                    window.liveHeadingLine = null;
                }}
            }}
        }})();
        """
        self.page().runJavaScript(js_code)

    def add_click_marker(self, coords: tuple[float, float]):
        """
        Tambahkan marker di peta untuk setiap titik yang diklik dan buat garis penghubung.
        
        Args:
            coords: Tuple (latitude, longitude) untuk posisi marker
        """
        # Cek apakah sudah mencapai maksimum marker
        if self.click_marker_count >= self.max_markers:
            # Tampilkan dialog window sebagai ganti printout
            from PySide6.QtWidgets import QMessageBox
            msg = QMessageBox(self)
            msg.setIcon(QMessageBox.Information)
            msg.setWindowTitle("Maximum Markers Reached")
            msg.setText(f"Maximum {self.max_markers} markers reached.")
            msg.setInformativeText("Cannot add more markers.")
            msg.exec()
            return
        
        self.click_marker_count += 1
        map_name = self.folium_map.get_name()
        
        # Tambahkan koordinat ke list
        self.click_marker_coords.append(coords)
        
        # Build popup content dengan informasi koordinat
        popup_content = f'📍 Point {self.click_marker_count}\\nLat: {coords[0]:.6f}\\nLon: {coords[1]:.6f}'
        
        # Pastikan polyline sudah dibuat
        has_previous = self.previous_marker_coords is not None
        
        js_code = f"""
        (function() {{
            // Pastikan marker group sudah ada
            if (!window.clickMarkers) {{
                window.clickMarkers = L.layerGroup().addTo({map_name});
            }}
            
            // Pastikan polyline sudah ada
            if (!window.clickMarkerLine) {{
                window.clickMarkerLine = L.polyline([], {{
                    color: '#3b82f6',
                    weight: 3,
                    opacity: 0.8,
                    lineCap: 'round',
                    lineJoin: 'round'
                }}).addTo({map_name});
                window.clickMarkerLine.bringToFront();
            }}
            
            // Add new marker untuk titik yang diklik
            var newMarker = L.marker({list(coords)}, {{
                icon: L.icon({{
                    iconUrl: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-blue.png',
                    shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/0.7.7/images/marker-shadow.png',
                    iconSize: [25, 41],
                    iconAnchor: [12, 41],
                    popupAnchor: [1, -34],
                    shadowSize: [41, 41]
                }})
            }})
                .addTo(window.clickMarkers)
                .bindPopup('{popup_content}')
                .bindTooltip('Point {self.click_marker_count}');
            
            // Open popup secara otomatis
            newMarker.openPopup();
            
            // Update polyline: Home → WP1 → WP2 …
            var lineCoords = {self._route_polyline_coords()};
            window.clickMarkerLine.setLatLngs(lineCoords);
            window.clickMarkerLine.bringToFront();
            
            console.log('✅ Click marker {self.click_marker_count} added at {list(coords)}');
            console.log('✅ Line updated with {len(self._route_polyline_coords())} route points');
        }})();
        """
        self.page().runJavaScript(js_code)
        
        # Update previous marker coords untuk marker berikutnya
        self.previous_marker_coords = coords
        
        # Update table dengan data marker terbaru
        self.update_table()


class MainWindow(QMainWindow):
    """
    Main window aplikasi dashboard monitoring kapal model.
    
    Class ini menangani:
    - Serial communication dengan ESP32-S3 receiver
    - Menampilkan peta interaktif dengan posisi kapal
    - Menampilkan time series plots untuk berbagai parameter
    - Menampilkan live indicators untuk nilai real-time
    - CSV logging untuk menyimpan data
    
    Attributes:
        ser: Objek Serial untuk komunikasi serial
        serial_timer: QTimer untuk polling serial data
        log_file: File object untuk logging CSV
        map_webview: Objek MapWebView untuk menampilkan peta
        rpm_plot_widget: PyQtGraph widget untuk plot rudder 1 (cmd + sensor)
        yaw_plot_widget: PyQtGraph widget untuk plot Yaw / Heading Setpoint
        rudder_plot_widget: PyQtGraph widget untuk plot rudder 2 (cmd + sensor)
    """
    def __init__(self):
        """
        Inisialisasi MainWindow dengan semua komponen GUI.
        
        Membuat:
        - Control panel (port, baud rate, map marker rate)
        - Indicator panel (live values)
        - Map webview
        - Time series plots (Rudder 1/2, Yaw/Setpoint)
        - Serial communication setup
        - Logging setup
        """
        super().__init__()
        self.resize(800, 700)
        self.setWindowTitle("Ship Model Local Dashboard — beta 1.4")
        self.tab_widget = QTabWidget(self)
        self.setCentralWidget(self.tab_widget)
        
        # Starting position (Surabaya)
        self.base_lat = -7.281500
        self.base_lon = 112.798900
        
        # Home point coordinates untuk Map Points tab (inisialisasi sebelum tab dibuat)
        self.home_point_coords = None  # Tuple (lat, lon) atau None
        self.latest_serial_lat = None  # Latest latitude dari serial
        self.latest_serial_lon = None  # Latest longitude dari serial
        self.latest_serial_heading = None  # Latest heading dari serial
        
        # Tab "Map Points" - tab baru sebelum Live Data
        map_points_tab = QWidget(self)
        map_points_tab.setLayout(QHBoxLayout())
        map_points_tab.layout().setContentsMargins(0, 0, 0, 0)
        
        # Map Points Left Panel (4/5 width)
        map_points_left_panel = QWidget(self)
        map_points_left_panel.setLayout(QVBoxLayout())
        map_points_left_panel.layout().setContentsMargins(0, 0, 0, 0)
        
        # Map Points Right Panel (1/5 width)
        map_points_right_panel = QWidget(self)
        map_points_right_panel.setLayout(QVBoxLayout())
        map_points_right_panel.layout().setContentsMargins(12, 12, 12, 12)
        
        # Peta interaktif untuk panel kiri
        self.map_points_webview = MapPointsWebView((self.base_lat, self.base_lon))
        map_points_left_panel.layout().addWidget(self.map_points_webview)
        
        # Button Home Points di panel kanan
        home_points_btn_group = QGroupBox("", self)
        home_points_btn_group.setLayout(QVBoxLayout())
        home_points_btn_group.layout().setContentsMargins(12, 12, 12, 12)
        
        # Label info posisi live terbaru dari serial (grid 2 baris x 3 kolom) — di atas tombol
        live_pos_grid_widget = QWidget(self)
        live_pos_grid = QGridLayout(live_pos_grid_widget)
        live_pos_grid.setContentsMargins(0, 0, 0, 6)
        live_pos_grid.setSpacing(2)

        header_style = "color: #9ca3af; font-size: 10px; font-family: monospace;"
        value_style  = "color: #f97316; font-size: 11px; font-family: monospace; font-weight: bold;"

        for col, name in enumerate(["Latitude (°)", "Longitude (°)", "Heading (°)"]):
            lbl = QLabel(name, self)
            lbl.setStyleSheet(header_style)
            lbl.setAlignment(Qt.AlignCenter)
            live_pos_grid.addWidget(lbl, 0, col)

        self.live_lat_val  = QLabel("—", self)
        self.live_lon_val  = QLabel("—", self)
        self.live_hdg_val  = QLabel("—", self)
        for col, lbl in enumerate([self.live_lat_val, self.live_lon_val, self.live_hdg_val]):
            lbl.setStyleSheet(value_style)
            lbl.setAlignment(Qt.AlignCenter)
            live_pos_grid.addWidget(lbl, 1, col)

        home_points_btn_group.layout().addWidget(live_pos_grid_widget)

        self.home_points_btn = QPushButton("Set Home Point", self)
        self.home_points_btn.setEnabled(False)  # Disabled by default, akan di-enable saat connected
        self.home_points_btn.clicked.connect(self.set_home_point_from_serial)
        home_points_btn_group.layout().addWidget(self.home_points_btn)

        map_points_right_panel.layout().addWidget(home_points_btn_group)
        
        # Table untuk menampilkan marker points di panel kanan
        map_points_table_group = QGroupBox("Marker Points", self)
        map_points_table_group.setLayout(QVBoxLayout())
        map_points_table_group.layout().setContentsMargins(12, 12, 12, 12)
        
        # Buat table dengan 4 kolom: No, Lat, Long, Action
        self.map_points_table = QTableWidget(self)
        self.map_points_table.setColumnCount(4)
        self.map_points_table.setRowCount(11)  # Total 11 baris (baris pertama kosong, baris 2-12 untuk data)
        self.map_points_table.setHorizontalHeaderLabels(["No", "Lat", "Long", "Action"])
        
        # Set table properties
        self.map_points_table.setEditTriggers(QTableWidget.NoEditTriggers)  # Read-only
        self.map_points_table.setSelectionBehavior(QTableWidget.SelectRows)
        self.map_points_table.horizontalHeader().setStretchLastSection(False)
        # Set kolom No lebih kecil (setengah dari lebar normal)
        self.map_points_table.horizontalHeader().setSectionResizeMode(0, QHeaderView.Fixed)
        self.map_points_table.setColumnWidth(0, 60)  # Lebar kolom No: 50px
        # Kolom Lat dan Long menggunakan stretch untuk mengisi sisa ruang
        self.map_points_table.horizontalHeader().setSectionResizeMode(1, QHeaderView.Stretch)
        self.map_points_table.horizontalHeader().setSectionResizeMode(2, QHeaderView.Stretch)
        # Kolom Action dengan lebar fixed
        self.map_points_table.horizontalHeader().setSectionResizeMode(3, QHeaderView.Fixed)
        self.map_points_table.setColumnWidth(3, 70)  # Lebar kolom Action: 80px
        self.map_points_table.verticalHeader().setVisible(False)
        
        # Set tinggi baris agar pas untuk 11 baris tanpa scroll
        # Tinggi header (sekitar 35-40px) + (11 baris * tinggi per baris)
        row_height = 30  # Tinggi per baris dalam pixel
        header_height = 40  # Tinggi header (nilai konsisten)
        total_height = header_height + (11 * row_height)  # Total: 370px
        
        # Set tinggi baris untuk semua baris
        for row in range(11):
            self.map_points_table.setRowHeight(row, row_height)
        
        # Set tinggi table agar pas untuk 10 baris tanpa scroll
        self.map_points_table.setFixedHeight(total_height)
        
        # Nonaktifkan scrollbar vertikal
        self.map_points_table.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        
        map_points_table_group.layout().addWidget(self.map_points_table)
        
        # Set "Home" di baris pertama (baris 0) saat inisialisasi
        # (akan di-update oleh update_home_point_table jika Home point sudah di-set)
        self.update_home_point_table()
        map_points_right_panel.layout().addWidget(map_points_table_group)

        # Group "Send Way Points" untuk mengirim Home + waypoint ke remote-side.
        # Catatan migrasi (Step 1):
        # - Field lama a/b/c/d (struct send_to_remote_side) tetap dipertahankan
        #   di kode tapi DISEMBUNYIKAN supaya mudah revert. Akan dihapus
        #   permanen di Step 4 setelah firmware user-side & remote-side migrasi
        #   ke struct waypoints_payload.
        # - Tombol & status label internal masih bernama set_param_btn /
        #   set_param_status_label untuk menjaga referensi lain (connect_serial,
        #   disconnect_serial) tidak putus selama transisi.
        set_param_group = QGroupBox("Send Way Points", self)
        set_param_group.setLayout(QVBoxLayout())
        set_param_group.layout().setContentsMargins(12, 12, 12, 12)

        set_param_form = QFormLayout()
        set_param_form.setContentsMargins(0, 0, 0, 0)
        set_param_form.setSpacing(8)

        self.param_a_input = QLineEdit("THIS IS A CHAR", self)
        self.param_a_input.setMaxLength(31)  # char a[32] -> max 31 char + null terminator
        self.param_a_input.setPlaceholderText("char a[32]")

        self.param_b_input = QLineEdit("1", self)
        self.param_b_input.setPlaceholderText("int b")

        self.param_c_input = QLineEdit("3.4", self)
        self.param_c_input.setPlaceholderText("float c")

        self.param_d_input = QLineEdit("true", self)
        self.param_d_input.setPlaceholderText("bool d (true/false)")

        # Bungkus form a/b/c/d dalam container terpisah supaya bisa di-hide
        # sebagai satu kesatuan tanpa membongkar layout.
        self._legacy_param_form_widget = QWidget(self)
        self._legacy_param_form_widget.setLayout(set_param_form)
        set_param_form.addRow(QLabel("a (char[32])"), self.param_a_input)
        set_param_form.addRow(QLabel("b (int)"), self.param_b_input)
        set_param_form.addRow(QLabel("c (float)"), self.param_c_input)
        set_param_form.addRow(QLabel("d (bool)"), self.param_d_input)
        # Sembunyikan field a/b/c/d - tidak relevan lagi untuk Send Way Points.
        # Tetap di kode (tidak dihapus) supaya mudah dikembalikan jika perlu.
        self._legacy_param_form_widget.setVisible(False)

        set_param_group.layout().addWidget(self._legacy_param_form_widget)

        # Info singkat untuk menggantikan form a/b/c/d. Akan di-update tiap
        # ada perubahan jumlah marker (handler dipanggil di update_table()).
        self.waypoints_info_label = QLabel("Points: 0  (need ≥ 3)", self)
        self.waypoints_info_label.setObjectName("waypointsInfoLabel")
        self.waypoints_info_label.setStyleSheet("color: #e5e7eb; padding: 2px 0;")
        set_param_group.layout().addWidget(self.waypoints_info_label)

        self.set_param_btn = QPushButton("Send Way Points", self)
        # Gate awal: tombol baru aktif setelah Connect berhasil (lihat connect_serial / disconnect_serial)
        self.set_param_btn.setEnabled(False)
        self.set_param_btn.clicked.connect(self.on_set_param_clicked)
        set_param_group.layout().addWidget(self.set_param_btn)

        # Status label untuk menampilkan respons terakhir dari user-side ESP32
        # (mis. $WACK,OK / $WACK,ERR,<reason>). Akan di-update oleh handler &
        # poll_serial saat respons tiba.
        self.set_param_status_label = QLabel("Status: idle", self)
        self.set_param_status_label.setObjectName("setParamStatusLabel")
        self.set_param_status_label.setWordWrap(True)
        self.set_param_status_label.setStyleSheet(
            "color: #9ca3af; font-style: italic; padding: 4px 2px 0 2px;"
        )
        set_param_group.layout().addWidget(self.set_param_status_label)

        map_points_right_panel.layout().addWidget(set_param_group)
        map_points_right_panel.layout().addStretch(1)

        # Simpan reference ke map_points_webview untuk update table
        self.map_points_webview.set_table_widget(self.map_points_table)

        # Hubungkan perubahan jumlah marker -> update label info waypoints.
        # Dipanggil otomatis dari MapPointsWebview.update_table()
        # (yang dipanggil oleh add_click_marker dan delete_marker).
        self.map_points_webview.set_change_callback(self.update_waypoints_info_label)
        # Inisialisasi label dengan state awal (0 points).
        self.update_waypoints_info_label()
        
        # Add panels to tab with ratio 3.75:1
        map_points_tab.layout().addWidget(map_points_left_panel, 3.75)
        map_points_tab.layout().addWidget(map_points_right_panel, 1)
        
        # Tab "Live Data" menampung seluruh layout eksisting
        live_tab = QWidget(self)
        live_tab.setLayout(QHBoxLayout())
        live_tab.layout().setContentsMargins(0, 0, 0, 0)
        
        # Koreksi rudder (display + log CSV saja, setelah terima serial)
        self.rudder1_correction_enabled = False
        self.rudder1_correction_value = 0.0
        self.rudder2_correction_enabled = False
        self.rudder2_correction_value = 0.0
        self.rudder_cmd_correction_enabled = False
        self.rudder_cmd_correction_value = 0.0
        
        # Serial state
        self.ser = None
        self.serial_timer = QTimer(self)
        self.serial_timer.timeout.connect(self.poll_serial)
        
        # Plot interval control (decimation)
        self.plot_counter = 0
        self.plot_interval = 10  # Default: plot setiap 10 data
        
        # Logging state
        self.log_btn = None
        self.log_file_path = None
        self.log_file = None
        self.log_buffer = []
        self.log_timer = QTimer(self)
        self.log_timer.setInterval(400)  # flush every 400 ms
        self.log_timer.timeout.connect(self.flush_log_buffer)

        # Set Param state: timer timeout untuk menunggu ACK $PACK,... dari user-side
        self._set_param_pending = False
        self._set_param_timeout_timer = QTimer(self)
        self._set_param_timeout_timer.setSingleShot(True)
        self._set_param_timeout_timer.setInterval(1500)  # 1.5 detik tanpa ACK -> TIMEOUT
        self._set_param_timeout_timer.timeout.connect(self._on_set_param_timeout)
        
        # Helper connection state method
        def _is_connected() -> bool:
            return bool(self.ser and getattr(self.ser, 'is_open', False) and self.serial_timer.isActive())
        self.is_connected = _is_connected
        
        # Right side wrapper panel (1/4 width)
        right_panel = QWidget(self)
        right_panel.setLayout(QVBoxLayout())
        right_panel.layout().setContentsMargins(0, 0, 0, 0)

        # Controls panel (top of right panel)
        controls_panel = QGroupBox("", self)
        controls_panel.setLayout(QVBoxLayout())
        controls_panel.layout().setContentsMargins(12, 12, 12, 12)

        # Grid 2x3: (row,col) =>
        # (0,0) Port label, (0,1) Port dropdown, (0,2) Refresh button
        # (1,0) Baud label, (1,1) Baud dropdown, (1,2) empty
        grid_widget = QWidget(self)
        grid = QGridLayout()
        grid.setContentsMargins(0, 0, 0, 0)
        grid.setHorizontalSpacing(8)
        grid.setVerticalSpacing(6)

        port_label = QLabel("Port:")
        self.port_combo = QComboBox(self)
        self.refresh_ports()
        self.refresh_btn = QPushButton("Refresh Ports", self)
        self.refresh_btn.clicked.connect(self.refresh_ports)

        baud_label = QLabel("Baud:")
        self.baud_combo = QComboBox(self)
        self.baud_combo.addItems(["115200", "57600", "38400", "19200", "9600", "230400", "460800", "921600"])
        self.baud_combo.setCurrentText("115200")

        map_marker_label = QLabel("Map Marker:")
        self.plot_int_combo = QComboBox(self)
        # Combo box items: marker per detik (@ 10Hz sampling)
        # Format: "value" -> interval calculation: 10/value
        self.plot_int_combo.addItems(["10", "5", "2", "1", "0.5", "0.2", "0.1"])
        self.plot_int_combo.setCurrentText("1")  # Default: 1 marker/detik
        self.plot_int_combo.setToolTip("Jumlah marker yang di-plot ke map per detik (@ 10Hz sampling)")
        map_marker_unit_label = QLabel("per detik")

        grid.addWidget(port_label, 0, 0)
        grid.addWidget(self.port_combo, 0, 1)
        grid.addWidget(self.refresh_btn, 0, 2)
        grid.addWidget(baud_label, 1, 0)
        grid.addWidget(self.baud_combo, 1, 1)
        # (1,2) intentionally left empty
        grid.addWidget(map_marker_label, 2, 0)
        grid.addWidget(self.plot_int_combo, 2, 1)
        grid.addWidget(map_marker_unit_label, 2, 2)

        grid_widget.setLayout(grid)
        controls_panel.layout().addWidget(grid_widget)

        row3 = QWidget(self)
        row3.setLayout(QHBoxLayout())
        row3.layout().setContentsMargins(0, 0, 0, 0)
        self.connect_btn = QPushButton("Connect", self)
        self.connect_btn.setCheckable(True)
        self.connect_btn.clicked.connect(self.toggle_connection)
        row3.layout().addWidget(self.connect_btn)
        # Start/Stop Log toggle
        self.log_btn = QPushButton("Start Log", self)
        self.log_btn.setCheckable(True)
        self.log_btn.clicked.connect(self.toggle_logging)
        row3.layout().addWidget(self.log_btn)
        controls_panel.layout().addWidget(row3)

        # Initial UI states
        self.log_btn.setEnabled(False)

        controls_panel.layout().addStretch(1)

        # Reserved panel for future UI elements (bottom of right panel)
        indicator_panel = QGroupBox("", self)
        indicator_panel.setLayout(QVBoxLayout())
        indicator_panel.layout().setContentsMargins(12, 12, 12, 12)

        # Live indicators — baris bebas (VBox + HBox per baris)
        indicator = QWidget(self)
        indicator.setLayout(QVBoxLayout())
        indicator.layout().setContentsMargins(0, 0, 0, 0)
        indicator.layout().setSpacing(8)

        value_style = "color: #e5e7eb; font-weight: bold; font-size: 12pt; text-align: center;"

        self.mode_label = QLabel("Manual", self)
        self.mode_label.setStyleSheet("color: #10b981; font-weight: bold; font-size: 13pt; text-align: center;")

        self.mini_pc_link_label = QLabel("Mini PC: —", self)
        self.mini_pc_link_label.setStyleSheet("color: #9ca3af; font-weight: bold; font-size: 11pt; text-align: center;")

        self.auto_warn_label = QLabel("", self)
        self.auto_warn_label.setStyleSheet("color: #f59e0b; font-weight: bold; font-size: 10pt; text-align: center;")
        self.auto_warn_label.setWordWrap(True)

        self.speed_label = QLabel("0.00 m/s", self)
        self.speed_label.setStyleSheet(value_style)

        self.track_wp_label = QLabel("—", self)
        self.track_wp_label.setStyleSheet(value_style)
        self.dist_wp_label = QLabel("— m", self)
        self.dist_wp_label.setStyleSheet(value_style)

        self.yaw_label = QLabel("0.0°", self)
        self.yaw_label.setStyleSheet(value_style)
        self.hdg_sp_label = QLabel("0.0°", self)
        self.hdg_sp_label.setStyleSheet(value_style)
        self.hdg_err_label = QLabel("0.0°", self)
        self.hdg_err_label.setStyleSheet(value_style)

        self.rudder_cmd_label = QLabel("0.0°", self)
        self.rudder_cmd_label.setStyleSheet(value_style)
        self.rud1_label = QLabel("0.0°", self)
        self.rud1_label.setStyleSheet(value_style)
        self.rud2_label = QLabel("0.0°", self)
        self.rud2_label.setStyleSheet(value_style)

        self.rpm1_label = QLabel("0 RPM", self)
        self.rpm1_label.setStyleSheet(value_style)
        self.rpm2_label = QLabel("0 RPM", self)
        self.rpm2_label.setStyleSheet(value_style)

        self.bat1_label = QLabel("12.00 V", self)
        self.bat1_label.setStyleSheet("color: #10b981; font-weight: bold; font-size: 12pt; text-align: center;")
        self.bat2_label = QLabel("12.00 V", self)
        self.bat2_label.setStyleSheet("color: #10b981; font-weight: bold; font-size: 12pt; text-align: center;")

        def _add_indicator_row(cells: list[QWidget]) -> None:
            row = QWidget(self)
            row_layout = QHBoxLayout(row)
            row_layout.setContentsMargins(0, 0, 0, 0)
            row_layout.setSpacing(6)
            for cell in cells:
                row_layout.addWidget(cell, 1)
            indicator.layout().addWidget(row)

        # Baris 1: Mode
        _add_indicator_row([_make_live_stat_cell(self, "Mode", self.mode_label)])

        # Baris 1b: Mini PC link + auto warning
        _add_indicator_row([_make_live_stat_cell(self, "Mini PC", self.mini_pc_link_label)])
        indicator.layout().addWidget(self.auto_warn_label)

        # Baris 2: GPS Speed
        _add_indicator_row([_make_live_stat_cell(self, "GPS Speed (m/s)", self.speed_label)])

        # Baris 3: Track WP + Distance WP
        _add_indicator_row([
            _make_live_stat_cell(self, "Track WP", self.track_wp_label),
            _make_live_stat_cell(self, "Distance WP (m)", self.dist_wp_label),
        ])

        # Baris 4: Yaw + Heading Setpoint + Heading Error
        _add_indicator_row([
            _make_live_stat_cell(self, "Yaw (°)", self.yaw_label),
            _make_live_stat_cell(self, "Heading Setpoint (°)", self.hdg_sp_label),
            _make_live_stat_cell(self, "Heading Error (°)", self.hdg_err_label),
        ])

        # Baris 5: Rudder Cmd + Rudder 1 + Rudder 2
        _add_indicator_row([
            _make_live_stat_cell(self, "Rudder Cmd (°)", self.rudder_cmd_label),
            _make_live_stat_cell(self, "Rudder 1 (°)", self.rud1_label),
            _make_live_stat_cell(self, "Rudder 2 (°)", self.rud2_label),
        ])

        # Baris 6: RPM
        _add_indicator_row([
            _make_live_stat_cell(self, "RPM Propeller 1", self.rpm1_label),
            _make_live_stat_cell(self, "RPM Propeller 2", self.rpm2_label),
        ])

        # Baris 7: Battery
        _add_indicator_row([
            _make_live_stat_cell(self, "Battery Control", self.bat1_label),
            _make_live_stat_cell(self, "Battery Motor", self.bat2_label),
        ])

        indicator_panel.layout().addWidget(indicator)
        indicator_panel.layout().addStretch(1)

        self.live_setup_btn = QPushButton("Setup", self)
        self.live_setup_btn.clicked.connect(self._open_live_setup_dialog)
        indicator_panel.layout().addWidget(self.live_setup_btn)

        # Map | plots — QSplitter agar lebar bisa di-drag
        left_panel = QSplitter(Qt.Orientation.Horizontal, self)
        left_panel.setChildrenCollapsible(False)
        left_panel.setHandleWidth(6)
        left_panel.setStyleSheet(
            "QSplitter::handle { background: #4b5563; margin: 0 1px; }"
            "QSplitter::handle:hover { background: #6b7280; }"
        )

        left_panel_map = QWidget(self)
        left_panel_map.setLayout(QVBoxLayout())
        left_panel_map.layout().setContentsMargins(0, 0, 0, 0)
        self.map_webview = MapWebView((self.base_lat, self.base_lon))
        left_panel_map.layout().addWidget(self.map_webview)

        left_panel_plots = QWidget(self)
        left_panel_plots.setLayout(QVBoxLayout())
        left_panel_plots.layout().setContentsMargins(0, 0, 0, 0)
        left_panel_plots.layout().setSpacing(0)

        self.start_time = time()
        self.max_points = 50

        # Plot 1 (atas): Yaw + Heading Setpoint
        self.yaw_plot_widget = pg.PlotWidget()
        self.yaw_plot_widget.setLabel('left', 'Heading (°)', color='#e5e7eb', **{'font-size': '11pt'})
        self.yaw_plot_widget.setLabel('bottom', 'Time (s)', color='#e5e7eb', **{'font-size': '11pt'})
        self.yaw_plot_widget.setTitle('Yaw & Heading Setpoint', color='#e5e7eb', size='11pt')
        self.yaw_plot_widget.setBackground('#1f2937')
        self.yaw_plot_widget.addLegend(offset=(10, 10))
        self.yaw_plot_widget.showGrid(x=False, y=False)
        self.yaw_plot_widget.getAxis('left').setPen(pg.mkPen(color='#10b981', width=2))
        self.yaw_plot_widget.getAxis('bottom').setPen(pg.mkPen(color='#e5e7eb', width=1))
        self.yaw_plot_widget.getAxis('left').setTextPen(pg.mkPen(color='#e5e7eb'))
        self.yaw_plot_widget.getAxis('bottom').setTextPen(pg.mkPen(color='#e5e7eb'))
        self.yaw_time_data = []
        self.yaw_data = []
        self.heading_sp_data = []
        self.yaw_curve = self.yaw_plot_widget.plot(name='Yaw', pen=pg.mkPen(color='#10b981', width=2))
        self.heading_sp_curve = self.yaw_plot_widget.plot(
            name='Heading Setpoint', pen=pg.mkPen(color='#06b6d4', width=2))

        # Plot 2 (tengah): Rudder 1 Cmd + Sensor
        self.rudder1_plot_widget = pg.PlotWidget()
        self.rudder1_plot_widget.setLabel('left', 'Angle (°)', color='#e5e7eb', **{'font-size': '11pt'})
        self.rudder1_plot_widget.setLabel('bottom', 'Time (s)', color='#e5e7eb', **{'font-size': '11pt'})
        self.rudder1_plot_widget.setTitle('Rudder 1: Cmd + Sensor', color='#e5e7eb', size='11pt')
        self.rudder1_plot_widget.setBackground('#1f2937')
        self.rudder1_plot_widget.addLegend(offset=(10, 10))
        self.rudder1_plot_widget.showGrid(x=False, y=False)
        self.rudder1_plot_widget.getAxis('left').setPen(pg.mkPen(color='#e5e7eb', width=1))
        self.rudder1_plot_widget.getAxis('bottom').setPen(pg.mkPen(color='#e5e7eb', width=1))
        self.rudder1_plot_widget.getAxis('left').setTextPen(pg.mkPen(color='#e5e7eb'))
        self.rudder1_plot_widget.getAxis('bottom').setTextPen(pg.mkPen(color='#e5e7eb'))
        self.rudder1_time_data = []
        self.rudder1_cmd_data = []
        self.rudder1_sensor_data = []
        self.rudder1_cmd_curve = self.rudder1_plot_widget.plot(
            name='Rudder Cmd', pen=pg.mkPen(color='#3b82f6', width=2))
        self.rudder1_sensor_curve = self.rudder1_plot_widget.plot(
            name='Rudder 1 Sensor', pen=pg.mkPen(color='#ec4899', width=2))

        # Plot 3 (bawah): Rudder 2 Cmd + Sensor
        self.rudder2_plot_widget = pg.PlotWidget()
        self.rudder2_plot_widget.setLabel('left', 'Angle (°)', color='#e5e7eb', **{'font-size': '11pt'})
        self.rudder2_plot_widget.setLabel('bottom', 'Time (s)', color='#e5e7eb', **{'font-size': '11pt'})
        self.rudder2_plot_widget.setTitle('Rudder 2: Cmd + Sensor', color='#e5e7eb', size='11pt')
        self.rudder2_plot_widget.setBackground('#1f2937')
        self.rudder2_plot_widget.addLegend(offset=(10, 10))
        self.rudder2_plot_widget.showGrid(x=False, y=False)
        self.rudder2_plot_widget.getAxis('left').setPen(pg.mkPen(color='#e5e7eb', width=1))
        self.rudder2_plot_widget.getAxis('bottom').setPen(pg.mkPen(color='#e5e7eb', width=1))
        self.rudder2_plot_widget.getAxis('left').setTextPen(pg.mkPen(color='#e5e7eb'))
        self.rudder2_plot_widget.getAxis('bottom').setTextPen(pg.mkPen(color='#e5e7eb'))
        self.rudder2_time_data = []
        self.rudder2_cmd_data = []
        self.rudder2_sensor_data = []
        self.rudder2_cmd_curve = self.rudder2_plot_widget.plot(
            name='Rudder Cmd', pen=pg.mkPen(color='#3b82f6', width=2))
        self.rudder2_sensor_curve = self.rudder2_plot_widget.plot(
            name='Rudder 2 Sensor', pen=pg.mkPen(color='#14b8a6', width=2))

        left_panel_plots.layout().addWidget(self.yaw_plot_widget, 1)
        left_panel_plots.layout().addWidget(self.rudder1_plot_widget, 1)
        left_panel_plots.layout().addWidget(self.rudder2_plot_widget, 1)

        left_panel.addWidget(left_panel_map)
        left_panel.addWidget(left_panel_plots)
        left_panel.setStretchFactor(0, 1)
        left_panel.setStretchFactor(1, 1)
        left_panel.setSizes([500, 500])
        self.live_map_plots_splitter = left_panel

        # Assemble right panel with stretch ratio 1:2 (controls : reserved)
        right_panel.layout().addWidget(controls_panel, 1)
        right_panel.layout().addWidget(indicator_panel, 4)

        # Add to Live tab layout dengan rasio 3:1
        live_tab.layout().addWidget(left_panel, 3)
        live_tab.layout().addWidget(right_panel, 1)
        
        # Tab Analyze — map | 3 plot (sama Live) + panel indikator di kanan
        analyze_tab = QWidget(self)
        analyze_tab.setLayout(QHBoxLayout())
        analyze_tab.layout().setContentsMargins(0, 0, 0, 0)

        analyze_left_panel = QWidget(self)
        analyze_left_panel.setLayout(QVBoxLayout())
        analyze_left_panel.layout().setContentsMargins(0, 0, 0, 0)
        analyze_left_panel.layout().setSpacing(12)

        analyze_content_splitter = QSplitter(Qt.Orientation.Horizontal, self)
        analyze_content_splitter.setChildrenCollapsible(False)
        analyze_content_splitter.setHandleWidth(6)
        analyze_content_splitter.setStyleSheet(
            "QSplitter::handle { background: #4b5563; margin: 0 1px; }"
            "QSplitter::handle:hover { background: #6b7280; }"
        )

        analyze_map_panel = QWidget(self)
        analyze_map_panel.setLayout(QVBoxLayout())
        analyze_map_panel.layout().setContentsMargins(12, 12, 12, 12)
        analyze_map_group = QGroupBox("Map Viewer (Analyze)", self)
        analyze_map_group.setLayout(QVBoxLayout())
        self.analyze_map_webview = MapWebView((self.base_lat, self.base_lon))
        analyze_map_group.layout().addWidget(self.analyze_map_webview)
        analyze_map_panel.layout().addWidget(analyze_map_group)

        analyze_plots_panel = QWidget(self)
        analyze_plots_panel.setLayout(QVBoxLayout())
        analyze_plots_panel.layout().setContentsMargins(12, 12, 12, 12)
        analyze_plots_panel.layout().setSpacing(0)

        self.analyze_yaw_plot_widget = pg.PlotWidget()
        self.analyze_yaw_plot_widget.setLabel('left', 'Heading (°)', color='#e5e7eb', **{'font-size': '11pt'})
        self.analyze_yaw_plot_widget.setLabel('bottom', 'Time (s)', color='#e5e7eb', **{'font-size': '11pt'})
        self.analyze_yaw_plot_widget.setTitle('Yaw & Heading Setpoint (Recorded)', color='#e5e7eb', size='11pt')
        self.analyze_yaw_plot_widget.setBackground('#1f2937')
        self.analyze_yaw_plot_widget.addLegend(offset=(10, 10))
        self.analyze_yaw_plot_widget.showGrid(x=False, y=False)
        self.analyze_yaw_plot_widget.getAxis('left').setPen(pg.mkPen(color='#10b981', width=2))
        self.analyze_yaw_plot_widget.getAxis('bottom').setPen(pg.mkPen(color='#e5e7eb', width=1))
        self.analyze_yaw_plot_widget.getAxis('left').setTextPen(pg.mkPen(color='#e5e7eb'))
        self.analyze_yaw_plot_widget.getAxis('bottom').setTextPen(pg.mkPen(color='#e5e7eb'))
        self.analyze_yaw_curve = self.analyze_yaw_plot_widget.plot(
            name='Yaw', pen=pg.mkPen(color='#10b981', width=2))
        self.analyze_heading_sp_curve = self.analyze_yaw_plot_widget.plot(
            name='Heading Setpoint', pen=pg.mkPen(color='#06b6d4', width=2))
        self.analyze_yaw_target = pg.TargetItem(
            size=16, pen=pg.mkPen(color='#10b981', width=1.5), movable=False, symbol='x')
        self.analyze_yaw_target.setZValue(2)
        self.analyze_yaw_plot_widget.addItem(self.analyze_yaw_target)
        self.analyze_yaw_target.hide()
        self.analyze_heading_sp_target = pg.TargetItem(
            size=14, pen=pg.mkPen(color='#06b6d4', width=1.5), movable=False, symbol='o')
        self.analyze_heading_sp_target.setZValue(2)
        self.analyze_yaw_plot_widget.addItem(self.analyze_heading_sp_target)
        self.analyze_heading_sp_target.hide()
        self.analyze_yaw_label = pg.TextItem(text='', color='#f9fafb', anchor=(0, 1))
        self.analyze_yaw_label.setZValue(2)
        self.analyze_yaw_plot_widget.addItem(self.analyze_yaw_label)
        self.analyze_yaw_label.hide()

        self.analyze_rudder1_plot_widget = pg.PlotWidget()
        self.analyze_rudder1_plot_widget.setLabel('left', 'Angle (°)', color='#e5e7eb', **{'font-size': '11pt'})
        self.analyze_rudder1_plot_widget.setLabel('bottom', 'Time (s)', color='#e5e7eb', **{'font-size': '11pt'})
        self.analyze_rudder1_plot_widget.setTitle('Rudder 1: Cmd + Sensor (Recorded)', color='#e5e7eb', size='11pt')
        self.analyze_rudder1_plot_widget.setBackground('#1f2937')
        self.analyze_rudder1_plot_widget.addLegend(offset=(10, 10))
        self.analyze_rudder1_plot_widget.showGrid(x=False, y=False)
        self.analyze_rudder1_plot_widget.getAxis('left').setPen(pg.mkPen(color='#e5e7eb', width=1))
        self.analyze_rudder1_plot_widget.getAxis('bottom').setPen(pg.mkPen(color='#e5e7eb', width=1))
        self.analyze_rudder1_plot_widget.getAxis('left').setTextPen(pg.mkPen(color='#e5e7eb'))
        self.analyze_rudder1_plot_widget.getAxis('bottom').setTextPen(pg.mkPen(color='#e5e7eb'))
        self.analyze_rudder1_cmd_curve = self.analyze_rudder1_plot_widget.plot(
            name='Rudder Cmd', pen=pg.mkPen(color='#3b82f6', width=2))
        self.analyze_rudder1_sensor_curve = self.analyze_rudder1_plot_widget.plot(
            name='Rudder 1 Sensor', pen=pg.mkPen(color='#ec4899', width=2))
        self.analyze_rudder1_cmd_target = pg.TargetItem(
            size=16, pen=pg.mkPen(color='#3b82f6', width=1.5), movable=False, symbol='x')
        self.analyze_rudder1_cmd_target.setZValue(2)
        self.analyze_rudder1_plot_widget.addItem(self.analyze_rudder1_cmd_target)
        self.analyze_rudder1_cmd_target.hide()
        self.analyze_rudder1_sensor_target = pg.TargetItem(
            size=14, pen=pg.mkPen(color='#ec4899', width=1.5), movable=False, symbol='o')
        self.analyze_rudder1_sensor_target.setZValue(2)
        self.analyze_rudder1_plot_widget.addItem(self.analyze_rudder1_sensor_target)
        self.analyze_rudder1_sensor_target.hide()
        self.analyze_rudder1_label = pg.TextItem(text='', color='#f9fafb', anchor=(0, 1))
        self.analyze_rudder1_label.setZValue(2)
        self.analyze_rudder1_plot_widget.addItem(self.analyze_rudder1_label)
        self.analyze_rudder1_label.hide()

        self.analyze_rudder2_plot_widget = pg.PlotWidget()
        self.analyze_rudder2_plot_widget.setLabel('left', 'Angle (°)', color='#e5e7eb', **{'font-size': '11pt'})
        self.analyze_rudder2_plot_widget.setLabel('bottom', 'Time (s)', color='#e5e7eb', **{'font-size': '11pt'})
        self.analyze_rudder2_plot_widget.setTitle('Rudder 2: Cmd + Sensor (Recorded)', color='#e5e7eb', size='11pt')
        self.analyze_rudder2_plot_widget.setBackground('#1f2937')
        self.analyze_rudder2_plot_widget.addLegend(offset=(10, 10))
        self.analyze_rudder2_plot_widget.showGrid(x=False, y=False)
        self.analyze_rudder2_plot_widget.getAxis('left').setPen(pg.mkPen(color='#e5e7eb', width=1))
        self.analyze_rudder2_plot_widget.getAxis('bottom').setPen(pg.mkPen(color='#e5e7eb', width=1))
        self.analyze_rudder2_plot_widget.getAxis('left').setTextPen(pg.mkPen(color='#e5e7eb'))
        self.analyze_rudder2_plot_widget.getAxis('bottom').setTextPen(pg.mkPen(color='#e5e7eb'))
        self.analyze_rudder2_cmd_curve = self.analyze_rudder2_plot_widget.plot(
            name='Rudder Cmd', pen=pg.mkPen(color='#3b82f6', width=2))
        self.analyze_rudder2_sensor_curve = self.analyze_rudder2_plot_widget.plot(
            name='Rudder 2 Sensor', pen=pg.mkPen(color='#14b8a6', width=2))
        self.analyze_rudder2_cmd_target = pg.TargetItem(
            size=16, pen=pg.mkPen(color='#3b82f6', width=1.5), movable=False, symbol='x')
        self.analyze_rudder2_cmd_target.setZValue(2)
        self.analyze_rudder2_plot_widget.addItem(self.analyze_rudder2_cmd_target)
        self.analyze_rudder2_cmd_target.hide()
        self.analyze_rudder2_sensor_target = pg.TargetItem(
            size=14, pen=pg.mkPen(color='#14b8a6', width=1.5), movable=False, symbol='o')
        self.analyze_rudder2_sensor_target.setZValue(2)
        self.analyze_rudder2_plot_widget.addItem(self.analyze_rudder2_sensor_target)
        self.analyze_rudder2_sensor_target.hide()
        self.analyze_rudder2_label = pg.TextItem(text='', color='#f9fafb', anchor=(0, 1))
        self.analyze_rudder2_label.setZValue(2)
        self.analyze_rudder2_plot_widget.addItem(self.analyze_rudder2_label)
        self.analyze_rudder2_label.hide()

        analyze_plots_panel.layout().addWidget(self.analyze_yaw_plot_widget, 1)
        analyze_plots_panel.layout().addWidget(self.analyze_rudder1_plot_widget, 1)
        analyze_plots_panel.layout().addWidget(self.analyze_rudder2_plot_widget, 1)

        analyze_content_splitter.addWidget(analyze_map_panel)
        analyze_content_splitter.addWidget(analyze_plots_panel)
        analyze_content_splitter.setStretchFactor(0, 1)
        analyze_content_splitter.setStretchFactor(1, 1)
        analyze_content_splitter.setSizes([500, 500])

        # Data containers Analyze (nilai tampilan dari CSV)
        self.analyze_time_data: list[float] = []
        self.analyze_lat_data: list[float] = []
        self.analyze_lon_data: list[float] = []
        self.analyze_speed_data: list[float] = []
        self.analyze_rud1_sensor_data: list[float] = []
        self.analyze_rud2_sensor_data: list[float] = []
        self.analyze_yaw_data: list[float] = []
        self.analyze_heading_sp_data: list[float] = []
        self.analyze_heading_error_data: list[float] = []
        self.analyze_rudder_cmd_data: list[float] = []
        self.analyze_track_wp_data: list[int] = []
        self.analyze_dist_wp_data: list[float] = []
        self.analyze_rpm1_data: list[float] = []
        self.analyze_rpm2_data: list[float] = []
        self.analyze_bat1_data: list[float] = []
        self.analyze_bat2_data: list[float] = []
        self.analyze_mode_auto_data: list[int] = []
        self.analyze_map_coords: list[tuple[float, float]] = []
        self.analyze_heading_values: list[float] = []

        analyze_left_panel_bottom = QGroupBox("Timeline Control", self)
        analyze_left_panel_bottom.setLayout(QVBoxLayout())
        analyze_left_panel_bottom.layout().setContentsMargins(12, 12, 12, 12)
        analyze_left_panel_bottom.layout().setSpacing(8)
        analyze_left_panel_bottom.setStyleSheet("QGroupBox::title { color: #ffffff; }")

        self.analyze_time_slider_scale = 1000
        self.analyze_time_slider_step = 100
        self.analyze_time_slider = QSlider(Qt.Horizontal, self)
        self.analyze_time_slider.setRange(0, 500 * self.analyze_time_slider_scale)
        self.analyze_time_slider.setSingleStep(self.analyze_time_slider_step)
        self.analyze_time_slider.setPageStep(self.analyze_time_slider_step * 5)
        self.analyze_time_slider.setValue(0)
        self.analyze_time_slider.valueChanged.connect(self._on_analyze_time_slider_changed)
        analyze_left_panel_bottom.layout().addWidget(self.analyze_time_slider)

        self.analyze_time_value_label = QLabel("Timestamp: 0 s", self)
        self.analyze_time_value_label.setStyleSheet("color: #e5e7eb; font-weight: 600;")
        analyze_left_panel_bottom.layout().addWidget(self.analyze_time_value_label)
        self._update_analyze_slider_display(0)

        analyze_left_panel.layout().addWidget(analyze_content_splitter, 9)
        analyze_left_panel.layout().addWidget(analyze_left_panel_bottom, 1)

        analyze_right_panel = QWidget(self)
        analyze_right_panel.setLayout(QVBoxLayout())
        analyze_right_panel.layout().setContentsMargins(12, 12, 12, 12)

        analyze_right_placeholder = QGroupBox("", self)
        analyze_right_placeholder.setLayout(QVBoxLayout())
        self.load_csv_btn = QPushButton("Load Recorded CSV", self)
        self.load_csv_btn.clicked.connect(self.load_analyze_csv)
        analyze_right_placeholder.layout().addWidget(self.load_csv_btn)

        map_checkbox_container = QGroupBox("Map Control", self)
        map_checkbox_layout = QHBoxLayout()
        map_checkbox_layout.setContentsMargins(12, 15, 12, 15)
        map_checkbox_layout.setSpacing(12)
        map_checkbox_container.setLayout(map_checkbox_layout)

        self.analyze_show_blue_line_cb = QCheckBox("Trail Line", self)
        self.analyze_show_blue_line_cb.setChecked(True)
        self.analyze_show_blue_line_cb.stateChanged.connect(self.toggle_analyze_map_blue_line)

        self.analyze_show_red_line_cb = QCheckBox("Heading Line", self)
        self.analyze_show_red_line_cb.setChecked(False)
        self.analyze_show_red_line_cb.stateChanged.connect(self.toggle_analyze_map_red_line)

        map_checkbox_layout.addWidget(self.analyze_show_blue_line_cb)
        map_checkbox_layout.addWidget(self.analyze_show_red_line_cb)
        map_checkbox_layout.addStretch(1)
        analyze_right_placeholder.layout().addWidget(map_checkbox_container)

        analyze_indicator_panel = QGroupBox("Recorded Values", self)
        analyze_indicator_panel.setLayout(QVBoxLayout())
        analyze_indicator_panel.layout().setContentsMargins(12, 12, 12, 12)

        analyze_indicator = QWidget(self)
        analyze_indicator.setLayout(QVBoxLayout())
        analyze_indicator.layout().setContentsMargins(0, 0, 0, 0)
        analyze_indicator.layout().setSpacing(8)

        analyze_value_style = "color: #e5e7eb; font-weight: bold; font-size: 12pt; text-align: center;"

        self.analyze_mode_label = QLabel("Manual", self)
        self.analyze_mode_label.setStyleSheet("color: #10b981; font-weight: bold; font-size: 13pt; text-align: center;")
        self.analyze_speed_label = QLabel("0.00 m/s", self)
        self.analyze_speed_label.setStyleSheet(analyze_value_style)
        self.analyze_track_wp_label = QLabel("—", self)
        self.analyze_track_wp_label.setStyleSheet(analyze_value_style)
        self.analyze_dist_wp_label = QLabel("— m", self)
        self.analyze_dist_wp_label.setStyleSheet(analyze_value_style)
        self.analyze_yaw_label_ind = QLabel("0.0°", self)
        self.analyze_yaw_label_ind.setStyleSheet(analyze_value_style)
        self.analyze_hdg_sp_label = QLabel("0.0°", self)
        self.analyze_hdg_sp_label.setStyleSheet(analyze_value_style)
        self.analyze_hdg_err_label = QLabel("0.0°", self)
        self.analyze_hdg_err_label.setStyleSheet(analyze_value_style)
        self.analyze_rudder_cmd_label = QLabel("0.0°", self)
        self.analyze_rudder_cmd_label.setStyleSheet(analyze_value_style)
        self.analyze_rud1_label = QLabel("0.0°", self)
        self.analyze_rud1_label.setStyleSheet(analyze_value_style)
        self.analyze_rud2_label = QLabel("0.0°", self)
        self.analyze_rud2_label.setStyleSheet(analyze_value_style)
        self.analyze_rpm1_label = QLabel("0 RPM", self)
        self.analyze_rpm1_label.setStyleSheet(analyze_value_style)
        self.analyze_rpm2_label = QLabel("0 RPM", self)
        self.analyze_rpm2_label.setStyleSheet(analyze_value_style)
        self.analyze_bat1_label = QLabel("12.00 V", self)
        self.analyze_bat1_label.setStyleSheet("color: #10b981; font-weight: bold; font-size: 12pt; text-align: center;")
        self.analyze_bat2_label = QLabel("12.00 V", self)
        self.analyze_bat2_label.setStyleSheet("color: #10b981; font-weight: bold; font-size: 12pt; text-align: center;")

        def _add_analyze_indicator_row(cells: list[QWidget]) -> None:
            row = QWidget(self)
            row_layout = QHBoxLayout(row)
            row_layout.setContentsMargins(0, 0, 0, 0)
            row_layout.setSpacing(6)
            for cell in cells:
                row_layout.addWidget(cell, 1)
            analyze_indicator.layout().addWidget(row)

        _add_analyze_indicator_row([_make_live_stat_cell(self, "Mode", self.analyze_mode_label)])
        _add_analyze_indicator_row([_make_live_stat_cell(self, "GPS Speed (m/s)", self.analyze_speed_label)])
        _add_analyze_indicator_row([
            _make_live_stat_cell(self, "Track WP", self.analyze_track_wp_label),
            _make_live_stat_cell(self, "Distance WP (m)", self.analyze_dist_wp_label),
        ])
        _add_analyze_indicator_row([
            _make_live_stat_cell(self, "Yaw (°)", self.analyze_yaw_label_ind),
            _make_live_stat_cell(self, "Heading Setpoint (°)", self.analyze_hdg_sp_label),
            _make_live_stat_cell(self, "Heading Error (°)", self.analyze_hdg_err_label),
        ])
        _add_analyze_indicator_row([
            _make_live_stat_cell(self, "Rudder Cmd (°)", self.analyze_rudder_cmd_label),
            _make_live_stat_cell(self, "Rudder 1 (°)", self.analyze_rud1_label),
            _make_live_stat_cell(self, "Rudder 2 (°)", self.analyze_rud2_label),
        ])
        _add_analyze_indicator_row([
            _make_live_stat_cell(self, "RPM Propeller 1", self.analyze_rpm1_label),
            _make_live_stat_cell(self, "RPM Propeller 2", self.analyze_rpm2_label),
        ])
        _add_analyze_indicator_row([
            _make_live_stat_cell(self, "Battery Control", self.analyze_bat1_label),
            _make_live_stat_cell(self, "Battery Motor", self.analyze_bat2_label),
        ])

        analyze_indicator_panel.layout().addWidget(analyze_indicator)
        analyze_right_placeholder.layout().addWidget(analyze_indicator_panel, 1)

        analyze_right_panel.layout().addWidget(analyze_right_placeholder)

        analyze_tab.layout().addWidget(analyze_left_panel, 3)
        analyze_tab.layout().addWidget(analyze_right_panel, 1)
        
        # Tambahkan tab ke tab widget utama
        self.tab_widget.addTab(map_points_tab, "Map Points")
        self.tab_widget.addTab(live_tab, "Live Data")
        self.tab_widget.addTab(analyze_tab, "Analize Data")
        self.tab_widget.setStyleSheet(
            """
            QTabWidget::pane {
                border: 1px solid #374151;
                border-radius: 12px;
                background: #0f172a;
                margin-top: 6px;
            }
            QTabBar::tab {
                background: #1f2937;
                color: #d1d5db;
                padding: 8px 24px;
                border-top-left-radius: 12px;
                border-top-right-radius: 12px;
                font-weight: 600;
                margin-right: 6px;
            }
            QTabBar::tab:hover {
                background: #2563eb;
                color: #f9fafb;
            }
            QTabBar::tab:selected {
                background: #3b82f6;
                color: #ffffff;
            }
            QTabBar::tab:!selected {
                color: #9ca3af;
            }
            """
        )

        # Modern UI styling
        self.setStyleSheet(
            """
            QWidget { font-family: 'Segoe UI', Arial; font-size: 11pt; }
            QGroupBox { border: 1px solid #d0d0d0; border-radius: 10px; margin-top: 10px; }
            QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; color: #222; font-weight: 600; }
            QLabel { color: #333; }
            QComboBox { padding: 6px 8px; border: 1px solid #bdbdbd; border-radius: 8px; background: #fff; }
            QComboBox:focus { border: 1px solid #3b82f6; }
            QComboBox:disabled { background: #f3f4f6; color: #9ca3af; border: 1px solid #e5e7eb; }
            QPushButton { padding: 8px 12px; background-color: #3b82f6; color: #fff; border: none; border-radius: 8px; }
            QPushButton:hover { background-color: #2563eb; }
            QPushButton:checked { background-color: #ef4444; }
            QPushButton:disabled { background-color: #9ca3af; color: #f3f4f6; }
            """
        )

        # Optional: dark theme for controls panel only
        controls_panel.setStyleSheet(
            """
            QGroupBox { background: #1f2937; border: 1px solid #374151; border-radius: 10px; }
            QGroupBox::title { color: #e5e7eb; }
            QLabel { color: #e5e7eb; }
            QComboBox { background: #374151; color: #e5e7eb; border: 1px solid #4b5563; }
            QComboBox:focus { border: 1px solid #60a5fa; }
            QComboBox:disabled { background: #2d3643; color: #9ca3af; border: 1px solid #3b4450; }
            QPushButton { background-color: #3b82f6; color: #fff; }
            QPushButton:hover { background-color: #2563eb; }
            QPushButton:checked { background-color: #ef4444; }
            QPushButton:disabled { background-color: #6b7280; color: #d1d5db; }
            """
        )
        # Apply same dark style to reserved panel
        indicator_panel.setStyleSheet(
            """
            QGroupBox { background: #1f2937; border: 1px solid #374151; border-radius: 10px; }
            QGroupBox::title { color: #e5e7eb; }
            QLabel { color: #e5e7eb; }
            QComboBox { background: #374151; color: #e5e7eb; border: 1px solid #4b5563; }
            QComboBox:focus { border: 1px solid #60a5fa; }
            QComboBox:disabled { background: #2d3643; color: #9ca3af; border: 1px solid #3b4450; }
            QPushButton { background-color: #3b82f6; color: #fff; }
            QPushButton:hover { background-color: #2563eb; }
            QPushButton:checked { background-color: #ef4444; }
            QPushButton:disabled { background-color: #6b7280; color: #d1d5db; }
            QProgressBar { background: #111827; border: 1px solid #374151; border-radius: 6px; text-align: center; }
            QProgressBar::chunk { background-color: #10b981; }
            """
        )
        # Apply dark style to analyze right panel with checkbox styling
        analyze_right_placeholder.setStyleSheet(
            """
            QGroupBox { background: #1f2937; border: 1px solid #374151; border-radius: 10px; }
            QGroupBox::title { color: #e5e7eb; }
            QLabel { color: #e5e7eb; }
            QCheckBox { color: #e5e7eb; }
            QCheckBox::indicator { width: 18px; height: 18px; }
            QCheckBox::indicator:unchecked { background: #374151; border: 1px solid #4b5563; border-radius: 4px; }
            QCheckBox::indicator:checked { background: #3b82f6; border: 1px solid #2563eb; border-radius: 4px; }
            QPushButton { background-color: #3b82f6; color: #fff; }
            QPushButton:hover { background-color: #2563eb; }
            QPushButton:checked { background-color: #ef4444; }
            QPushButton:disabled { background-color: #6b7280; color: #d1d5db; }
            """
        )
        analyze_indicator_panel.setStyleSheet(
            """
            QGroupBox { background: #1f2937; border: 1px solid #374151; border-radius: 10px; }
            QGroupBox::title { color: #e5e7eb; }
            QLabel { color: #e5e7eb; }
            """
        )
        # Apply dark style to Map Points right panel table
        map_points_table_group.setStyleSheet(
            """
            QGroupBox { background: #1f2937; border: 1px solid #374151; border-radius: 10px; }
            QGroupBox::title { color: #e5e7eb; }
            QTableWidget { 
                background: #111827; 
                color: #e5e7eb; 
                border: 1px solid #374151; 
                border-radius: 6px;
                gridline-color: #374151;
            }
            QTableWidget::item {
                padding: 4px;
            }
            QTableWidget::item:selected {
                background: #3b82f6;
                color: #ffffff;
            }
            QHeaderView::section {
                background: #1f2937;
                color: #e5e7eb;
                padding: 6px;
                border: none;
                border-bottom: 1px solid #374151;
                font-weight: 600;
            }
            QPushButton {
                background-color: #ef4444;
                color: #ffffff;
                border: none;
                border-radius: 4px;
                padding: 4px 8px;
                font-size: 10pt;
                font-weight: 600;
            }
            QPushButton:hover {
                background-color: #dc2626;
            }
            QPushButton:pressed {
                background-color: #b91c1c;
            }
            """
        )
        # Apply dark style to "Set Parameter" group on Map Points right panel
        set_param_group.setStyleSheet(
            """
            QGroupBox { background: #1f2937; border: 1px solid #374151; border-radius: 10px; margin-top: 8px; }
            QGroupBox::title { color: #e5e7eb; subcontrol-origin: margin; left: 10px; padding: 0 4px; }
            QLabel { color: #e5e7eb; }
            QLineEdit {
                background: #111827;
                color: #e5e7eb;
                border: 1px solid #374151;
                border-radius: 6px;
                padding: 4px 6px;
                selection-background-color: #3b82f6;
            }
            QLineEdit:focus { border: 1px solid #3b82f6; }
            QPushButton { background-color: #3b82f6; color: #fff; }
            QPushButton:hover { background-color: #2563eb; }
            QPushButton:pressed { background-color: #1d4ed8; }
            QPushButton:disabled { background-color: #6b7280; color: #d1d5db; }
            """
        )

    
    def _update_set_param_status(self, text: str, color: str = "#9ca3af", italic: bool = False):
        """Update label status di group Set Parameter dengan warna konsisten."""
        if not hasattr(self, 'set_param_status_label'):
            return
        italic_css = "italic" if italic else "normal"
        self.set_param_status_label.setText(text)
        self.set_param_status_label.setStyleSheet(
            f"color: {color}; font-style: {italic_css}; padding: 4px 2px 0 2px;"
        )

    def update_waypoints_info_label(self):
        """
        Update label informasi jumlah waypoint di group Send Way Points.

        Layout sekarang: Home (dari "Set Home Point") TERPISAH dari
        click_marker_coords. Total point yang akan dikirim ke remote =
        1 (Home, kalau sudah set) + len(click_marker_coords).
        Minimal 3 total point untuk bisa kirim.

        Dipanggil dari:
        - MapPointsWebView.update_table()          (saat marker add/delete)
        - set_home_point_from_serial()             (saat Home di-set)
        - delete_home_point()                      (saat Home di-hapus)
        """
        if not hasattr(self, 'waypoints_info_label'):
            return
        home_set = bool(getattr(self, 'home_point_coords', None))
        n_wp = 0
        if hasattr(self, 'map_points_webview') and self.map_points_webview:
            n_wp = len(self.map_points_webview.click_marker_coords)
        total = (1 if home_set else 0) + n_wp

        if home_set and total >= 3:
            self.waypoints_info_label.setText(
                f"Home: ✓  WP: {n_wp}  (total {total})"
            )
            self.waypoints_info_label.setStyleSheet(
                "color: #10b981; padding: 2px 0;"
            )
        else:
            need = []
            if not home_set:
                need.append("Home")
            wp_short = max(0, 2 - n_wp) if home_set else max(0, 3 - n_wp - 1)
            if wp_short > 0:
                need.append(f"{wp_short} more WP")
            need_text = " + ".join(need) if need else "—"
            self.waypoints_info_label.setText(
                f"Home: {'✓' if home_set else '✗'}  WP: {n_wp}  (need {need_text})"
            )
            self.waypoints_info_label.setStyleSheet(
                "color: #f59e0b; padding: 2px 0;"
            )
        self._update_live_waypoint_route()

    def _update_live_waypoint_route(self) -> None:
        """Sinkronkan Home + waypoint dari tab Map Points ke peta tab Live."""
        if not hasattr(self, "map_webview") or not self.map_webview:
            return
        home = getattr(self, "home_point_coords", None)
        waypoints: list[tuple[float, float]] = []
        if hasattr(self, "map_points_webview") and self.map_points_webview:
            waypoints = list(self.map_points_webview.click_marker_coords)
        if home is None and not waypoints:
            self.map_webview.clear_waypoint_route()
            return
        self.map_webview.show_waypoint_route(home, waypoints)

    def on_set_param_clicked(self):
        """
        Handler tombol "Send Way Points" di tab Map Points.

        Alur:
        1. Gate: pastikan port serial sudah terkoneksi.
        2. Validasi:
           - Home harus sudah di-set (self.home_point_coords).
           - Total minimal 3 point: 1 Home + minimal 2 waypoint navigasi
             dari self.map_points_webview.click_marker_coords.
           - Tiap koordinat numerik dan dalam rentang lat/lon valid.
        3. Susun payload:
             $WPSET,<home_lat>,<home_lon>,<wp_count>,<lat1>,<lon1>,...,<latN>,<lonN>\\n
           dengan wp_count = jumlah waypoint navigasi.
        4. Tulis ke serial User-Side; tunggu $WACK,OK / $WACK,ERR,... (timeout ~1.5 s).
        5. Di kapal: User-Side → ESP-NOW 0xA1 → Remote menyimpan waypoint dan
           mencetak [WP] ... ke USB Serial mini PC (Cpp_ReadWriteSerial,
           --print all|wp). Dashboard tidak bicara langsung ke mini PC.
        """
        if not self.is_connected():
            self._update_set_param_status(
                "Status: not connected", color="#f59e0b", italic=True
            )
            return

        if getattr(self, 'home_point_coords', None) is None:
            self._update_set_param_status(
                "Status: ERR - Home not set (use Set Home Point)",
                color="#ef4444",
            )
            return

        clicks = []
        if hasattr(self, 'map_points_webview') and self.map_points_webview:
            clicks = list(self.map_points_webview.click_marker_coords)

        total = 1 + len(clicks)
        if total < 3:
            self._update_set_param_status(
                f"Status: ERR - need at least 3 points "
                f"(have Home + {len(clicks)} WP, total {total})",
                color="#ef4444",
            )
            return

        all_points = [self.home_point_coords] + clicks
        for i, point in enumerate(all_points):
            try:
                lat_f = float(point[0])
                lon_f = float(point[1])
            except (TypeError, ValueError, IndexError):
                self._update_set_param_status(
                    f"Status: ERR - point {i} not numeric", color="#ef4444"
                )
                return
            if not (-90.0 <= lat_f <= 90.0):
                tag = "Home" if i == 0 else f"WP {i}"
                self._update_set_param_status(
                    f"Status: ERR - {tag} lat out of range", color="#ef4444"
                )
                return
            if not (-180.0 <= lon_f <= 180.0):
                tag = "Home" if i == 0 else f"WP {i}"
                self._update_set_param_status(
                    f"Status: ERR - {tag} lon out of range", color="#ef4444"
                )
                return

        # Susun payload $WPSET
        home_lat = float(self.home_point_coords[0])
        home_lon = float(self.home_point_coords[1])
        wp_count = len(clicks)
        parts = [
            "$WPSET",
            f"{home_lat:.6f}",
            f"{home_lon:.6f}",
            str(wp_count),
        ]
        for lat_v, lon_v in clicks:
            parts.append(f"{float(lat_v):.6f}")
            parts.append(f"{float(lon_v):.6f}")
        payload = ",".join(parts) + "\n"

        # Simpan snapshot tabel waypoint sebelum mengirim ke remote
        # (agar data yang dikirim bisa dilacak kembali walaupun serial write/ACK gagal).
        try:
            self._save_waypoints_table_snapshot_csv()
        except Exception as e:
            # Jangan mengganggu pengiriman protokol jika file snapshot gagal.
            print(f"[WP SAVE] Failed: {e}")

        try:
            self.ser.write(payload.encode("utf-8"))
            try:
                self.ser.flush()
            except Exception:
                pass
        except Exception as e:
            self._update_set_param_status(
                f"Status: ERR - write failed: {e}", color="#ef4444"
            )
            return

        ts = strftime("%H:%M:%S")
        self._update_set_param_status(
            f"Status: sending {total} points... ({ts})",
            color="#f59e0b",
            italic=True,
        )
        self._set_param_pending = True
        if hasattr(self, 'set_param_btn'):
            self.set_param_btn.setEnabled(False)
        self._set_param_timeout_timer.start()
        self._update_live_waypoint_route()
        # Debug print ke konsol Python supaya mudah verifikasi payload
        print(f"[WPSET] {payload.strip()}")

    def _save_waypoints_table_snapshot_csv(self) -> str | None:
        """
        Simpan isi tabel Map Points (kolom No/Lat/Long) ke file CSV snapshot.

        File tersimpan di folder 'WayPoints' yang lokasinya berdampingan dengan file py ini,
        dengan nama: DDMMYYYY_HHMM_WayPoints.csv
        """
        if not hasattr(self, "map_points_table") or not self.map_points_table:
            return None

        base_dir = os.path.dirname(os.path.abspath(__file__))
        out_dir = os.path.join(base_dir, "WayPoints")
        os.makedirs(out_dir, exist_ok=True)

        ts = datetime.now().strftime("%d%m%Y_%H%M")
        base_name = f"{ts}_WayPoints"
        file_path = os.path.join(out_dir, f"{base_name}.csv")

        # Hindari overwrite jika user menekan dalam menit yang sama
        if os.path.exists(file_path):
            idx = 1
            while True:
                candidate = os.path.join(out_dir, f"{base_name}_{idx:02d}.csv")
                if not os.path.exists(candidate):
                    file_path = candidate
                    break
                idx += 1

        rows: list[tuple[str, str, str]] = []
        # Table row 0 berisi Home ("No" = "Home"), waypoint klik mulai dari row 1
        for r in range(self.map_points_table.rowCount()):
            no_item = self.map_points_table.item(r, 0)
            lat_item = self.map_points_table.item(r, 1)
            lon_item = self.map_points_table.item(r, 2)

            lat_txt = lat_item.text().strip() if lat_item and lat_item.text() else ""
            lon_txt = lon_item.text().strip() if lon_item and lon_item.text() else ""
            if not lat_txt or not lon_txt:
                continue

            no_txt = no_item.text().strip() if no_item and no_item.text() else ""
            rows.append((no_txt, lat_txt, lon_txt))

        if not rows:
            return None

        with open(file_path, "w", encoding="utf-8", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(["No", "Lat", "Long"])
            writer.writerows(rows)

        print(f"[WP SAVE] Snapshot saved: {file_path}")
        return file_path

    def _on_set_param_timeout(self):
        """Dipanggil bila tidak ada $WACK,... dalam 1.5 detik setelah pengiriman."""
        if not self._set_param_pending:
            return
        self._set_param_pending = False
        ts = strftime("%H:%M:%S")
        self._update_set_param_status(
            f"Status: TIMEOUT no ACK ({ts})", color="#f59e0b"
        )
        if self.is_connected() and hasattr(self, 'set_param_btn'):
            self.set_param_btn.setEnabled(True)

    def _handle_set_param_response(self, text: str):
        """
        Diparse dari poll_serial saat baris diawali '$WACK' (atau '$PACK'
        legacy; lihat poll_serial filter).

        Format yang diharapkan:
          $WACK,OK
          $WACK,ERR,<reason>[,<extra>...]
        """
        if self._set_param_timeout_timer.isActive():
            self._set_param_timeout_timer.stop()
        self._set_param_pending = False

        parts = [p.strip() for p in text.split(",")]
        ts = strftime("%H:%M:%S")
        if len(parts) >= 2 and parts[1] == "OK":
            self._update_set_param_status(
                f"Status: OK ({ts})", color="#10b981"
            )
        elif len(parts) >= 3 and parts[1] == "ERR":
            # Gabungkan reason + field extra (mis. COUNT_MISMATCH,5,exp,7)
            reason = ",".join(parts[2:])
            self._update_set_param_status(
                f"Status: ERR - {reason} ({ts})", color="#ef4444"
            )
        else:
            self._update_set_param_status(
                f"Status: {text} ({ts})", color="#f59e0b"
            )

        if self.is_connected() and hasattr(self, 'set_param_btn'):
            self.set_param_btn.setEnabled(True)


    def update_indicators(self, yaw: float, heading_setpoint: float, heading_error: float,
                          rudder_cmd: float, rud1_sensor: float, rud2_sensor: float,
                          rpm1: float, rpm2: float, bat1: float, bat2: float,
                          speed: float, track_wp_index: int, distance_to_wp: float,
                          mode_auto: int = 0, timestamp: float = 0.0,
                          mini_pc_link: int = 0):
        """Update panel indikator live dan time-series plots."""
        try:
            self.yaw_label.setText(f"{yaw:.1f}°")
            self.hdg_sp_label.setText(f"{heading_setpoint:.1f}°")
            self.hdg_err_label.setText(f"{heading_error:.1f}°")
            self.rudder_cmd_label.setText(f"{rudder_cmd:.1f}°")
            self.track_wp_label.setText(_format_track_wp_index(track_wp_index))
            if track_wp_index == 0:
                self.dist_wp_label.setText("— m")
            else:
                self.dist_wp_label.setText(f"{distance_to_wp:.1f} m")
            self.rud1_label.setText(f"{rud1_sensor:.1f}°")
            self.rud2_label.setText(f"{rud2_sensor:.1f}°")
        except Exception:
            pass

        try:
            sp = max(0.0, min(20.0, float(speed)))
            self.speed_label.setText(f"{sp:.2f} m/s")
        except Exception:
            pass

        try:
            self.rpm1_label.setText(f"{int(rpm1)} RPM")
            self.rpm2_label.setText(f"{int(rpm2)} RPM")
        except Exception:
            pass

        # Yaw + heading setpoint plot
        try:
            self.yaw_time_data.append(timestamp)
            self.yaw_data.append(yaw)
            self.heading_sp_data.append(heading_setpoint)
            while len(self.yaw_time_data) > self.max_points:
                self.yaw_time_data.pop(0)
                self.yaw_data.pop(0)
                self.heading_sp_data.pop(0)
            self.yaw_curve.setData(self.yaw_time_data, self.yaw_data)
            self.heading_sp_curve.setData(self.yaw_time_data, self.heading_sp_data)
        except Exception as e:
            print(f"[ERROR] Yaw plot update failed: {e}")

        # Rudder 1: cmd + sensor
        try:
            self.rudder1_time_data.append(timestamp)
            self.rudder1_cmd_data.append(rudder_cmd)
            self.rudder1_sensor_data.append(rud1_sensor)
            while len(self.rudder1_time_data) > self.max_points:
                self.rudder1_time_data.pop(0)
                self.rudder1_cmd_data.pop(0)
                self.rudder1_sensor_data.pop(0)
            self.rudder1_cmd_curve.setData(self.rudder1_time_data, self.rudder1_cmd_data)
            self.rudder1_sensor_curve.setData(self.rudder1_time_data, self.rudder1_sensor_data)
        except Exception as e:
            print(f"[ERROR] Rudder 1 plot update failed: {e}")

        # Rudder 2: cmd + sensor
        try:
            self.rudder2_time_data.append(timestamp)
            self.rudder2_cmd_data.append(rudder_cmd)
            self.rudder2_sensor_data.append(rud2_sensor)
            while len(self.rudder2_time_data) > self.max_points:
                self.rudder2_time_data.pop(0)
                self.rudder2_cmd_data.pop(0)
                self.rudder2_sensor_data.pop(0)
            self.rudder2_cmd_curve.setData(self.rudder2_time_data, self.rudder2_cmd_data)
            self.rudder2_sensor_curve.setData(self.rudder2_time_data, self.rudder2_sensor_data)
        except Exception as e:
            print(f"[ERROR] Rudder 2 plot update failed: {e}")

        def _bat_color(voltage: float) -> str:
            if voltage < 10.5:
                return '#ef4444'
            if voltage < 11.5:
                return '#f59e0b'
            return '#10b981'

        try:
            v1 = float(bat1)
            v2 = float(bat2)
            self.bat1_label.setText(f"{v1:.2f} V")
            self.bat2_label.setText(f"{v2:.2f} V")
            self.bat1_label.setStyleSheet(
                f"color: {_bat_color(v1)}; font-weight: bold; font-size: 12pt;")
            self.bat2_label.setStyleSheet(
                f"color: {_bat_color(v2)}; font-weight: bold; font-size: 12pt;")
        except Exception:
            pass

        mode_descriptions = {
            0: "Manual",
            1: "Auto Alg 1 (PD)",
            2: "Auto Mini PC",
        }
        mode_colors = {
            0: "#6b7280",
            1: "#10b981",
            2: "#3b82f6",
        }
        try:
            mode_int = int(mode_auto)
            mode_text = mode_descriptions.get(mode_int, f"Unknown ({mode_int})")
            mode_color = mode_colors.get(mode_int, "#6b7280")
            self.mode_label.setText(mode_text)
            self.mode_label.setStyleSheet(f"color: {mode_color}; font-weight: bold;")
        except Exception:
            pass

        try:
            if int(mini_pc_link) == 1:
                self.mini_pc_link_label.setText("CONNECTED")
                self.mini_pc_link_label.setStyleSheet(
                    "color: #10b981; font-weight: bold; font-size: 11pt; text-align: center;")
            else:
                self.mini_pc_link_label.setText("DISCONNECTED")
                self.mini_pc_link_label.setStyleSheet(
                    "color: #ef4444; font-weight: bold; font-size: 11pt; text-align: center;")
        except Exception:
            pass

        try:
            mode_int = int(mode_auto)
            if mode_int == 2 and int(mini_pc_link) == 0:
                self.auto_warn_label.setText("⚠ Auto aktif — Mini PC tidak terhubung")
            else:
                self.auto_warn_label.setText("")
        except Exception:
            pass

    def clear_all_plots(self):
        """Clear semua plot data pada tab Live."""
        self.yaw_time_data.clear()
        self.yaw_data.clear()
        self.heading_sp_data.clear()
        self.yaw_curve.setData([], [])
        self.heading_sp_curve.setData([], [])

        self.rudder1_time_data.clear()
        self.rudder1_cmd_data.clear()
        self.rudder1_sensor_data.clear()
        self.rudder1_cmd_curve.setData([], [])
        self.rudder1_sensor_curve.setData([], [])

        self.rudder2_time_data.clear()
        self.rudder2_cmd_data.clear()
        self.rudder2_sensor_data.clear()
        self.rudder2_cmd_curve.setData([], [])
        self.rudder2_sensor_curve.setData([], [])

        print("[PLOTS] All plots cleared")
    
    def clear_analyze_plots(self):
        """Clear semua data plot dan panel pada tab Analyze."""
        self.analyze_time_data.clear()
        self.analyze_lat_data.clear()
        self.analyze_lon_data.clear()
        self.analyze_speed_data.clear()
        self.analyze_rud1_sensor_data.clear()
        self.analyze_rud2_sensor_data.clear()
        self.analyze_yaw_data.clear()
        self.analyze_heading_sp_data.clear()
        self.analyze_heading_error_data.clear()
        self.analyze_rudder_cmd_data.clear()
        self.analyze_track_wp_data.clear()
        self.analyze_dist_wp_data.clear()
        self.analyze_rpm1_data.clear()
        self.analyze_rpm2_data.clear()
        self.analyze_bat1_data.clear()
        self.analyze_bat2_data.clear()
        self.analyze_mode_auto_data.clear()
        self.analyze_map_coords.clear()
        self.analyze_heading_values.clear()

        self.analyze_yaw_curve.setData([], [])
        self.analyze_heading_sp_curve.setData([], [])
        self.analyze_rudder1_cmd_curve.setData([], [])
        self.analyze_rudder1_sensor_curve.setData([], [])
        self.analyze_rudder2_cmd_curve.setData([], [])
        self.analyze_rudder2_sensor_curve.setData([], [])
        self._hide_analyze_yaw_marker()
        self._hide_analyze_rudder1_marker()
        self._hide_analyze_rudder2_marker()

        self.analyze_map_webview.clear_markers()
        self.analyze_map_webview.trail_coords = []
        self.analyze_map_webview.marker_count = 0

        if hasattr(self, "analyze_time_slider"):
            max_default = 500 * getattr(self, "analyze_time_slider_scale", 1)
            self.analyze_time_slider.setRange(0, max_default)
            self.analyze_time_slider.setValue(0)
            self._update_analyze_slider_display(0)

    def _analyze_index_at_timestamp(self, timestamp: float) -> int:
        if not self.analyze_time_data:
            return -1
        idx = bisect_left(self.analyze_time_data, timestamp)
        if idx >= len(self.analyze_time_data):
            idx = len(self.analyze_time_data) - 1
        elif idx > 0 and idx < len(self.analyze_time_data):
            prev_time = self.analyze_time_data[idx - 1]
            curr_time = self.analyze_time_data[idx]
            if abs(timestamp - prev_time) <= abs(curr_time - timestamp):
                idx -= 1
        return max(0, idx)

    def _update_analyze_indicators(self, idx: int) -> None:
        if idx < 0 or idx >= len(self.analyze_time_data):
            return
        try:
            self.analyze_speed_label.setText(f"{self.analyze_speed_data[idx]:.2f} m/s")
            self.analyze_track_wp_label.setText(_format_track_wp_index(self.analyze_track_wp_data[idx]))
            if self.analyze_track_wp_data[idx] == 0:
                self.analyze_dist_wp_label.setText("— m")
            else:
                self.analyze_dist_wp_label.setText(f"{self.analyze_dist_wp_data[idx]:.1f} m")
            self.analyze_yaw_label_ind.setText(f"{self.analyze_yaw_data[idx]:.1f}°")
            self.analyze_hdg_sp_label.setText(f"{self.analyze_heading_sp_data[idx]:.1f}°")
            self.analyze_hdg_err_label.setText(f"{self.analyze_heading_error_data[idx]:.1f}°")
            self.analyze_rudder_cmd_label.setText(f"{self.analyze_rudder_cmd_data[idx]:.1f}°")
            self.analyze_rud1_label.setText(f"{self.analyze_rud1_sensor_data[idx]:.1f}°")
            self.analyze_rud2_label.setText(f"{self.analyze_rud2_sensor_data[idx]:.1f}°")
            self.analyze_rpm1_label.setText(f"{int(self.analyze_rpm1_data[idx])} RPM")
            self.analyze_rpm2_label.setText(f"{int(self.analyze_rpm2_data[idx])} RPM")
        except Exception:
            pass

        def _bat_color(voltage: float) -> str:
            if voltage < 10.5:
                return '#ef4444'
            if voltage < 11.5:
                return '#f59e0b'
            return '#10b981'

        mode_descriptions = {0: "Manual", 1: "Auto Alg 1", 2: "Auto Alg 2"}
        mode_colors = {0: "#6b7280", 1: "#10b981", 2: "#3b82f6"}
        try:
            mode_int = int(self.analyze_mode_auto_data[idx])
            mode_text = mode_descriptions.get(mode_int, f"Unknown ({mode_int})")
            mode_color = mode_colors.get(mode_int, "#6b7280")
            self.analyze_mode_label.setText(mode_text)
            self.analyze_mode_label.setStyleSheet(
                f"color: {mode_color}; font-weight: bold; font-size: 13pt; text-align: center;")
            v1 = float(self.analyze_bat1_data[idx])
            v2 = float(self.analyze_bat2_data[idx])
            self.analyze_bat1_label.setText(f"{v1:.2f} V")
            self.analyze_bat2_label.setText(f"{v2:.2f} V")
            self.analyze_bat1_label.setStyleSheet(
                f"color: {_bat_color(v1)}; font-weight: bold; font-size: 12pt; text-align: center;")
            self.analyze_bat2_label.setStyleSheet(
                f"color: {_bat_color(v2)}; font-weight: bold; font-size: 12pt; text-align: center;")
        except Exception:
            pass

    def _update_analyze_at_timestamp(self, timestamp: float) -> None:
        self._update_analyze_yaw_marker(timestamp)
        self._update_analyze_rudder1_marker(timestamp)
        self._update_analyze_rudder2_marker(timestamp)
        self._update_analyze_map_marker(timestamp)
        idx = self._analyze_index_at_timestamp(timestamp)
        self._update_analyze_indicators(idx)

    def _update_analyze_slider_display(self, value: int | None = None):
        """
        Update label dan tooltip slider timeline Analyze.
        """
        if not hasattr(self, "analyze_time_slider"):
            return
        if value is None:
            value = self.analyze_time_slider.value()
        scale = getattr(self, "analyze_time_slider_scale", 1)
        seconds = value / scale if scale else float(value)
        self.analyze_time_slider.setToolTip(f"Timestamp: {seconds:.3f} s")
        if hasattr(self, "analyze_time_value_label"):
            self.analyze_time_value_label.setText(f"Timestamp: {seconds:.3f} s")
    
    def _on_analyze_time_slider_changed(self, value: int):
        """Snap slider ke step dan update plot, peta, panel indikator."""
        if not hasattr(self, "analyze_time_slider"):
            return
        snapped_value = self._snap_analyze_slider_value(value)
        if snapped_value != value:
            self.analyze_time_slider.blockSignals(True)
            self.analyze_time_slider.setValue(snapped_value)
            self.analyze_time_slider.blockSignals(False)
        self._update_analyze_slider_display(snapped_value)
        scale = getattr(self, "analyze_time_slider_scale", 1)
        seconds = snapped_value / scale if scale else float(snapped_value)
        self._update_analyze_at_timestamp(seconds)

    def _snap_analyze_slider_value(self, value: int) -> int:
        if not hasattr(self, "analyze_time_slider"):
            return value
        step = getattr(self, "analyze_time_slider_step", 1)
        if step <= 0:
            return value
        slider_min = self.analyze_time_slider.minimum()
        relative = value - slider_min
        snapped_relative = round(relative / step) * step
        return slider_min + snapped_relative

    @staticmethod
    def _interpolate_analyze_series_value(time_data: list[float], value_data: list[float], x_value: float) -> float | None:
        if not time_data or not value_data:
            return None
        index = bisect_left(time_data, x_value)
        if index <= 0:
            return value_data[0]
        if index >= len(time_data):
            return value_data[-1]
        x0 = time_data[index - 1]
        x1 = time_data[index]
        y0 = value_data[index - 1]
        y1 = value_data[index]
        if x1 == x0:
            return y0
        ratio = (x_value - x0) / (x1 - x0)
        return y0 + ratio * (y1 - y0)

    def _update_analyze_yaw_marker(self, timestamp: float | None = None):
        if not hasattr(self, "analyze_yaw_plot_widget"):
            return
        if timestamp is None:
            scale = getattr(self, "analyze_time_slider_scale", 1)
            timestamp = self.analyze_time_slider.value() / scale if scale else 0.0
        if not self.analyze_time_data:
            self._hide_analyze_yaw_marker()
            return
        yaw_val = self._interpolate_analyze_series_value(self.analyze_time_data, self.analyze_yaw_data, timestamp)
        hdg_sp = self._interpolate_analyze_series_value(self.analyze_time_data, self.analyze_heading_sp_data, timestamp)
        if yaw_val is None and hdg_sp is None:
            self._hide_analyze_yaw_marker()
            return
        label_lines = [f"t={timestamp:.3f} s"]
        if yaw_val is not None:
            label_lines.append(f"Yaw: {yaw_val:.2f}°")
        if hdg_sp is not None:
            label_lines.append(f"Setpoint: {hdg_sp:.2f}°")
        y_val = yaw_val if yaw_val is not None else hdg_sp
        if y_val is not None:
            self.analyze_yaw_label.setText("\n".join(label_lines))
            self.analyze_yaw_label.setPos(timestamp, y_val)
            self.analyze_yaw_label.show()
        if yaw_val is not None:
            self.analyze_yaw_target.setPos(timestamp, yaw_val)
            self.analyze_yaw_target.show()
        else:
            self.analyze_yaw_target.hide()
        if hdg_sp is not None:
            self.analyze_heading_sp_target.setPos(timestamp, hdg_sp)
            self.analyze_heading_sp_target.show()
        else:
            self.analyze_heading_sp_target.hide()

    def _hide_analyze_yaw_marker(self):
        if hasattr(self, "analyze_yaw_label"):
            self.analyze_yaw_label.hide()
        if hasattr(self, "analyze_yaw_target"):
            self.analyze_yaw_target.hide()
        if hasattr(self, "analyze_heading_sp_target"):
            self.analyze_heading_sp_target.hide()

    def _update_analyze_rudder1_marker(self, timestamp: float | None = None):
        if not hasattr(self, "analyze_rudder1_plot_widget"):
            return
        if timestamp is None:
            scale = getattr(self, "analyze_time_slider_scale", 1)
            timestamp = self.analyze_time_slider.value() / scale if scale else 0.0
        if not self.analyze_time_data:
            self._hide_analyze_rudder1_marker()
            return
        cmd = self._interpolate_analyze_series_value(self.analyze_time_data, self.analyze_rudder_cmd_data, timestamp)
        sensor = self._interpolate_analyze_series_value(self.analyze_time_data, self.analyze_rud1_sensor_data, timestamp)
        if cmd is None and sensor is None:
            self._hide_analyze_rudder1_marker()
            return
        label_lines = [f"t={timestamp:.3f} s"]
        if cmd is not None:
            label_lines.append(f"Cmd: {cmd:.2f}°")
        if sensor is not None:
            label_lines.append(f"Sensor: {sensor:.2f}°")
        y_val = cmd if cmd is not None else sensor
        if y_val is not None:
            self.analyze_rudder1_label.setText("\n".join(label_lines))
            self.analyze_rudder1_label.setPos(timestamp, y_val)
            self.analyze_rudder1_label.show()
        if cmd is not None:
            self.analyze_rudder1_cmd_target.setPos(timestamp, cmd)
            self.analyze_rudder1_cmd_target.show()
        else:
            self.analyze_rudder1_cmd_target.hide()
        if sensor is not None:
            self.analyze_rudder1_sensor_target.setPos(timestamp, sensor)
            self.analyze_rudder1_sensor_target.show()
        else:
            self.analyze_rudder1_sensor_target.hide()

    def _hide_analyze_rudder1_marker(self):
        if hasattr(self, "analyze_rudder1_label"):
            self.analyze_rudder1_label.hide()
        if hasattr(self, "analyze_rudder1_cmd_target"):
            self.analyze_rudder1_cmd_target.hide()
        if hasattr(self, "analyze_rudder1_sensor_target"):
            self.analyze_rudder1_sensor_target.hide()

    def _update_analyze_rudder2_marker(self, timestamp: float | None = None):
        if not hasattr(self, "analyze_rudder2_plot_widget"):
            return
        if timestamp is None:
            scale = getattr(self, "analyze_time_slider_scale", 1)
            timestamp = self.analyze_time_slider.value() / scale if scale else 0.0
        if not self.analyze_time_data:
            self._hide_analyze_rudder2_marker()
            return
        cmd = self._interpolate_analyze_series_value(self.analyze_time_data, self.analyze_rudder_cmd_data, timestamp)
        sensor = self._interpolate_analyze_series_value(self.analyze_time_data, self.analyze_rud2_sensor_data, timestamp)
        if cmd is None and sensor is None:
            self._hide_analyze_rudder2_marker()
            return
        label_lines = [f"t={timestamp:.3f} s"]
        if cmd is not None:
            label_lines.append(f"Cmd: {cmd:.2f}°")
        if sensor is not None:
            label_lines.append(f"Sensor: {sensor:.2f}°")
        y_val = cmd if cmd is not None else sensor
        if y_val is not None:
            self.analyze_rudder2_label.setText("\n".join(label_lines))
            self.analyze_rudder2_label.setPos(timestamp, y_val)
            self.analyze_rudder2_label.show()
        if cmd is not None:
            self.analyze_rudder2_cmd_target.setPos(timestamp, cmd)
            self.analyze_rudder2_cmd_target.show()
        else:
            self.analyze_rudder2_cmd_target.hide()
        if sensor is not None:
            self.analyze_rudder2_sensor_target.setPos(timestamp, sensor)
            self.analyze_rudder2_sensor_target.show()
        else:
            self.analyze_rudder2_sensor_target.hide()

    def _hide_analyze_rudder2_marker(self):
        if hasattr(self, "analyze_rudder2_label"):
            self.analyze_rudder2_label.hide()
        if hasattr(self, "analyze_rudder2_cmd_target"):
            self.analyze_rudder2_cmd_target.hide()
        if hasattr(self, "analyze_rudder2_sensor_target"):
            self.analyze_rudder2_sensor_target.hide()

    def _update_analyze_map_marker(self, timestamp: float | None = None):
        if not hasattr(self, "analyze_map_webview") or not self.analyze_map_coords:
            return
        if timestamp is None:
            scale = getattr(self, "analyze_time_slider_scale", 1)
            timestamp = self.analyze_time_slider.value() / scale if scale else 0.0
        idx = self._analyze_index_at_timestamp(timestamp)
        if idx < 0 or idx >= len(self.analyze_map_coords):
            return
        coords = self.analyze_map_coords[idx]
        heading = self.analyze_heading_values[idx] if idx < len(self.analyze_heading_values) else None
        try:
            self.analyze_map_webview.move_start_marker(coords, heading, timestamp)
            self.analyze_map_webview.update_slider_heading_line(coords, heading)
        except Exception:
            pass

    def refresh_ports(self):
        """
        Refresh daftar port COM yang tersedia.
        
        Method ini:
        - Mendeteksi semua port COM yang tersedia
        - Update combo box dengan port yang terdeteksi
        - Auto-select port jika hanya ada satu port
        - Prefer port yang sebelumnya dipilih jika masih tersedia
        """
        current = self.port_combo.currentText() if hasattr(self, 'port_combo') else ""
        self.port_combo.clear()
        ports = list(list_ports.comports())
        items = [p.device for p in ports]
        # Prefer last used/current if still present
        self.port_combo.addItems(items)
        if current and current in items:
            self.port_combo.setCurrentText(current)
        elif len(items) == 1:
            self.port_combo.setCurrentIndex(0)
        print(f"🔌 Ports detected: {items}")
    
    def set_home_point_from_serial(self):
        """
        Ambil koordinat Home point dari data serial terbaru dan update tabel serta peta.
        """
        if self.latest_serial_lat is None or self.latest_serial_lon is None:
            from PySide6.QtWidgets import QMessageBox
            msg = QMessageBox(self)
            msg.setIcon(QMessageBox.Warning)
            msg.setWindowTitle("No Data")
            msg.setText("No serial data available.")
            msg.setInformativeText("Please wait for serial data to be received.")
            msg.exec()
            return
        
        # Set Home point coordinates
        self.home_point_coords = (self.latest_serial_lat, self.latest_serial_lon)
        
        # Update tabel baris 1 (Home)
        self.update_home_point_table()
        
        # Draw marker Home di peta + segmen garis Home → WP1
        self.map_points_webview.set_route_home_coords(self.home_point_coords)
        self.map_points_webview.add_home_marker(self.home_point_coords)

        # Update label info Send Way Points (Home + WP count)
        self.update_waypoints_info_label()

        print(f"[MAP POINTS] Home point set: Lat={self.home_point_coords[0]:.6f}, Lon={self.home_point_coords[1]:.6f}")
    
    def update_home_point_table(self):
        """
        Update tabel baris 1 (Home) dengan koordinat Home point dan tombol hapus.
        """
        if not self.map_points_table:
            return
        
        # Clear baris Home terlebih dahulu
        for col in range(self.map_points_table.columnCount()):
            item = self.map_points_table.item(0, col)
            if item:
                item.setText("")
        widget = self.map_points_table.cellWidget(0, 3)
        if widget:
            self.map_points_table.removeCellWidget(0, 3)
        
        # Set "Home" di kolom No
        item_home = QTableWidgetItem("Home")
        item_home.setTextAlignment(Qt.AlignCenter)
        self.map_points_table.setItem(0, 0, item_home)
        
        # Set koordinat jika Home point sudah di-set
        if self.home_point_coords:
            # Kolom Lat
            item_lat = QTableWidgetItem(f"{self.home_point_coords[0]:.6f}")
            item_lat.setTextAlignment(Qt.AlignRight | Qt.AlignVCenter)
            self.map_points_table.setItem(0, 1, item_lat)
            
            # Kolom Long
            item_lon = QTableWidgetItem(f"{self.home_point_coords[1]:.6f}")
            item_lon.setTextAlignment(Qt.AlignRight | Qt.AlignVCenter)
            self.map_points_table.setItem(0, 2, item_lon)
            
            # Kolom Action - tombol Hapus untuk Home
            delete_home_btn = QPushButton("Hapus", self)
            delete_home_btn.setMaximumWidth(60)
            delete_home_btn.setMaximumHeight(25)
            delete_home_btn.clicked.connect(self.delete_home_point)
            self.map_points_table.setCellWidget(0, 3, delete_home_btn)
    
    def delete_home_point(self):
        """
        Hapus Home point dari tabel dan peta.
        """
        # Clear Home point coordinates
        self.home_point_coords = None
        
        # Update tabel
        self.update_home_point_table()
        
        # Hapus marker Home dari peta dan segmen garis dari Home
        self.map_points_webview.set_route_home_coords(None)
        self.map_points_webview.remove_home_marker()

        # Update label info Send Way Points (Home + WP count)
        self.update_waypoints_info_label()

        print("[MAP POINTS] Home point deleted")

    def toggle_connection(self, checked: bool):
        if checked:
            ok = self.connect_serial()
            if not ok:
                # revert button state if failed
                self.connect_btn.setChecked(False)
        else:
            self.disconnect_serial()

    def connect_serial(self) -> bool:
        """
        Koneksi ke serial port yang dipilih.
        
        Returns:
            True jika koneksi berhasil, False jika gagal
            
        Method ini:
        - Membaca port dan baud rate dari combo box
        - Membaca map marker rate dan mengkonversi ke interval
        - Membuka serial port
        - Clear plots dan map markers
        - Start polling timer
        - Enable logging toggle
        """
        port = self.port_combo.currentText().strip()
        if not port:
            print("[SERIAL] No port selected")
            return False
        try:
            baud = int(self.baud_combo.currentText())
        except ValueError:
            baud = 115200
        
        # Read plot interval before connecting
        # Convert from "marker per detik" to "interval" (@ 10Hz sampling)
        # Formula: interval = 10 / marker_per_detik
        try:
            marker_per_detik = float(self.plot_int_combo.currentText())
            self.plot_interval = int(10.0 / marker_per_detik)
            self.plot_counter = 0  # Reset counter
        except (ValueError, ZeroDivisionError):
            self.plot_interval = 10  # Default fallback (1 marker/detik)
            self.plot_counter = 0
        
        try:
            self.ser = serial.Serial(port, baudrate=baud, timeout=0.1)
            # small delay for device ready
            QTimer.singleShot(200, lambda: None)
            try:
                self.ser.reset_input_buffer()
            except Exception:
                pass
            # Clear all plots when connecting
            self.clear_all_plots()
            # Clear all markers from map when connecting
            self.map_webview.clear_markers()
            self._update_live_waypoint_route()
            self.serial_timer.start(50)  # poll every 50 ms
            self.connect_btn.setText("Disconnect")
            print(f"[SERIAL] Connected to {port} @ {baud}")
            marker_per_detik = 10.0 / self.plot_interval
            print(f"[MAP] Map marker rate: {marker_per_detik:.1f} marker/detik (interval: {self.plot_interval} data)")
            
            # Disable controls when connected
            self.port_combo.setEnabled(False)
            self.baud_combo.setEnabled(False)
            self.plot_int_combo.setEnabled(False)
            self.refresh_btn.setEnabled(False)
            
            # Enable logging toggle only when connected
            if self.log_btn:
                self.log_btn.setEnabled(True)
            
            # Enable Home Points button when connected
            if hasattr(self, 'home_points_btn'):
                self.home_points_btn.setEnabled(True)

            # Enable Set Param button when connected (gate by is_connected)
            if hasattr(self, 'set_param_btn'):
                self.set_param_btn.setEnabled(True)

            return True
        except Exception as e:
            print(f"[SERIAL] Connect failed: {e}")
            self.ser = None
            return False

    def disconnect_serial(self):
        """
        Putus koneksi serial port.
        
        Method ini:
        - Stop polling timer
        - Close serial port
        - Reset serial object
        - Update button text
        - Re-enable controls
        - Stop logging jika aktif
        - Disable logging toggle
        """
        try:
            if self.serial_timer.isActive():
                self.serial_timer.stop()
            if self.ser and self.ser.is_open:
                self.ser.close()
        except Exception:
            pass
        finally:
            self.ser = None
            self.connect_btn.setText("Connect")
            print("[SERIAL] Disconnected")
            
            # Re-enable controls when disconnected
            self.port_combo.setEnabled(True)
            self.baud_combo.setEnabled(True)
            self.plot_int_combo.setEnabled(True)
            self.refresh_btn.setEnabled(True)
            
            # stop logging if active (optional safety)
            if self.log_btn and self.log_btn.isChecked():
                self.log_btn.setChecked(False)
                self.stop_logging()
            # Disable logging toggle when disconnected
            if self.log_btn:
                self.log_btn.setEnabled(False)
            
            # Disable Home Points button when disconnected
            if hasattr(self, 'home_points_btn'):
                self.home_points_btn.setEnabled(False)

            # Disable Set Param button when disconnected (gate by is_connected)
            if hasattr(self, 'set_param_btn'):
                self.set_param_btn.setEnabled(False)
            # Reset Set Param state (timeout timer + pending flag) saat disconnect
            try:
                if hasattr(self, '_set_param_timeout_timer') and self._set_param_timeout_timer.isActive():
                    self._set_param_timeout_timer.stop()
                self._set_param_pending = False
            except Exception:
                pass
            # Reset status label ke idle saat disconnect
            if hasattr(self, 'set_param_status_label'):
                self.set_param_status_label.setText("Status: idle")
                self.set_param_status_label.setStyleSheet(
                    "color: #9ca3af; font-style: italic; padding: 4px 2px 0 2px;"
                )

            # Reset latest serial coordinates
            self.latest_serial_lat = None
            self.latest_serial_lon = None
            self.latest_serial_heading = None

            # Reset label live position di Map Points
            if hasattr(self, 'live_lat_val'):
                self.live_lat_val.setText("—")
                self.live_lon_val.setText("—")
                self.live_hdg_val.setText("—")

    def _apply_rudder_correction_deg(
        self, value_deg: float, enabled: bool, correction_deg: float,
    ) -> float:
        if not enabled:
            return value_deg
        return value_deg - correction_deg

    def _open_live_setup_dialog(self) -> None:
        dialog = LiveRudderSetupDialog(self)
        if dialog.exec() == QDialog.DialogCode.Accepted:
            dialog.apply_to_parent(self)

    def poll_serial(self):
        """
        Poll serial port untuk membaca data baru.
        
        Method ini:
        - Membaca data dari serial port dalam buffer
        - Memproses data CSV (23 kolom)
        - Update map dengan decimation (setiap N data)
        - Update indicators untuk setiap data (real-time)
        - Append data ke log buffer jika logging aktif
        
        Format data yang diharapkan (23 kolom, raw fixed-point):
        timestamp,latitude,longitude,speedMps,Calc_deg_servo_1,Calc_deg_servo_2,
        yaw,heading_setpoint,heading_error,rudder_cmd,track_wp_index,distance_to_wp,
        accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,
        rpm_prop_1,rpm_prop_2,battery_1,battery_2,mode_auto
        """
        if not self.ser:
            return
        try:
            # Buffering untuk memastikan hanya memproses baris utuh (akhiri dengan \n)
            if not hasattr(self, 'serial_buffer'):
                self.serial_buffer = b''
            available = self.ser.in_waiting
            chunk = self.ser.read(available or 1)
            if not chunk:
                return
            self.serial_buffer += chunk
            while b"\n" in self.serial_buffer:
                line, self.serial_buffer = self.serial_buffer.split(b"\n", 1)
                text = line.decode('utf-8', errors='replace').strip()
                if not text:
                    continue
                # Tangkap respons control protocol dari user-side ESP32 sebelum
                # filter telemetri 23-kolom, agar tidak ikut di-drop.
                # $WACK,... = balasan baru untuk $WPSET (Send Way Points).
                # $PACK,... = balasan lama untuk $PARAM (deprecated, masih
                #             di-handle untuk backward compatibility singkat).
                if text.startswith("$WACK") or text.startswith("$PACK"):
                    self._handle_set_param_response(text)
                    continue
                # Format target: 1854.900,-7.286621,112.796040,1.53,-3.95,7.07,3.18,62.33,98.57,0.00,463.38,2880.63,10.54,11.88
                parts = [p.strip() for p in text.split(',')]
                col_count = len(parts)
                if col_count not in (23, TELEMETRY_COL_COUNT):
                    continue
                try:
                    lat = float(parts[1])
                    lon = float(parts[2])

                    if lat == 0.0 and lon == 0.0:
                        lat = -7.2854032
                        lon = 112.7902512

                    if not (-90.0 <= lat <= 90.0 and -180.0 <= lon <= 180.0):
                        continue

                    self.latest_serial_lat = lat
                    self.latest_serial_lon = lon

                    speed = _telemetry_scale(parts, 3)
                    rud1_sensor = self._apply_rudder_correction_deg(
                        _telemetry_scale(parts, 4),
                        self.rudder1_correction_enabled,
                        self.rudder1_correction_value,
                    )
                    rud2_sensor = self._apply_rudder_correction_deg(
                        _telemetry_scale(parts, 5),
                        self.rudder2_correction_enabled,
                        self.rudder2_correction_value,
                    )
                    heading = _telemetry_scale(parts, 6)
                    heading_setpoint = _telemetry_scale(parts, 7)
                    heading_error = _telemetry_scale(parts, 8)
                    rudder_cmd = self._apply_rudder_correction_deg(
                        _telemetry_scale(parts, 9),
                        self.rudder_cmd_correction_enabled,
                        self.rudder_cmd_correction_value,
                    )
                    track_wp_index = int(parts[10])
                    distance_to_wp = _telemetry_scale(parts, 11, 10.0)
                    accel_x = _telemetry_scale(parts, 12)
                    accel_y = _telemetry_scale(parts, 13)
                    accel_z = _telemetry_scale(parts, 14)
                    gyro_x = _telemetry_scale(parts, 15)
                    gyro_y = _telemetry_scale(parts, 16)
                    gyro_z = _telemetry_scale(parts, 17)
                    rpm1 = _telemetry_rpm(parts, 18)
                    rpm2 = _telemetry_rpm(parts, 19)
                    bat1 = _telemetry_scale(parts, 20)
                    bat2 = _telemetry_scale(parts, 21)
                    mode_auto = int(parts[22])
                    mini_pc_link = int(parts[23]) if col_count >= TELEMETRY_COL_COUNT else 0
                    timestamp = float(parts[0])
                    self.latest_serial_heading = heading
                except Exception:
                    continue
                
                # Increment plot counter
                self.plot_counter += 1
                
                # Update label lat/lon/heading setiap data (real-time, tanpa decimation)
                if hasattr(self, 'live_lat_val'):
                    self.live_lat_val.setText(f"{lat:.6f}")
                    self.live_lon_val.setText(f"{lon:.6f}")
                    self.live_hdg_val.setText(f"{heading:.1f}°")

                # Update peta hanya setiap N data (decimation untuk performa)
                if self.plot_counter >= self.plot_interval:
                    self.map_webview.update_map((lat, lon), heading)

                    # Update live position marker + heading line di tab Map Points
                    self.map_points_webview.update_live_position((lat, lon), heading)

                    self.plot_counter = 0  # Reset counter
                
                # Update indicators tetap setiap data (real-time)
                self.update_indicators(
                    heading, heading_setpoint, heading_error,
                    rudder_cmd, rud1_sensor, rud2_sensor,
                    rpm1, rpm2, bat1, bat2, speed,
                    track_wp_index, distance_to_wp,
                    mode_auto, timestamp, mini_pc_link)
                # Append raw CSV line to log buffer if logging enabled
                if self.log_file is not None:
                    try:
                        self.log_buffer.append(
                            _build_telemetry_log_line(
                                timestamp, lat, lon, speed,
                                rud1_sensor, rud2_sensor,
                                heading, heading_setpoint, heading_error, rudder_cmd,
                                track_wp_index, distance_to_wp,
                                accel_x, accel_y, accel_z,
                                gyro_x, gyro_y, gyro_z,
                                rpm1, rpm2, bat1, bat2, mode_auto, mini_pc_link,
                            )
                        )
                    except Exception:
                        pass
        except Exception as e:
            print(f"[SERIAL] Read error: {e}")

    def toggle_logging(self, checked: bool):
        """
        Toggle logging CSV data ke file.
        
        Args:
            checked: True untuk start logging, False untuk stop logging
            
        Method ini:
        - Meminta user memilih file CSV untuk logging
        - Menulis header CSV ke file
        - Start logging timer untuk flush buffer
        - Disable Connect/Disconnect button saat logging aktif
        """
        # Only allow logging when connected
        if checked:
            if not self.is_connected():
                # guard: should not happen since button disabled, but double-check
                self.log_btn.setChecked(False)
                return
            # choose file path
            path, _ = QFileDialog.getSaveFileName(self, "Save log CSV", "", "CSV Files (*.csv)")
            if not path:
                # cancel
                self.log_btn.setChecked(False)
                return
            try:
                self.log_file_path = path
                self.log_file = open(self.log_file_path, 'w', buffering=1, encoding='utf-8')
                self.log_file.write(TELEMETRY_LOG_HEADER)
                self.log_buffer.clear()
                self.log_timer.start()
                self.log_btn.setText("Stop Log")
                print(f"[LOG] Start logging to {self.log_file_path}")
                # Disable Connect/Disconnect while logging
                if self.connect_btn:
                    self.connect_btn.setEnabled(False)
            except Exception as e:
                QMessageBox.critical(self, "Log Error", f"Gagal membuka file:\n{e}")
                self.log_btn.setChecked(False)
                self.log_file = None
                self.log_file_path = None
        else:
            self.stop_logging()

    def stop_logging(self):
        """
        Stop logging CSV data.
        
        Method ini:
        - Stop logging timer
        - Flush log buffer ke file
        - Close log file
        - Reset log file object
        - Update button text
        - Re-enable Connect/Disconnect button
        """
        try:
            self.log_timer.stop()
            self.flush_log_buffer()
            if self.log_file:
                self.log_file.close()
        except Exception:
            pass
        finally:
            self.log_file = None
            self.log_file_path = None
            if self.log_btn:
                self.log_btn.setText("Start Log")
            print("[LOG] Stop logging")
            # Re-enable Connect/Disconnect after logging stops
            if self.connect_btn:
                self.connect_btn.setEnabled(True)

    def flush_log_buffer(self):
        """
        Flush log buffer ke file CSV.
        
        Method ini:
        - Menulis semua data dalam buffer ke file
        - Clear buffer setelah menulis
        - Mengurangi I/O overhead dengan batch writing
        """
        if not self.log_file or not self.log_buffer:
            return
        try:
            # write and clear in batch to reduce IO overhead
            self.log_file.writelines(self.log_buffer)
            self.log_buffer.clear()
        except Exception:
            pass
    
    def load_analyze_csv(self):
        """Load CSV rekaman (display v23, raw v23, atau legacy) ke tab Analyze."""
        path, _ = QFileDialog.getOpenFileName(self, "Load Recorded CSV", "", "CSV Files (*.csv)")
        if not path:
            return
        try:
            with open(path, "r", encoding="utf-8") as csv_file:
                csv_text = csv_file.read()
        except Exception as e:
            QMessageBox.critical(self, "Load CSV Error", f"Gagal membaca file:\n{e}")
            return

        try:
            reader = csv.DictReader(io.StringIO(csv_text))
            if not reader.fieldnames:
                raise ValueError("Header CSV tidak ditemukan")

            self.clear_analyze_plots()
            fmt = _detect_analyze_csv_format(set(reader.fieldnames or []))
            row_count = 0
            loaded = 0

            for row in reader:
                row_count += 1
                parsed = _parse_analyze_csv_row(row, fmt)
                if parsed is None:
                    continue

                lat = parsed["lat"]
                lon = parsed["lon"]
                if lat == 0.0 and lon == 0.0:
                    lat = self.base_lat
                    lon = self.base_lon
                if not (-90.0 <= lat <= 90.0 and -180.0 <= lon <= 180.0):
                    continue

                ts = parsed["timestamp"]
                self.analyze_time_data.append(ts)
                self.analyze_lat_data.append(lat)
                self.analyze_lon_data.append(lon)
                self.analyze_speed_data.append(parsed["speed"])
                self.analyze_rud1_sensor_data.append(parsed["rud1_sensor"])
                self.analyze_rud2_sensor_data.append(parsed["rud2_sensor"])
                self.analyze_yaw_data.append(parsed["yaw"])
                self.analyze_heading_sp_data.append(parsed["heading_setpoint"])
                self.analyze_heading_error_data.append(parsed["heading_error"])
                self.analyze_rudder_cmd_data.append(parsed["rudder_cmd"])
                self.analyze_track_wp_data.append(parsed["track_wp_index"])
                self.analyze_dist_wp_data.append(parsed["distance_to_wp"])
                self.analyze_rpm1_data.append(parsed["rpm1"])
                self.analyze_rpm2_data.append(parsed["rpm2"])
                self.analyze_bat1_data.append(parsed["bat1"])
                self.analyze_bat2_data.append(parsed["bat2"])
                self.analyze_mode_auto_data.append(parsed["mode_auto"])
                self.analyze_map_coords.append((lat, lon))
                self.analyze_heading_values.append(parsed["yaw"])
                loaded += 1

            if loaded == 0:
                print("[ANALYZE] CSV tidak memiliki baris data siap pakai.")
                return

            self.analyze_yaw_curve.setData(self.analyze_time_data, self.analyze_yaw_data)
            self.analyze_heading_sp_curve.setData(self.analyze_time_data, self.analyze_heading_sp_data)
            self.analyze_rudder1_cmd_curve.setData(self.analyze_time_data, self.analyze_rudder_cmd_data)
            self.analyze_rudder1_sensor_curve.setData(self.analyze_time_data, self.analyze_rud1_sensor_data)
            self.analyze_rudder2_cmd_curve.setData(self.analyze_time_data, self.analyze_rudder_cmd_data)
            self.analyze_rudder2_sensor_curve.setData(self.analyze_time_data, self.analyze_rud2_sensor_data)

            if hasattr(self, "analyze_time_slider"):
                scale = getattr(self, "analyze_time_slider_scale", 1)
                slider_min = int(self.analyze_time_data[0] * scale)
                slider_max = int(self.analyze_time_data[-1] * scale)
                if slider_min == slider_max:
                    slider_max = slider_min + 1
                self.analyze_time_slider.blockSignals(True)
                self.analyze_time_slider.setRange(slider_min, slider_max)
                self.analyze_time_slider.setValue(slider_min)
                self.analyze_time_slider.blockSignals(False)
                self._update_analyze_slider_display(slider_min)
                first_timestamp = self.analyze_time_data[0]
                self._update_analyze_at_timestamp(first_timestamp)

            if self.analyze_map_coords:
                first_coord = self.analyze_map_coords[0]
                first_heading = self.analyze_heading_values[0]
                self.analyze_map_webview.trail_coords = [first_coord]
                self.analyze_map_webview.marker_count = 0
                self.analyze_map_webview.add_initial_marker(first_coord, first_heading)
                if first_heading is not None:
                    self.analyze_map_webview.add_heading_line_segment(first_coord, first_heading)

                last_coord = first_coord
                last_heading = first_heading
                for coord, heading in zip(self.analyze_map_coords[1:], self.analyze_heading_values[1:]):
                    self.analyze_map_webview.add_marker_js(coord, heading)
                    if heading is not None:
                        self.analyze_map_webview.add_heading_line_segment(coord, heading)
                    last_coord = coord
                    last_heading = heading

                self._hide_analyze_markers()
                if not self.analyze_show_blue_line_cb.isChecked():
                    self._hide_analyze_blue_line()
                if not self.analyze_show_red_line_cb.isChecked():
                    self._hide_analyze_red_line()

                self.analyze_map_webview.folium_map.location = last_coord
                map_name = self.analyze_map_webview.folium_map.get_name()
                self.analyze_map_webview.page().runJavaScript(f'{map_name}.setView({list(last_coord)})')

            print(f"[ANALYZE] Loaded {loaded}/{row_count} rows (format={fmt}) into graphs and map.")
        except Exception as e:
            QMessageBox.critical(self, "Load CSV Error", f"Gagal mem-parsing file:\n{e}")
    
    def toggle_analyze_map_blue_line(self, state: int):
        """Toggle visibility blue line di peta Analyze"""
        if hasattr(self, 'analyze_map_webview') and self.analyze_map_webview:
            if state == 2:  # Checked
                self._show_analyze_blue_line()
            else:  # Unchecked
                self._hide_analyze_blue_line()
    
    def toggle_analyze_map_red_line(self, state: int):
        """Toggle visibility red line heading di peta Analyze"""
        if hasattr(self, 'analyze_map_webview') and self.analyze_map_webview:
            if state == 2:  # Checked
                self._show_analyze_red_line()
            else:  # Unchecked
                self._hide_analyze_red_line()
    
    def _hide_analyze_markers(self):
        """Hide semua trail marker di peta Analyze (pointer tetap terlihat)"""
        map_name = self.analyze_map_webview.folium_map.get_name()
        js_code = f"""
        if (window.trailMarkers && typeof window.trailMarkers.eachLayer === 'function') {{
            window.trailMarkers.eachLayer(function(layer) {{
                if (layer.setOpacity) {{
                    layer.setOpacity(0);
                }}
            }});
        }}
        """
        self.analyze_map_webview.page().runJavaScript(js_code)
    
    def _hide_analyze_blue_line(self):
        """Hide blue line di peta Analyze"""
        map_name = self.analyze_map_webview.folium_map.get_name()
        js_code = f"""
        if (window.trailLine && typeof window.trailLine.setStyle === 'function') {{
            window.trailLine.setStyle({{opacity: 0}});
        }}
        """
        self.analyze_map_webview.page().runJavaScript(js_code)
    
    def _show_analyze_blue_line(self):
        """Show blue line di peta Analyze"""
        map_name = self.analyze_map_webview.folium_map.get_name()
        js_code = f"""
        if (window.trailLine && typeof window.trailLine.setStyle === 'function') {{
            window.trailLine.setStyle({{opacity: 0.8}});
            window.trailLine.bringToFront();
        }}
        """
        self.analyze_map_webview.page().runJavaScript(js_code)
    
    def _hide_analyze_red_line(self):
        """Hide red line heading di peta Analyze"""
        map_name = self.analyze_map_webview.folium_map.get_name()
        js_code = f"""
        if (window.headingLine && typeof window.headingLine.setStyle === 'function') {{
            window.headingLine.setStyle({{opacity: 0}});
        }}
        if (window.headingLineLayers && window.headingLineLayers.length) {{
            window.headingLineLayers.forEach(function(line) {{
                if (line.setStyle) {{
                    line.setStyle({{opacity: 0}});
                }}
            }});
        }}
        """
        self.analyze_map_webview.page().runJavaScript(js_code)
    
    def _show_analyze_red_line(self):
        """Show red line heading di peta Analyze (hanya yang terakhir)"""
        # Red line heading hanya menampilkan yang terakhir
        if hasattr(self, 'analyze_map_coords') and self.analyze_map_coords and hasattr(self, 'analyze_heading_values') and self.analyze_heading_values:
            js_code = f"""
            if (window.headingLine && typeof window.headingLine.setStyle === 'function') {{
                window.headingLine.setStyle({{opacity: 0.9}});
            }}
            if (window.headingLineLayers && window.headingLineLayers.length) {{
                window.headingLineLayers.forEach(function(line) {{
                    if (line.setStyle) {{
                        line.setStyle({{opacity: 0.9}});
                    }}
                }});
            }}
            """
            self.analyze_map_webview.page().runJavaScript(js_code)

    def closeEvent(self, event):
        self.disconnect_serial()
        super().closeEvent(event)


if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = MainWindow()
    # Start in maximized state to fit user screen
    window.showMaximized()
    sys.exit(app.exec())


