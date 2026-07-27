import sys
import os
import folium
from PySide6.QtCore import QUrl
from PySide6.QtWebEngineWidgets import QWebEngineView
from PySide6.QtWidgets import QApplication, QMainWindow

# Get current directory
current_dir = os.path.dirname(os.path.abspath(__file__))
map_file = os.path.join(current_dir, "map.html")

print(f"📁 Current directory: {current_dir}")
print(f"🗺️ Map file will be saved to: {map_file}")

# Create and save a Folium map
m = folium.Map(location=[-7.281104, 112.798512], zoom_start=15)  # Surabaya coordinates
folium.Marker(
    [-7.281104, 112.798512],
    popup="🚢 APM Test Location",
    tooltip="Click for details"
).add_to(m)

m.save(map_file)
print(f"✅ Map saved to: {map_file}")

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("🚢 Folium Map in PySide6 - Simple Test")

        self.browser = QWebEngineView()
        
        # Try different loading methods
        print("🔄 Trying Method 1: Load from file URL...")
        
        # Check if file exists
        if os.path.exists(map_file):
            print(f"✅ Loading map from: {map_file}")
            # Use absolute path with proper URL format
            file_url = QUrl.fromLocalFile(map_file)
            print(f"🔗 File URL: {file_url.toString()}")
            self.browser.load(file_url)
            
            # Wait a bit then try alternative method if needed
            from PySide6.QtCore import QTimer
            self.timer = QTimer()
            self.timer.timeout.connect(self.check_and_fallback)
            self.timer.setSingleShot(True)
            self.timer.start(3000)  # 3 seconds
            
        else:
            print(f"❌ Map file not found: {map_file}")
            self.show_error()
        
        self.setCentralWidget(self.browser)
        self.setGeometry(100, 100, 1000, 700)
    
    def check_and_fallback(self):
        """Check if map loaded, if not use fallback method"""
        print("🔄 Checking map load status...")
        
        # Try to execute JavaScript to check if Leaflet loaded
        self.browser.page().runJavaScript(
            "typeof L !== 'undefined' ? 'loaded' : 'error'",
            self.on_check_result
        )
    
    def on_check_result(self, result):
        """Handle check result"""
        if result == 'error':
            print("❌ Leaflet not loaded, trying fallback method...")
            self.use_fallback_method()
        else:
            print("✅ Map loaded successfully with Leaflet")
    
    def use_fallback_method(self):
        """Use setHtml method with simple HTML"""
        print("🔄 Using fallback method: setHtml with simple map...")
        
        simple_html = """
        <!DOCTYPE html>
        <html>
        <head>
            <title>Simple Map</title>
            <style>
                body { 
                    font-family: Arial, sans-serif; 
                    margin: 0; 
                    padding: 20px;
                    background-color: #f0f8ff;
                }
                .map-container {
                    background-color: white;
                    border: 2px solid #4CAF50;
                    border-radius: 10px;
                    padding: 20px;
                    text-align: center;
                    box-shadow: 0 4px 8px rgba(0,0,0,0.1);
                }
                .coordinates {
                    font-size: 18px;
                    color: #2c3e50;
                    margin: 10px 0;
                }
                .icon {
                    font-size: 48px;
                    margin: 20px 0;
                }
                .info {
                    background-color: #e8f4f8;
                    padding: 15px;
                    border-radius: 5px;
                    margin: 20px 0;
                }
            </style>
        </head>
        <body>
            <div class="map-container">
                <div class="icon">🚢</div>
                <h1>APM GPS Location</h1>
                <div class="coordinates">
                    <strong>Latitude:</strong> -7.281104°<br>
                    <strong>Longitude:</strong> 112.798512°<br>
                    <strong>Location:</strong> Surabaya, Indonesia
                </div>
                <div class="info">
                    <strong>Note:</strong> Interactive map unavailable in QWebEngineView<br>
                    This is a simplified coordinate display.
                </div>
                <div style="margin-top: 20px;">
                    <button onclick="alert('Coordinates: -7.281104, 112.798512')" 
                            style="padding: 10px 20px; font-size: 16px; background-color: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer;">
                        📍 Show Coordinates
                    </button>
                </div>
            </div>
        </body>
        </html>
        """
        
        self.browser.setHtml(simple_html)
        print("✅ Fallback simple map displayed")
    
    def show_error(self):
        """Show error message"""
        error_html = """
        <html><body style="font-family: Arial; text-align: center; padding: 50px;">
            <h2>❌ Map File Not Found</h2>
            <p>Could not load map.html</p>
            <p>File path: """ + map_file + """</p>
        </body></html>
        """
        self.browser.setHtml(error_html)

app = QApplication(sys.argv)
window = MainWindow()
window.show()
sys.exit(app.exec())