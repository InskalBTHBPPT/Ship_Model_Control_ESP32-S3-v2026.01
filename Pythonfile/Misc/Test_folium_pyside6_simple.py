import io
import sys
import folium
from PySide6.QtCore import QTimer
from PySide6.QtWebEngineWidgets import QWebEngineView
from PySide6.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget


class MapWebView(QWebEngineView):
    def __init__(self, initial_coordinates: tuple[float, float]):
        super().__init__()
        self.folium_map = folium.Map(
            location=initial_coordinates,
            zoom_start=15,
            zoom_control=True,
            attribution_control=True
        )
        
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
        """Add different tile layers including satellite view"""
        # Esri Satellite
        folium.TileLayer(
            tiles='https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}',
            attr='Esri',
            name='🛰️ Satellite',
            overlay=False,
            control=True
        ).add_to(self.folium_map)
        
        # Google Satellite (alternative)
        folium.TileLayer(
            tiles='https://mt1.google.com/vt/lyrs=s&x={x}&y={y}&z={z}',
            attr='Google',
            name='🌍 Google Satellite',
            overlay=False,
            control=True
        ).add_to(self.folium_map)
        
        # Google Hybrid (Satellite + Labels)
        folium.TileLayer(
            tiles='https://mt1.google.com/vt/lyrs=y&x={x}&y={y}&z={z}',
            attr='Google',
            name='🗺️ Hybrid',
            overlay=False,
            control=True
        ).add_to(self.folium_map)
        
        # CartoDB Dark Matter
        folium.TileLayer(
            tiles='https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png',
            attr='CartoDB',
            name='🌙 Dark Mode',
            overlay=False,
            control=True
        ).add_to(self.folium_map)
        
        # Add layer control
        folium.LayerControl(position='topright').add_to(self.folium_map)
        
        print("🛰️ Added satellite and hybrid tile layers")
    
    def add_initial_marker(self, coords: tuple[float, float]):
        """Add initial marker using JavaScript"""
        self.marker_count += 1
        map_name = self.folium_map.get_name()
        
        js_code = f"""
        // Add initial marker
        var startMarker = L.marker({list(coords)})
            .addTo({map_name})
            .bindPopup('🚢 Start Position<br>Lat: {coords[0]:.6f}<br>Lon: {coords[1]:.6f}')
            .bindTooltip('Start Point');
        
        // Create marker group for trail markers
        window.trailMarkers = L.layerGroup().addTo({map_name});
        
        // Create polyline for trail
        window.trailLine = L.polyline([{list(coords)}], {{
            color: 'blue',
            weight: 3,
            opacity: 0.7
        }}).addTo({map_name});
        
        console.log('✅ Initial marker added');
        """
        
        self.page().runJavaScript(js_code)
        print(f"📍 Initial marker added at {coords}")
    
    def add_marker_js(self, coords: tuple[float, float]):
        """Add marker using JavaScript without regenerating HTML"""
        self.marker_count += 1
        self.trail_coords.append(coords)
        
        map_name = self.folium_map.get_name()
        
        js_code = f"""
        // Add new marker
        var newMarker = L.marker({list(coords)})
            .bindPopup('📍 Point {self.marker_count}<br>Lat: {coords[0]:.6f}<br>Lon: {coords[1]:.6f}')
            .bindTooltip('Point {self.marker_count}');
        
        // Add to marker group
        window.trailMarkers.addLayer(newMarker);
        
        // Update trail line
        var allCoords = {[list(coord) for coord in self.trail_coords]};
        window.trailLine.setLatLngs(allCoords);
        
        // Update popup for trail
        window.trailLine.bindPopup('GPS Trail ({len(self.trail_coords)} points)');
        
        console.log('✅ Marker {self.marker_count} added at {list(coords)}');
        """
        
        self.page().runJavaScript(js_code)
        print(f"📍 Marker {self.marker_count} added at {coords} via JavaScript")

    # def update_map(self, new_coords: tuple[float, float]):
    #     self.folium_map = folium.Map(
    #         location=new_coords,
    #         zoom_start=13,
    #         zoom_control=False,
    #         attribution_control=False
    #     )
    #     self.data = io.BytesIO()
    #     self.folium_map.save(self.data, close_file=False)
    #     self.setHtml(self.data.getvalue().decode())
    
    def update_map(self, new_coords: tuple[float, float]):
         """Update map position and add marker using JavaScript"""
         # Update Python object location
         self.folium_map.location = new_coords
         
         # Add marker for new position
         self.add_marker_js(new_coords)
         
         # Move map view to new position
         map_name = self.folium_map.get_name()
         js_code = f'{map_name}.setView({list(new_coords)})'
         self.page().runJavaScript(js_code)
         
         print(f"🗺️ Map moved to {new_coords} with {len(self.trail_coords)} total points")


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.resize(800, 700)
        self.setCentralWidget(QWidget())
        self.centralWidget().setLayout(QVBoxLayout())
        
        # Starting position (Surabaya)
        self.base_lat = -7.281500
        self.base_lon = 112.798900
        self.current_angle = 0  # For circular/spiral movement
        self.radius = 0.001     # Initial radius for movement
        self.step_count = 0

        self.map_webview = MapWebView((self.base_lat, self.base_lon))
        self.centralWidget().layout().addWidget(self.map_webview)

        # Update every 2 seconds for better visibility
        timer = QTimer(self)
        timer.timeout.connect(self.update_map)
        timer.start(2000)

    def update_map(self):
        """Generate non-linear coordinate pattern"""
        import math
        import random
        
        self.step_count += 1
        
        # Create spiral pattern with random variations
        self.current_angle += 45  # 45 degrees per step
        self.radius += 0.0002     # Gradually increase radius
        
        # Calculate base position using spiral
        angle_rad = math.radians(self.current_angle)
        spiral_lat = self.base_lat + (self.radius * math.cos(angle_rad))
        spiral_lon = self.base_lon + (self.radius * math.sin(angle_rad))
        
        # Add random variations to make it more realistic
        random_lat_offset = random.uniform(-0.0005, 0.0005)
        random_lon_offset = random.uniform(-0.0005, 0.0005)
        
        new_coords = (
            spiral_lat + random_lat_offset,
            spiral_lon + random_lon_offset
        )
        
        print(f"🌀 Step {self.step_count}: Updating map to {new_coords} (angle: {self.current_angle}°)")
        self.map_webview.update_map(new_coords)
        
        # Reset after full circle to prevent going too far
        if self.current_angle >= 360:
            self.current_angle = 0
            self.radius = 0.001  # Reset radius


if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())
