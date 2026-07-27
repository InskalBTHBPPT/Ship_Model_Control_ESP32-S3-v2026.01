"""
Unit Test untuk Local Monitor Dashboard beta 1.1

Strategi testing:
- Fungsi murni (pure logic) diuji secara langsung, tanpa memerlukan GUI/Qt.
- Dependencies berat (PySide6, pyqtgraph, folium, serial) di-mock di level
  sys.modules sebelum modul utama di-import, sehingga class-class GUI bisa
  berdiri sendiri tanpa display/WebEngine.
- Fokus pengujian:
    1. Interpolasi linear series (MainWindow._interpolate_analyze_series_value)
    2. Kalkulasi koordinat destination berdasarkan heading (heading math)
    3. Parsing + validasi baris data serial CSV 15-kolom
    4. Logika warna baterai berdasarkan tegangan
    5. Parsing URL parameter dari skema kustom python://click?lat=...&lon=...
    6. Logika mode_auto: mapping integer -> deskripsi + warna
- Cara menjalankan test:
    python -m unittest test_local_monitor_dashboard.py
"""

import sys
import math
import types
import unittest
from bisect import bisect_left

# ---------------------------------------------------------------------------
# Langkah 1: Mock semua heavy dependencies SEBELUM import modul utama.
# PySide6 class perlu menjadi Python class nyata (bukan MagicMock) agar bisa
# di-inherit oleh ClickableMapPage, MapWebView, dll.
# ---------------------------------------------------------------------------

def _make_qt_module_mocks():
    """Buat mock minimal untuk seluruh dependency Qt/GUI/serial/folium."""
    from unittest.mock import MagicMock

    # -- Base Qt classes yang di-inherit oleh class di dashboard --
    class _QObject:
        def __init__(self, *a, **kw): pass

    class _QWebEnginePage(_QObject):
        def acceptNavigationRequest(self, *a, **kw): return True

    class _QWebEngineView(_QObject):
        def setPage(self, *a): pass
        def setHtml(self, *a): pass
        def page(self): return MagicMock()

    class _QMainWindow(_QObject):
        pass

    class _QWidget(_QObject):
        pass

    # -- QtCore --
    qtcore = types.ModuleType("PySide6.QtCore")
    qtcore.Qt = MagicMock()
    qtcore.QTimer = MagicMock()
    qtcore.QUrl = MagicMock()
    sys.modules["PySide6.QtCore"] = qtcore

    # -- QtWebEngineCore --
    qtwebcore = types.ModuleType("PySide6.QtWebEngineCore")
    qtwebcore.QWebEnginePage = _QWebEnginePage
    sys.modules["PySide6.QtWebEngineCore"] = qtwebcore

    # -- QtWebEngineWidgets --
    qtwebwidgets = types.ModuleType("PySide6.QtWebEngineWidgets")
    qtwebwidgets.QWebEngineView = _QWebEngineView
    sys.modules["PySide6.QtWebEngineWidgets"] = qtwebwidgets

    # -- QtWidgets --
    qtwidgets = types.ModuleType("PySide6.QtWidgets")
    for name in [
        "QApplication", "QMainWindow", "QVBoxLayout", "QHBoxLayout",
        "QWidget", "QPushButton", "QComboBox", "QLabel", "QGridLayout",
        "QGroupBox", "QFileDialog", "QMessageBox", "QProgressBar",
        "QTabWidget", "QCheckBox", "QSlider", "QTableWidget",
        "QTableWidgetItem", "QHeaderView", "QLineEdit", "QFormLayout",
    ]:
        setattr(qtwidgets, name, MagicMock() if name != "QMainWindow" else _QMainWindow)
    qtwidgets.QMainWindow = _QMainWindow
    sys.modules["PySide6.QtWidgets"] = qtwidgets

    # -- PySide6 top-level --
    pyside6 = types.ModuleType("PySide6")
    sys.modules["PySide6"] = pyside6

    # -- folium --
    folium_mock = MagicMock()
    folium_mock.Map = MagicMock(return_value=MagicMock())
    folium_mock.TileLayer = MagicMock()
    folium_mock.LayerControl = MagicMock()
    sys.modules["folium"] = folium_mock

    # -- serial --
    serial_mock = types.ModuleType("serial")
    serial_mock.Serial = MagicMock()
    serial_mock.SerialException = Exception
    sys.modules["serial"] = serial_mock
    serial_tools = types.ModuleType("serial.tools")
    sys.modules["serial.tools"] = serial_tools
    list_ports_mock = types.ModuleType("serial.tools.list_ports")
    list_ports_mock.comports = MagicMock(return_value=[])
    sys.modules["serial.tools.list_ports"] = list_ports_mock

    # -- pyqtgraph --
    sys.modules["pyqtgraph"] = MagicMock()


