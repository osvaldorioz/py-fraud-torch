import pandas as pd
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader
from sklearn.preprocessing import StandardScaler
import numpy as np
import os
import math

# Implementación de haversine en Python
def haversine(lat1, lon1, lat2, lon2):
    R = 6371000  # Radio de la Tierra en metros
    dlat = math.radians(lat2 - lat1)
    dlon = math.radians(lon2 - lon1)
    lat1 = math.radians(lat1)
    lat2 = math.radians(lat2)
    a = math.sin(dlat / 2) * math.sin(dlat / 2) + \
        math.cos(lat1) * math.cos(lat2) * math.sin(dlon / 2) * math.sin(dlon / 2)
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
    return R * c

class Autoencoder(nn.Module):
    def __init__(self):
        super(Autoencoder, self).__init__()
        self.encoder = nn.Sequential(
            nn.Linear(5, 16),
            nn.ReLU(),
            nn.Linear(16, 8),
            nn.ReLU(),
            nn.Linear(8, 4),
            nn.ReLU()
        )
        self.decoder = nn.Sequential(
            nn.Linear(4, 8),
            nn.ReLU(),
            nn.Linear(8, 16),
            nn.ReLU(),
            nn.Linear(16, 5)
        )
    
    def forward(self, x):
        x = self.encoder(x)
        x = self.decoder(x)
        return x

class TransactionDataset(Dataset):
    def __init__(self, data):
        self.data = data
    
    def __len__(self):
        return len(self.data)
    
    def __getitem__(self, idx):
        return self.data[idx]

# Cargar datos
df = pd.read_csv("data/clients_transactions_v6.csv")
primary_cities = df.groupby('idClient')['city'].agg(lambda x: x.value_counts().index[0]).to_dict()
primary_locations = df.groupby('idClient').agg({'latitude': 'median', 'longitude': 'median'}).to_dict('index')

# Calcular estadísticas por cliente
client_stats = df.groupby('idClient').apply(
    lambda x: pd.Series({
        'mean_amount': x['amount'].mean(),
        'std_amount': x['amount'].std(),
        'mean_distance': x.apply(
            lambda row: haversine(
                row['latitude'], row['longitude'],
                primary_locations[row['idClient']]['latitude'],
                primary_locations[row['idClient']]['longitude']
            ), axis=1
        ).mean()
    })
).reset_index()

# Normalizar datos
data = []
for _, row in df.iterrows():
    client_id = row['idClient']
    amount = row['amount']
    distance = haversine(
        row['latitude'], row['longitude'],
        primary_locations[client_id]['latitude'],
        primary_locations[client_id]['longitude']
    )
    stats = client_stats.loc[client_stats['idClient'] == client_id].iloc[0]
    norm_amount = (amount - stats['mean_amount']) / (stats['std_amount'] + 1e-6)
    norm_distance = (distance - stats['mean_distance']) / (stats['std_amount'] + 1e-6)
    hour = pd.to_datetime(row['datetime']).hour
    day_of_week = pd.to_datetime(row['datetime']).dayofweek
    is_foreign = 1.0 if row['city'] != primary_cities[client_id] else 0.0
    data.append([norm_amount, norm_distance, hour, day_of_week, is_foreign])

# Filtrar valores atípicos
data = np.array(data)
data = data[np.abs(data[:, 0]) < 5]  # Filtrar norm_amount > 5 desviaciones
data = data[np.abs(data[:, 1]) < 5]  # Filtrar norm_distance > 5 desviaciones

dataset = TransactionDataset(torch.tensor(data, dtype=torch.float32))
dataloader = DataLoader(dataset, batch_size=32, shuffle=True)

# Entrenar modelo
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
model = Autoencoder().to(device)
criterion = nn.MSELoss()
optimizer = optim.Adam(model.parameters(), lr=0.001)

for epoch in range(50):
    for batch in dataloader:
        batch = batch.to(device)
        optimizer.zero_grad()
        output = model(batch)
        loss = criterion(output, batch)
        loss.backward()
        optimizer.step()
    print(f"Epoch {epoch+1}, Loss: {loss.item()}")

# Guardar modelo
os.makedirs("model", exist_ok=True)
torch.jit.script(model).save("model/autoencoder.pt")