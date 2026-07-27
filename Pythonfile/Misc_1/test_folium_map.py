"""
Test Folium Map - PySide6 GPS Map Testing
Menguji Folium dengan PySide6 untuk menampilkan maps di window desktop
"""

import folium
import json
import time
import webbrowser
import os
from datetime import datetime
import sys
import tempfile

try:
    from PySide6.QtWidgets import (QApplication, QMainWindow, QVBoxLayout, 
                                   QWidget, QPushButton, QHBoxLayout, 
                                   QLabel, QTextEdit, QSplitter, QFrame)
    from PySide6.QtWebEngineWidgets import QWebEngineView
    from PySide6.QtCore import Qt, QTimer, Signal, QThread
    from PySide6.QtGui import QFont, QIcon
    WEBENGINE_AVAILABLE = True
    print("✅ PySide6 WebEngine tersedia")
except Exception as e:
    print(f"❌ Error importing PySide6 WebEngine: {e}")
    print(f"❌ Error type: {type(e).__name__}")
    print("⚠️ PySide6 WebEngine tidak tersedia")
    WEBENGINE_AVAILABLE = False

class FoliumMapTester:
    """Class untuk testing Folium map functionality"""
    
    def __init__(self):
        self.map = None
        self.gps_trail = []
        self.max_trail_points = 100
        self.current_position = None
        self.output_file = "apm_gps_map.html"
        self.temp_html_file = None
        
    def create_initial_map(self):
        """Create initial map with default position"""
        print("🗺️ Creating initial Folium map...")
        
        # Default center (Surabaya, Indonesia)
        default_lat, default_lon = -7.281104, 112.798512
        
        # Create Folium map
        self.map = folium.Map(
            location=[default_lat, default_lon],
            zoom_start=15,
            tiles='OpenStreetMap'
        )
        
        # Add initial marker
        folium.Marker(
            [default_lat, default_lon],
            popup="APM Starting Position",
            tooltip="Click for details",
            icon=folium.Icon(color='red', icon='ship', prefix='fa')
        ).add_to(self.map)
        
        # Add map title
        folium.Marker(
            [default_lat + 0.001, default_lon],
            popup="🚢 APM GPS Map",
            icon=folium.DivIcon(
                html="<div style='font-size: 16px; font-weight: bold; color: #2c3e50;'>🚢 APM GPS Map</div>",
                icon_size=(120, 20),
                icon_anchor=(60, 10)
            )
        ).add_to(self.map)
        
        print(f"✅ Initial map created at {default_lat}, {default_lon}")
        
    def add_gps_position(self, lat, lon, alt=None, satellites=None, timestamp=None):
        """Add GPS position to map"""
        if lat is None or lon is None or lat == 0 or lon == 0:
            return False
            
        try:
            print(f"📍 Adding GPS position: {lat:.6f}, {lon:.6f}")
            
            # Create popup text
            popup_text = f"""
            <b>🚢 APM Position</b><br>
            <b>Time:</b> {timestamp or datetime.now().strftime('%H:%M:%S')}<br>
            <b>Latitude:</b> {lat:.6f}°<br>
            <b>Longitude:</b> {lon:.6f}°<br>
            <b>Altitude:</b> {alt:.1f}m<br>
            <b>Satellites:</b> {satellites}<br>
            <b>Trail Points:</b> {len(self.gps_trail) + 1}
            """
            
            # Add current position marker
            folium.Marker(
                [lat, lon],
                popup=popup_text,
                tooltip="Current Position",
                icon=folium.Icon(color='red', icon='ship', prefix='fa')
            ).add_to(self.map)
            
            # Add to trail
            self.gps_trail.append((lat, lon, alt, timestamp))
            if len(self.gps_trail) > self.max_trail_points:
                self.gps_trail.pop(0)
            
            # Add trail line if we have history
            if len(self.gps_trail) > 1:
                trail_coords = [[point[0], point[1]] for point in self.gps_trail]
                
                # Create polyline for trail
                folium.PolyLine(
                    trail_coords,
                    color='blue',
                    weight=3,
                    opacity=0.7,
                    popup=f"GPS Trail ({len(trail_coords)} points)"
                ).add_to(self.map)
                
                # Add start marker
                folium.Marker(
                    trail_coords[0],
                    popup="Trail Start",
                    tooltip="GPS Trail Start",
                    icon=folium.Icon(color='green', icon='play', prefix='fa')
                ).add_to(self.map)
            
            # Update current position
            self.current_position = (lat, lon, alt)
            
            return True
            
        except Exception as e:
            print(f"❌ Error adding GPS position: {e}")
            return False
    
    def save_map(self, filename=None):
        """Save map to HTML file"""
        if self.map is None:
            print("❌ No map to save!")
            return False
            
        try:
            output_file = filename or self.output_file
            self.map.save(output_file)
            print(f"✅ Map saved to: {output_file}")
            return True
            
        except Exception as e:
            print(f"❌ Error saving map: {e}")
            return False
    
    def save_map_to_temp(self):
        """Save map to temporary HTML file for PySide6"""
        if self.map is None:
            return None
            
        try:
            # Create temporary file
            temp_file = tempfile.NamedTemporaryFile(mode='w', suffix='.html', delete=False)
            self.map.save(temp_file.name)
            temp_file.close()
            
            # Modify HTML for QWebEngineView compatibility
            self._fix_html_for_webengine(temp_file.name)
            
            self.temp_html_file = temp_file.name
            print(f"✅ Map saved to temporary file: {temp_file.name}")
            return temp_file.name
            
        except Exception as e:
            print(f"❌ Error saving to temp file: {e}")
            return None
    
    def _fix_html_for_webengine(self, html_file):
        """Fix HTML file for QWebEngineView compatibility"""
        try:
            with open(html_file, 'r', encoding='utf-8') as f:
                html_content = f.read()
            
            # Replace CDN links with embedded resources or fallback
            fixed_html = self._create_webengine_compatible_html()
            
            with open(html_file, 'w', encoding='utf-8') as f:
                f.write(fixed_html)
                
            print("🔧 HTML fixed for QWebEngineView")
            
        except Exception as e:
            print(f"⚠️ Could not fix HTML: {e}")
    
    def _create_webengine_compatible_html(self):
        """Create HTML that works with QWebEngineView"""
        # Get trail coordinates
        trail_coords = []
        if self.gps_trail:
            trail_coords = [[point[0], point[1]] for point in self.gps_trail]
        
        # Get current position
        current_lat, current_lon = -7.281104, 112.798512
        if self.current_position:
            current_lat, current_lon = self.current_position[0], self.current_position[1]
        
        html_content = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <title>APM GPS Map</title>
            <meta charset="utf-8" />
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            
            <!-- Use HTTPS CDN with fallback -->
            <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"
                  integrity="sha256-p4NxAoJBhIIN+hmNHrzRCf9tD/miZyoHS5obTRR9BMY="
                  crossorigin=""/>
            <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"
                    integrity="sha256-20nQCchB9co0qIjJZRGuk2/Z9VM+kNiyxNV1lvTlZBo="
                    crossorigin=""></script>
                    
            <style>
                body {{ margin: 0; padding: 0; }}
                #map {{ height: 100vh; width: 100%; }}
                .loading {{ 
                    position: absolute; 
                    top: 50%; 
                    left: 50%; 
                    transform: translate(-50%, -50%);
                    font-family: Arial, sans-serif;
                    font-size: 18px;
                    z-index: 1000;
                }}
            </style>
        </head>
        <body>
            <div id="loading" class="loading">🗺️ Loading Map...</div>
            <div id="map"></div>
            
            <script>
                // Wait for page to load completely
                window.addEventListener('load', function() {{
                    setTimeout(initializeMap, 1000); // Delay to ensure resources are loaded
                }});
                
                function initializeMap() {{
                    try {{
                        // Check if Leaflet is available
                        if (typeof L === 'undefined') {{
                            document.getElementById('loading').innerHTML = '❌ Map library not available<br>Please check internet connection';
                            return;
                        }}
                        
                        // Hide loading
                        document.getElementById('loading').style.display = 'none';
                        
                        // Initialize map
                        var map = L.map('map').setView([{current_lat}, {current_lon}], 15);
                        
                        // Add tile layer
                        L.tileLayer('https://{{s}}.tile.openstreetmap.org/{{z}}/{{x}}/{{y}}.png', {{
                            attribution: '© OpenStreetMap contributors',
                            maxZoom: 19
                        }}).addTo(map);
                        
                        // Add current position marker
                        var currentMarker = L.marker([{current_lat}, {current_lon}])
                            .addTo(map)
                            .bindPopup('<b>🚢 Current Position</b><br>Lat: {current_lat:.6f}<br>Lon: {current_lon:.6f}')
                            .openPopup();
                        
                        // Add trail if exists
                        var trailCoords = {trail_coords};
                        if (trailCoords.length > 1) {{
                            var polyline = L.polyline(trailCoords, {{
                                color: 'blue',
                                weight: 3,
                                opacity: 0.7
                            }}).addTo(map);
                            
                            // Add trail markers
                            for (var i = 0; i < trailCoords.length; i++) {{
                                var coord = trailCoords[i];
                                var marker = L.marker(coord).addTo(map);
                                
                                if (i === 0) {{
                                    marker.bindPopup('🟢 Trail Start');
                                }} else if (i === trailCoords.length - 1) {{
                                    marker.bindPopup('🔴 Trail End');
                                }} else {{
                                    marker.bindPopup('📍 Point ' + (i + 1));
                                }}
                            }}
                            
                            // Fit map to show all points
                            map.fitBounds(polyline.getBounds());
                        }}
                        
                        console.log('✅ Map initialized successfully');
                        
                    }} catch (error) {{
                        console.error('❌ Map initialization error:', error);
                        document.getElementById('loading').innerHTML = '❌ Map initialization failed<br>' + error.message;
                    }}
                }}
                
                // Fallback if resources don't load
                setTimeout(function() {{
                    if (typeof L === 'undefined') {{
                        document.getElementById('loading').innerHTML = 
                            '⚠️ Interactive map unavailable<br>' +
                            '<div style="margin-top: 20px; font-size: 14px;">' +
                            'Current Position:<br>' +
                            'Latitude: {current_lat:.6f}°<br>' +
                            'Longitude: {current_lon:.6f}°<br>' +
                            'Trail Points: {len(self.gps_trail)}</div>';
                    }}
                }}, 5000);
            </script>
        </body>
        </html>
        """
        
        return html_content
    
    def open_map_in_browser(self):
        """Open map in default web browser"""
        if os.path.exists(self.output_file):
            try:
                file_url = f"file:///{os.path.abspath(self.output_file).replace(os.sep, '/')}"
                webbrowser.open(file_url)
                print(f"🌐 Map opened in browser: {file_url}")
                return True
            except Exception as e:
                print(f"❌ Error opening browser: {e}")
                return False
        else:
            print("❌ Map file not found!")
            return False
    
    def add_test_positions(self):
        """Add some test GPS positions to simulate movement"""
        print("🧪 Adding test GPS positions...")
        
        # Test positions around Surabaya
        test_positions = [
            (-7.281104, 112.798512, 26.0, 8, "14:30:00"),  # Starting position
            (-7.281200, 112.798600, 26.5, 9, "14:30:10"),  # Move north-east
            (-7.281300, 112.798700, 27.0, 10, "14:30:20"), # Continue movement
            (-7.281400, 112.798800, 27.5, 11, "14:30:30"), # More movement
            (-7.281500, 112.798900, 28.0, 12, "14:30:40"), # Final position
        ]
        
        for lat, lon, alt, sat, timestamp in test_positions:
            self.add_gps_position(lat, lon, alt, sat, timestamp)
            time.sleep(0.5)  # Small delay for effect
        
        print(f"✅ Added {len(test_positions)} test positions")
    
    def show_statistics(self):
        """Show map statistics"""
        print("\n📊 Map Statistics:")
        print(f"   Trail Points: {len(self.gps_trail)}")
        print(f"   Current Position: {self.current_position}")
        print(f"   Output File: {self.output_file}")
        
        if self.gps_trail:
            print(f"   Trail Start: {self.gps_trail[0][0]:.6f}, {self.gps_trail[0][1]:.6f}")
            print(f"   Trail End: {self.gps_trail[-1][0]:.6f}, {self.gps_trail[-1][1]:.6f}")

class PySide6MapWindow(QMainWindow):
    """PySide6 Window untuk menampilkan Folium map"""
    
    def __init__(self, folium_tester):
        super().__init__()
        self.folium_tester = folium_tester
        self.web_view = None
        self.setup_ui()
        
    def setup_ui(self):
        """Setup user interface"""
        self.setWindowTitle("🚢 APM GPS Map - PySide6 Test")
        self.setGeometry(100, 100, 1200, 800)
        
        # Central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Main layout
        main_layout = QVBoxLayout(central_widget)
        
        # Header panel
        header_layout = QHBoxLayout()
        
        # Title
        title_label = QLabel("🚢 APM GPS Map Testing")
        title_label.setFont(QFont("Arial", 16, QFont.Bold))
        header_layout.addWidget(title_label)
        
        header_layout.addStretch()
        
        # Control buttons
        self.add_point_btn = QPushButton("📍 Add Test Point")
        self.add_point_btn.clicked.connect(self.add_test_point)
        header_layout.addWidget(self.add_point_btn)
        
        self.clear_btn = QPushButton("🗑️ Clear Trail")
        self.clear_btn.clicked.connect(self.clear_trail)
        header_layout.addWidget(self.clear_btn)
        
        self.refresh_btn = QPushButton("🔄 Refresh Map")
        self.refresh_btn.clicked.connect(self.refresh_map)
        header_layout.addWidget(self.refresh_btn)
        
        self.browser_btn = QPushButton("🌐 Buka di Browser")
        self.browser_btn.clicked.connect(self.open_in_browser)
        header_layout.addWidget(self.browser_btn)
        
        main_layout.addLayout(header_layout)
        
        # Splitter for map and info
        splitter = QSplitter(Qt.Horizontal)
        
        # Map panel
        if WEBENGINE_AVAILABLE:
            self.web_view = QWebEngineView()
            splitter.addWidget(self.web_view)
            print("✅ WebEngine view created")
        else:
            # Fallback - simple info panel
            fallback_widget = QWidget()
            fallback_layout = QVBoxLayout(fallback_widget)
            
            fallback_label = QLabel("⚠️ WebEngine tidak tersedia\nBuka file HTML di browser")
            fallback_label.setAlignment(Qt.AlignCenter)
            fallback_label.setFont(QFont("Arial", 14))
            fallback_layout.addWidget(fallback_label)
            
            open_browser_btn = QPushButton("🌐 Buka di Browser")
            open_browser_btn.clicked.connect(self.open_in_browser)
            fallback_layout.addWidget(open_browser_btn)
            
            splitter.addWidget(fallback_widget)
        
        # Info panel
        info_widget = QWidget()
        info_layout = QVBoxLayout(info_widget)
        
        # Statistics
        stats_label = QLabel("📊 Map Statistics")
        stats_label.setFont(QFont("Arial", 12, QFont.Bold))
        info_layout.addWidget(stats_label)
        
        self.stats_text = QTextEdit()
        self.stats_text.setMaximumHeight(200)
        self.stats_text.setReadOnly(True)
        info_layout.addWidget(self.stats_text)
        
        # GPS Trail
        trail_label = QLabel("📍 GPS Trail Points")
        trail_label.setFont(QFont("Arial", 12, QFont.Bold))
        info_layout.addWidget(trail_label)
        
        self.trail_text = QTextEdit()
        self.trail_text.setReadOnly(True)
        info_layout.addWidget(self.trail_text)
        
        splitter.addWidget(info_widget)
        
        # Set splitter proportions (map takes more space)
        splitter.setSizes([800, 400])
        
        main_layout.addWidget(splitter)
        
        # Initial map load
        self.load_initial_map()
        
        # Map load timeout
        self.map_timeout = QTimer()
        self.map_timeout.timeout.connect(self.check_map_load)
        self.map_timeout.setSingleShot(True)
        self.map_timeout.start(5000)  # 5 seconds timeout
        
        # Update timer
        self.update_timer = QTimer()
        self.update_timer.timeout.connect(self.update_info)
        self.update_timer.start(1000)  # Update every second
    
    def load_initial_map(self):
        """Load initial map"""
        if self.web_view and self.folium_tester.temp_html_file:
            try:
                file_url = f"file:///{self.folium_tester.temp_html_file.replace(os.sep, '/')}"
                self.web_view.load(file_url)
                print(f"✅ Map loaded in WebView: {file_url}")
                
                # Add error handling for JavaScript errors
                self.web_view.page().setJavaScriptConsoleMessageLevel(1)  # Show errors
                
            except Exception as e:
                print(f"❌ Error loading map: {e}")
                self.show_map_error()
    
    def show_map_error(self):
        """Show error message when map fails to load"""
        error_html = """
        <!DOCTYPE html>
        <html>
        <head>
            <title>Map Error</title>
            <style>
                body { 
                    font-family: Arial, sans-serif; 
                    text-align: center; 
                    padding: 50px;
                    background-color: #f0f0f0;
                }
                .error-box {
                    background-color: #fff;
                    border: 2px solid #ff6b6b;
                    border-radius: 10px;
                    padding: 30px;
                    max-width: 500px;
                    margin: 0 auto;
                }
                .error-icon { font-size: 48px; color: #ff6b6b; }
                .error-title { color: #333; font-size: 24px; margin: 20px 0; }
                .error-message { color: #666; font-size: 16px; margin: 20px 0; }
                .solution { background-color: #e8f4f8; padding: 15px; border-radius: 5px; margin: 20px 0; }
            </style>
        </head>
        <body>
            <div class="error-box">
                <div class="error-icon">⚠️</div>
                <div class="error-title">Map Loading Error</div>
                <div class="error-message">
                    Tidak dapat memuat peta interaktif.<br>
                    Kemungkinan karena masalah dengan Leaflet library atau CDN.
                </div>
                <div class="solution">
                    <strong>Solusi:</strong><br>
                    1. Gunakan tombol "🌐 Buka di Browser" untuk melihat peta<br>
                    2. Atau periksa koneksi internet untuk CDN resources
                </div>
                <div style="margin-top: 20px;">
                    <button onclick="location.reload()" style="padding: 10px 20px; font-size: 16px;">
                        🔄 Coba Lagi
                    </button>
                </div>
            </div>
        </body>
        </html>
        """
        
        if self.web_view:
            self.web_view.setHtml(error_html)
    
    def check_map_load(self):
        """Check if map loaded successfully"""
        if self.web_view:
            try:
                # Try to execute JavaScript to check if Leaflet is loaded
                self.web_view.page().runJavaScript(
                    "typeof L !== 'undefined' ? 'loaded' : 'error'",
                    self.on_map_check_result
                )
            except Exception as e:
                print(f"⚠️ Map check failed: {e}")
                self.show_map_error()
    
    def on_map_check_result(self, result):
        """Handle result of map load check"""
        if result == 'error':
            print("⚠️ Leaflet library not loaded, showing error page")
            self.show_map_error()
        else:
            print("✅ Map loaded successfully")
    
    def add_test_point(self):
        """Add a random test GPS point"""
        import random
        
        # Generate random position around current location
        base_lat, base_lon = -7.281104, 112.798512
        lat_offset = random.uniform(-0.001, 0.001)
        lon_offset = random.uniform(-0.001, 0.001)
        
        new_lat = base_lat + lat_offset
        new_lon = base_lon + lon_offset
        new_alt = random.uniform(25.0, 30.0)
        new_sat = random.randint(8, 12)
        
        # Add to map
        self.folium_tester.add_gps_position(
            new_lat, new_lon, new_alt, new_sat,
            datetime.now().strftime('%H:%M:%S')
        )
        
        # Save updated map
        self.folium_tester.save_map_to_temp()
        
        # Refresh web view
        if self.web_view:
            self.load_initial_map()
        
        print(f"📍 Added test point: {new_lat:.6f}, {new_lon:.6f}")
    
    def clear_trail(self):
        """Clear GPS trail"""
        self.folium_tester.gps_trail = []
        self.folium_tester.current_position = None
        
        # Recreate map
        self.folium_tester.create_initial_map()
        self.folium_tester.save_map_to_temp()
        
        if self.web_view:
            self.load_initial_map()
        
        print("🗑️ GPS trail cleared")
    
    def refresh_map(self):
        """Refresh map display"""
        self.folium_tester.save_map_to_temp()
        if self.web_view:
            self.load_initial_map()
        print("🔄 Map refreshed")
    
    def open_in_browser(self):
        """Open map in browser"""
        self.folium_tester.save_map()
        self.folium_tester.open_map_in_browser()
    
    def update_info(self):
        """Update information panels"""
        # Update statistics
        stats_text = f"""Trail Points: {len(self.folium_tester.gps_trail)}
Current Position: {self.folium_tester.current_position}
Max Trail Points: {self.folium_tester.max_trail_points}
Output File: {self.folium_tester.output_file}
Temp File: {self.folium_tester.temp_html_file}"""
        
        self.stats_text.setText(stats_text)
        
        # Update trail points
        if self.folium_tester.gps_trail:
            trail_text = "GPS Trail Points:\n\n"
            for i, point in enumerate(self.folium_tester.gps_trail):
                lat, lon, alt, timestamp = point
                trail_text += f"{i+1}. {timestamp} | {lat:.6f}, {lon:.6f} | Alt: {alt:.1f}m\n"
            self.trail_text.setText(trail_text)
        else:
            self.trail_text.setText("No GPS trail points yet.")
    
    def closeEvent(self, event):
        """Cleanup when window is closed"""
        # Stop timer
        if hasattr(self, 'update_timer'):
            self.update_timer.stop()
        
        # Clean up temporary file
        if self.folium_tester.temp_html_file and os.path.exists(self.folium_tester.temp_html_file):
            try:
                os.unlink(self.folium_tester.temp_html_file)
                print(f"🗑️ Cleaned up temp file: {self.folium_tester.temp_html_file}")
            except Exception as e:
                print(f"⚠️ Could not delete temp file: {e}")
        
        event.accept()

def main():
    """Main function untuk testing Folium dengan PySide6"""
    print("🚀 Starting Folium Map Testing dengan PySide6...")
    print("=" * 50)
    
    if not WEBENGINE_AVAILABLE:
        print("❌ PySide6 WebEngine tidak tersedia!")
        print("🔧 Install PySide6 dengan: pip install PySide6")
        return
    
    # Create map tester
    tester = FoliumMapTester()
    
    # Create initial map
    tester.create_initial_map()
    
    # Add test positions
    tester.add_test_positions()
    
    # Save maps (both regular and temp)
    tester.save_map()
    tester.save_map_to_temp()
    
    # Show statistics
    tester.show_statistics()
    
    print("\n" + "=" * 50)
    print("🖥️ Starting PySide6 Window...")
    
    # Create PySide6 application
    app = QApplication(sys.argv)
    
    # Create and show window
    window = PySide6MapWindow(tester)
    window.show()
    
    print("✅ PySide6 Window opened!")
    print("📍 Use buttons to add test points and interact with map")
    print("🌐 Regular HTML file also saved for browser viewing")
    
    # Run application
    sys.exit(app.exec())

def main_browser_only():
    """Alternative main function untuk browser-only testing"""
    print("🚀 Starting Folium Map Testing (Browser Only)...")
    print("=" * 50)
    
    # Create map tester
    tester = FoliumMapTester()
    
    # Create initial map
    tester.create_initial_map()
    
    # Add test positions
    tester.add_test_positions()
    
    # Save map
    tester.save_map()
    
    # Show statistics
    tester.show_statistics()
    
    # Ask user if they want to open in browser
    print("\n" + "=" * 50)
    response = input("🌐 Open map in browser? (y/n): ").lower().strip()
    if response in ['y', 'yes', '']:
        tester.open_map_in_browser()
    
    print("\n✅ Folium testing completed!")
    print(f"📁 Map file saved as: {tester.output_file}")
    print("🔍 You can open this file in any web browser to view the map")

if __name__ == "__main__":
    # Check command line arguments
    if len(sys.argv) > 1 and sys.argv[1] == "--browser":
        main_browser_only()
    else:
        main()