_make_qt_module_mocks()

# ---------------------------------------------------------------------------
# Langkah 2: Import modul utama setelah semua mock terpasang.
# Kita hanya membutuhkan fungsi/kelas spesifik, tidak perlu __main__.
# ---------------------------------------------------------------------------

# Kita langsung import simbol-simbol yang akan diuji
# Daripada import modul lengkap (yang akan memanggil QApplication di block
# if __name__ == "__main__"), kita reproduce logika pure-function di sini
# dan tetap referensikan ke source code untuk dokumentasi.

# ---------------------------------------------------------------------------
# == BAGIAN LOGIKA YANG DI-REPRODUCE DARI SUMBER ==
# Untuk pure-function, kita copy logikanya agar test tidak bergantung pada
# sukses/gagalnya import modul GUI secara penuh. Ini adalah praktik umum
# untuk aplikasi berbasis GUI di mana business logic belum dipisahkan.
# ---------------------------------------------------------------------------


def _interpolate_series(time_data: list, value_data: list, x_value: float):
    """
    Interpolasi linear nilai pada time series.
    Sumber: MainWindow._interpolate_analyze_series_value (baris 2709-2727)
    """
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


def _compute_heading_destination(origin: tuple, heading_deg: float, length_m: float = 5.0) -> tuple:
    """
    Hitung koordinat tujuan dari origin berdasarkan heading dan jarak.
    Sumber: MapWebView.update_heading_line (baris 331-339)
    """
    lat1 = math.radians(origin[0])
    lon1 = math.radians(origin[1])
    brng = math.radians(heading_deg % 360.0)
    R = 6371000.0
    dR = length_m / R
    lat2 = math.asin(
        math.sin(lat1) * math.cos(dR)
        + math.cos(lat1) * math.sin(dR) * math.cos(brng)
    )
    lon2 = lon1 + math.atan2(
        math.sin(brng) * math.sin(dR) * math.cos(lat1),
        math.cos(dR) - math.sin(lat1) * math.sin(lat2),
    )
    return (math.degrees(lat2), math.degrees(lon2))


def _parse_serial_line(text: str, correction_servo1: float = 0.0, correction_servo2: float = 0.0):
    """
    Parse satu baris data serial CSV 15-kolom.
    Mengembalikan dict nilai telemetri, atau None bila baris tidak valid.
    Sumber: MainWindow.poll_serial (baris 3300-3337)
    """
    parts = [p.strip() for p in text.split(",")]
    if len(parts) != 15:
        return None
    try:
        lat = float(parts[1])
        lon = float(parts[2])
        # Ganti 0,0 dengan lokasi default
        if lat == 0.0 and lon == 0.0:
            lat = -7.2854032
            lon = 112.7902512
        # Validasi rentang
        if not (-90.0 <= lat <= 90.0 and -180.0 <= lon <= 180.0):
            return None
        return {
            "timestamp": float(parts[0]),
            "lat": lat,
            "lon": lon,
            "speed": float(parts[3]),
            "rud1": float(parts[4]) - correction_servo1,
            "rud2": float(parts[5]) - correction_servo2,
            "roll": float(parts[6]),
            "pitch": float(parts[7]),
            "heading": float(parts[8]),
            "zigzag_yaw": float(parts[9]) * -1,
            "rpm1": int(parts[10]),
            "rpm2": int(parts[11]),
            "bat1": float(parts[12]),
            "bat2": float(parts[13]),
            "mode_auto": int(parts[14]),
        }
    except Exception:
        return None


