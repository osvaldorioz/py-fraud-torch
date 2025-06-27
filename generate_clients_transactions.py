import random
import csv
from datetime import datetime, timedelta
import math
from collections import defaultdict
import calendar

# Configuración general
num_clients = 100
start_date = datetime(2024, 1, 1)
end_date = datetime(2025, 6, 23)
meters_to_deg = 1 / 111_320

cities_coords = {
    "Mexico City": (19.4326, -99.1332),
    "Guadalajara": (20.6597, -103.3496),
    "Monterrey": (25.6866, -100.3161),
    "New York": (40.7128, -74.0060),
    "Los Angeles": (34.0522, -118.2437),
    "Tokyo": (35.6895, 139.6917),
    "Paris": (48.8566, 2.3522),
    "London": (51.5074, -0.1278),
    "Berlin": (52.5200, 13.4050),
    "Madrid": (40.4168, -3.7038),
    "Toronto": (43.6532, -79.3832),
    "Buenos Aires": (-34.6037, -58.3816),
}

foreign_cities = ["Paris", "London", "Tokyo", "Berlin", "New York", "Toronto", "Buenos Aires", "Madrid"]

def num_transactions_per_month():
    p = random.random()
    if p < 0.5:
        return random.randint(5, 50)
    elif p < 0.8:
        return random.randint(51, 200)
    elif p < 0.95:
        return random.randint(201, 400)
    else:
        return random.randint(401, 500)

foreign_clients = random.sample(range(1000, 1000 + num_clients), 10)
unusual_clients = random.sample(range(1000, 1000 + num_clients), 10)

# Obtener lista de meses válidos
months = []
current_month = start_date.replace(day=1)
while current_month <= end_date:
    months.append(current_month)
    year = current_month.year
    month = current_month.month
    if month == 12:
        next_month = datetime(year + 1, 1, 1)
    else:
        next_month = datetime(year, month + 1, 1)
    current_month = next_month

rows = [("idClient", "datetime", "amount", "latitude", "longitude", "city")]

for client_id in range(1000, 1000 + num_clients):
    primary_city = "Mexico City" if random.random() < 0.8 else random.choice(list(cities_coords.keys()))
    primary_lat, primary_lon = cities_coords[primary_city]

    insert_geo_fraud = client_id in unusual_clients and random.random() < 0.7
    insert_amount_fraud = random.random() < 0.1
    insert_foreign_tx = client_id in foreign_clients

    unusual_triggered = defaultdict(lambda: False)

    for month_start in months:
        year = month_start.year
        month = month_start.month
        days_in_month = calendar.monthrange(year, month)[1]  # <-- esta es la corrección!

        tx_count = num_transactions_per_month()
        days_with_tx = sorted(random.choices(range(days_in_month), k=random.randint(5, min(tx_count, days_in_month))))

        for tx_num in range(tx_count):
            day = random.choice(days_with_tx) + 1  # día válido (1 a days_in_month)
            tx_datetime = datetime(year, month, day,
                                   hour=random.randint(0, 23),
                                   minute=random.randint(0, 59),
                                   second=random.randint(0, 59))

            if tx_datetime > end_date:
                continue

            tx_datetime_str = tx_datetime.strftime("%Y-%m-%d %H:%M:%S")

            distance_meters = random.choice([10, 50, 100, 300, 500, 1000, 2000]) if random.random() < 0.8 else random.choice([5000, 10000])
            distance_deg = distance_meters * meters_to_deg

            angle = random.uniform(0, 2 * math.pi)
            delta_lat = distance_deg * math.cos(angle)
            delta_lon = distance_deg * math.sin(angle)

            latitude = primary_lat + delta_lat
            longitude = primary_lon + delta_lon

            if insert_amount_fraud and random.random() < 0.05:
                amount = round(random.uniform(10000, 50000), 2)
            else:
                amount = round(random.uniform(10, 500), 2)

            city = primary_city

            if insert_foreign_tx and month_start == months[0] and tx_num == 0:
                foreign_city = random.choice(foreign_cities)
                f_lat, f_lon = cities_coords[foreign_city]
                latitude = f_lat + random.uniform(-0.01, 0.01)
                longitude = f_lon + random.uniform(-0.01, 0.01)
                city = foreign_city

            rows.append((client_id, tx_datetime_str, amount, latitude, longitude, city))

            if (insert_geo_fraud and not unusual_triggered[month_start] and random.random() < 0.3 and tx_num < tx_count - 1):
                unusual_triggered[month_start] = True
                tx_datetime_str_dup = tx_datetime.strftime("%Y-%m-%d %H:%M:%S")

                far_distance_meters = random.choice([5000, 10000, 20000, 30000])
                far_distance_deg = far_distance_meters * meters_to_deg

                angle_dup = random.uniform(0, 2 * math.pi)
                delta_lat_dup = far_distance_deg * math.cos(angle_dup)
                delta_lon_dup = far_distance_deg * math.sin(angle_dup)

                latitude_dup = primary_lat + delta_lat_dup
                longitude_dup = primary_lon + delta_lon_dup

                amount_dup = round(random.uniform(10, 500), 2)
                city_dup = primary_city if random.random() < 0.5 else random.choice(list(cities_coords.keys()))

                rows.append((client_id, tx_datetime_str_dup, amount_dup, latitude_dup, longitude_dup, city_dup))

# Guardar CSV
output_file = "clients_transactions_v6.csv"
with open(output_file, "w", newline='') as csvfile:
    writer = csv.writer(csvfile)
    writer.writerows(rows)

print(f"Archivo generado: {output_file}")