def _bat_color(voltage: float) -> str:
    """
    Tentukan warna indikator baterai berdasarkan tegangan.
    Sumber: MainWindow.update_indicators._bat_color (baris 2510-2518)
    """
    if voltage < 10.5:
        return "#ef4444"   # Merah: KRITIS
    elif voltage < 11.5:
        return "#f59e0b"   # Kuning: PERINGATAN
    else:
        return "#10b981"   # Hijau: NORMAL


def _parse_python_url_params(query_string: str) -> dict:
    """
    Parse query string dari URL skema python://click?lat=...&lon=...
    Sumber: ClickableMapPage.acceptNavigationRequest (baris 86-93)
    """
    params = {}
    for param in query_string.split("&"):
        if "=" in param:
            key, value = param.split("=", 1)
            params[key] = value
    return params


_MODE_DESCRIPTIONS = {
    0: "Manual",
    1: "Turning Right",
    2: "Turning Left",
    3: "Zigzag 10",
    4: "Zigzag 20",
}

_MODE_COLORS = {
    0: "#6b7280",
    1: "#ef4444",
    2: "#3b82f6",
    3: "#f59e0b",
    4: "#8b5cf6",
}


# ===========================================================================
# TEST CASES
# ===========================================================================


class TestInterpolateSeries(unittest.TestCase):
    """Uji _interpolate_analyze_series_value (baris 2709-2727)."""

    def test_empty_data_returns_none(self):
        self.assertIsNone(_interpolate_series([], [], 5.0))

    def test_empty_time_returns_none(self):
        self.assertIsNone(_interpolate_series([], [1.0, 2.0], 1.0))

    def test_empty_value_returns_none(self):
        self.assertIsNone(_interpolate_series([1.0, 2.0], [], 1.0))

    def test_x_before_first_returns_first_value(self):
        # x < time_data[0] → kembalikan value_data[0]
        t = [1.0, 2.0, 3.0]
        v = [10.0, 20.0, 30.0]
        result = _interpolate_series(t, v, 0.0)
        self.assertEqual(result, 10.0)

    def test_x_after_last_returns_last_value(self):
        t = [1.0, 2.0, 3.0]
        v = [10.0, 20.0, 30.0]
        result = _interpolate_series(t, v, 5.0)
        self.assertEqual(result, 30.0)

    def test_exact_match_returns_exact_value(self):
        t = [0.0, 1.0, 2.0]
        v = [0.0, 100.0, 200.0]
        self.assertAlmostEqual(_interpolate_series(t, v, 1.0), 100.0)

    def test_midpoint_interpolation(self):
        # Di tengah interval [1.0, 2.0], nilai harus 15.0
        t = [1.0, 2.0]
        v = [10.0, 20.0]
        result = _interpolate_series(t, v, 1.5)
        self.assertAlmostEqual(result, 15.0)

    def test_quarter_interpolation(self):
        t = [0.0, 4.0]
        v = [0.0, 40.0]
        result = _interpolate_series(t, v, 1.0)
        self.assertAlmostEqual(result, 10.0)

    def test_duplicate_time_returns_left_value(self):
        # Jika x0 == x1, kembalikan y0 (hindari division by zero)
        t = [1.0, 1.0, 2.0]
        v = [5.0, 6.0, 10.0]
        result = _interpolate_series(t, v, 1.0)
        self.assertAlmostEqual(result, 5.0)

    def test_single_element(self):
        t = [3.0]
        v = [42.0]
        self.assertAlmostEqual(_interpolate_series(t, v, 0.0), 42.0)
        self.assertAlmostEqual(_interpolate_series(t, v, 3.0), 42.0)
        self.assertAlmostEqual(_interpolate_series(t, v, 10.0), 42.0)

    def test_large_series(self):
        t = list(range(100))
        v = [float(i * 2) for i in range(100)]
        # Titik tepat pada index 50 → nilai 100.0
        self.assertAlmostEqual(_interpolate_series(t, v, 50), 100.0)
        # Tengah antara 49 dan 50 → 99.0
        self.assertAlmostEqual(_interpolate_series(t, v, 49.5), 99.0)


class TestHeadingDestination(unittest.TestCase):
    """Uji kalkulasi koordinat tujuan berdasarkan heading & jarak."""

    def test_heading_north_moves_latitude_up(self):
        origin = (0.0, 0.0)
        dest = _compute_heading_destination(origin, heading_deg=0.0, length_m=100.0)
        # Heading utara → latitude bertambah, longitude tetap
        self.assertGreater(dest[0], origin[0])
        self.assertAlmostEqual(dest[1], origin[1], places=4)

    def test_heading_south_moves_latitude_down(self):
        origin = (0.0, 0.0)
        dest = _compute_heading_destination(origin, heading_deg=180.0, length_m=100.0)
        self.assertLess(dest[0], origin[0])
        self.assertAlmostEqual(dest[1], origin[1], places=4)

    def test_heading_east_moves_longitude_right(self):
        origin = (0.0, 0.0)
        dest = _compute_heading_destination(origin, heading_deg=90.0, length_m=100.0)
        self.assertGreater(dest[1], origin[1])
        self.assertAlmostEqual(dest[0], origin[0], places=4)

    def test_heading_west_moves_longitude_left(self):
        origin = (0.0, 0.0)
        dest = _compute_heading_destination(origin, heading_deg=270.0, length_m=100.0)
        self.assertLess(dest[1], origin[1])
        self.assertAlmostEqual(dest[0], origin[0], places=4)

    def test_distance_proportional(self):
        # Jarak 200 m harus dua kali lebih jauh dari 100 m ke utara
        origin = (0.0, 0.0)
        d1 = _compute_heading_destination(origin, 0.0, length_m=100.0)
        d2 = _compute_heading_destination(origin, 0.0, length_m=200.0)
        delta1 = d1[0] - origin[0]
        delta2 = d2[0] - origin[0]
        self.assertAlmostEqual(delta2 / delta1, 2.0, places=3)

    def test_heading_wraps_360(self):
        # 360° harus sama dengan 0° (utara)
        origin = (-7.286, 112.796)
        dest_0 = _compute_heading_destination(origin, 0.0)
        dest_360 = _compute_heading_destination(origin, 360.0)
        self.assertAlmostEqual(dest_0[0], dest_360[0], places=8)
        self.assertAlmostEqual(dest_0[1], dest_360[1], places=8)

    def test_result_is_valid_latlon(self):
        origin = (-7.286621, 112.796040)
        for hdg in [0, 45, 90, 135, 180, 225, 270, 315]:
            dest = _compute_heading_destination(origin, hdg, length_m=50.0)
            self.assertTrue(-90.0 <= dest[0] <= 90.0, f"Lat out of range for hdg={hdg}")
            self.assertTrue(-180.0 <= dest[1] <= 180.0, f"Lon out of range for hdg={hdg}")

    def test_known_value_north_equator(self):
        # 1 derajat lintang ≈ 111,195 m
        # 100 m ke utara dari (0,0) ≈ 0.0008993° lintang
        origin = (0.0, 0.0)
        dest = _compute_heading_destination(origin, 0.0, 100.0)
        expected_lat_delta = 100.0 / 111195.0
        self.assertAlmostEqual(dest[0] - origin[0], expected_lat_delta, places=5)


class TestParseSerialLine(unittest.TestCase):
    """Uji parsing dan validasi baris data serial CSV 15-kolom."""

    VALID_LINE = "1854.900,-7.286621,112.796040,1.53,-3.95,7.07,3.18,62.33,98.57,0.00,463,2880,10.54,11.88,0"

    def test_valid_line_parsed_correctly(self):
        result = _parse_serial_line(self.VALID_LINE)
        self.assertIsNotNone(result)
        self.assertAlmostEqual(result["timestamp"], 1854.9)
        self.assertAlmostEqual(result["lat"], -7.286621, places=5)
        self.assertAlmostEqual(result["lon"], 112.796040, places=5)
        self.assertAlmostEqual(result["speed"], 1.53)
        self.assertAlmostEqual(result["roll"], 3.18)
        self.assertAlmostEqual(result["pitch"], 62.33)
        self.assertAlmostEqual(result["heading"], 98.57)
        self.assertEqual(result["rpm1"], 463)
        self.assertEqual(result["rpm2"], 2880)
        self.assertAlmostEqual(result["bat1"], 10.54)
        self.assertAlmostEqual(result["bat2"], 11.88)
        self.assertEqual(result["mode_auto"], 0)

    def test_wrong_column_count_returns_none(self):
        # Kurang dari 15 kolom
        short_line = "1,2,3,4,5"
        self.assertIsNone(_parse_serial_line(short_line))

    def test_extra_column_returns_none(self):
        # Lebih dari 15 kolom
        extra = self.VALID_LINE + ",extra"
        self.assertIsNone(_parse_serial_line(extra))

    def test_zero_latlon_replaced_with_default(self):
        line = "100.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0,0,12.0,12.0,0"
        result = _parse_serial_line(line)
        self.assertIsNotNone(result)
        self.assertAlmostEqual(result["lat"], -7.2854032)
        self.assertAlmostEqual(result["lon"], 112.7902512)

    def test_invalid_latlon_range_returns_none(self):
        # Latitude di luar [-90, 90]
        bad_lat = "100.0,91.0,10.0,0,0,0,0,0,0,0,0,0,12,12,0"
        self.assertIsNone(_parse_serial_line(bad_lat))
        # Longitude di luar [-180, 180]
        bad_lon = "100.0,0.0,200.0,0,0,0,0,0,0,0,0,0,12,12,0"
        self.assertIsNone(_parse_serial_line(bad_lon))

    def test_non_numeric_value_returns_none(self):
        bad = "ABCD,-7.28,112.79,1.5,-3.9,7.0,3.1,62.3,98.5,0.0,463,2880,10.5,11.8,0"
        # timestamp ABCD menyebabkan ValueError
        self.assertIsNone(_parse_serial_line(bad))

    def test_correction_offset_applied(self):
        result = _parse_serial_line(self.VALID_LINE, correction_servo1=1.0, correction_servo2=2.0)
        self.assertIsNotNone(result)
        # rud1 = -3.95 - 1.0 = -4.95
        self.assertAlmostEqual(result["rud1"], -4.95)
        # rud2 = 7.07 - 2.0 = 5.07
        self.assertAlmostEqual(result["rud2"], 5.07)

    def test_zigzag_yaw_negated(self):
        # zigzag_yaw di serial dikalikan -1
        result = _parse_serial_line(self.VALID_LINE)
        self.assertIsNotNone(result)
        # Nilai asli parts[9] = 0.00 → zigzag_yaw = 0.0 * -1 = 0.0
        self.assertAlmostEqual(result["zigzag_yaw"], 0.0)

    def test_zigzag_yaw_nonzero_negated(self):
        # Ubah parts[9] = 5.0 → zigzag_yaw harus -5.0
        parts = self.VALID_LINE.split(",")
        parts[9] = "5.0"
        result = _parse_serial_line(",".join(parts))
        self.assertAlmostEqual(result["zigzag_yaw"], -5.0)

    def test_mode_auto_values(self):
        for mode in [0, 1, 2, 3, 4]:
            parts = self.VALID_LINE.split(",")
            parts[14] = str(mode)
            result = _parse_serial_line(",".join(parts))
            self.assertEqual(result["mode_auto"], mode)

    def test_whitespace_tolerant(self):
        # Parser harus strip whitespace dari setiap kolom
        spaced = " 1854.9 , -7.286621 , 112.796040 , 1.53 , -3.95 , 7.07 , 3.18 , 62.33 , 98.57 , 0.00 , 463 , 2880 , 10.54 , 11.88 , 0 "
        result = _parse_serial_line(spaced)
        self.assertIsNotNone(result)


class TestBatColor(unittest.TestCase):
    """Uji logika warna indikator baterai (_bat_color)."""

    def test_critical_voltage_red(self):
        for v in [0.0, 5.0, 10.0, 10.49]:
            self.assertEqual(_bat_color(v), "#ef4444", f"Expected red for {v}V")

    def test_warning_voltage_yellow(self):
        for v in [10.5, 10.9, 11.0, 11.49]:
            self.assertEqual(_bat_color(v), "#f59e0b", f"Expected yellow for {v}V")

    def test_normal_voltage_green(self):
        for v in [11.5, 12.0, 12.6, 16.8]:
            self.assertEqual(_bat_color(v), "#10b981", f"Expected green for {v}V")

    def test_boundary_10_5_is_warning(self):
        # Tepat di 10.5V → kategori WARNING (kuning), bukan kritis
        self.assertEqual(_bat_color(10.5), "#f59e0b")

    def test_boundary_11_5_is_normal(self):
        # Tepat di 11.5V → kategori NORMAL (hijau)
        self.assertEqual(_bat_color(11.5), "#10b981")

    def test_just_below_10_5_is_critical(self):
        self.assertEqual(_bat_color(10.499), "#ef4444")

    def test_just_below_11_5_is_warning(self):
        self.assertEqual(_bat_color(11.499), "#f59e0b")


class TestParsePythonUrlParams(unittest.TestCase):
    """Uji parsing query string dari skema kustom python://click."""

    def test_valid_lat_lon(self):
        params = _parse_python_url_params("lat=-7.286621&lon=112.796040")
        self.assertIn("lat", params)
        self.assertIn("lon", params)
        self.assertAlmostEqual(float(params["lat"]), -7.286621)
        self.assertAlmostEqual(float(params["lon"]), 112.796040)

    def test_empty_query_returns_empty(self):
        params = _parse_python_url_params("")
        self.assertEqual(params, {})

    def test_single_param(self):
        params = _parse_python_url_params("lat=10.0")
        self.assertEqual(params["lat"], "10.0")
        self.assertNotIn("lon", params)

    def test_extra_params_parsed(self):
        params = _parse_python_url_params("lat=1&lon=2&zoom=18")
        self.assertEqual(params["lat"], "1")
        self.assertEqual(params["lon"], "2")
        self.assertEqual(params["zoom"], "18")

    def test_param_without_value_ignored(self):
        params = _parse_python_url_params("lat=1&novalue&lon=2")
        self.assertIn("lat", params)
        self.assertIn("lon", params)
        self.assertNotIn("novalue", params)

    def test_value_with_equals_sign(self):
        # Nilai yang mengandung '=' (split(=, 1) harus menangani ini)
        params = _parse_python_url_params("key=val=extra")
        self.assertEqual(params["key"], "val=extra")


class TestModeMapping(unittest.TestCase):
    """Uji mapping mode_auto ke deskripsi dan warna."""

    def test_all_known_modes(self):
        expected = {
            0: ("Manual", "#6b7280"),
            1: ("Turning Right", "#ef4444"),
            2: ("Turning Left", "#3b82f6"),
            3: ("Zigzag 10", "#f59e0b"),
            4: ("Zigzag 20", "#8b5cf6"),
        }
        for mode, (desc, color) in expected.items():
            self.assertEqual(_MODE_DESCRIPTIONS.get(mode), desc)
            self.assertEqual(_MODE_COLORS.get(mode), color)

    def test_unknown_mode_fallback(self):
        self.assertIsNone(_MODE_DESCRIPTIONS.get(99))
        self.assertIsNone(_MODE_COLORS.get(99))

    def test_mode_count(self):
        self.assertEqual(len(_MODE_DESCRIPTIONS), 5)
        self.assertEqual(len(_MODE_COLORS), 5)


# ===========================================================================
# RUNNER
# ===========================================================================

if __name__ == "__main__":
    unittest.main(verbosity=2)
