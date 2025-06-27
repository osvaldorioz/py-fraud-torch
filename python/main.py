from fastapi import FastAPI, HTTPException
from fastapi.responses import FileResponse
import fraud_detector
import os
import json
from pathlib import Path
import matplotlib.pyplot as plt

app = FastAPI(docs_url="/docs", redoc_url="/redoc")

DATA_DIR = "/home/hadoop/Documentos/cpp_programs/pybind/py-fraud-torch/data"
Path(DATA_DIR).mkdir(exist_ok=True)
MODEL_PATH = "/home/hadoop/Documentos/cpp_programs/pybind/py-fraud-torch/model/autoencoder.pt"
CSV_PATH = "/home/hadoop/Documentos/cpp_programs/pybind/py-fraud-torch/data/clients_transactions_v6.csv"
detector = fraud_detector.FraudDetector(MODEL_PATH)
loader = fraud_detector.CsvTransactionLoader()

@app.post("/detect-fraud")
async def detect_fraud(idClient: int, datetime: str, amount: float, latitude: float, longitude: float, city: str):
    try:
        # Cargar transacciones históricas solo para idClient
        historical_transactions = [t for t in loader.load_transactions(CSV_PATH) if t.idClient == idClient]
        
        # Crear transacción actual
        transaction = fraud_detector.Transaction()
        transaction.idClient = idClient
        transaction.datetime = datetime
        transaction.amount = amount
        transaction.latitude = latitude
        transaction.longitude = longitude
        transaction.city = city

        # Combinar transacciones históricas y actual
        transactions = historical_transactions + [transaction]
        
        # Detectar fraudes
        alerts = detector.detect_fraud(transactions)
        
        # Generar resultado para la transacción actual
        transaction_id = f"{idClient}_{datetime.replace(' ', '_').replace(':', '-')}"
        is_fraud = False
        reason = "No anomaly detected"
        current_mse = None
        mse_threshold = None
        for alert in alerts:
            if alert.datetime == datetime and alert.idClient == idClient:
                is_fraud = True
                reason = alert.reason
                current_mse = alert.mse
                mse_threshold = alert.mse_threshold
                break
        
        # Generar gráfica de MSE
        mse_values = [alert.mse for alert in alerts if alert.idClient == idClient]
        transaction_ids = [f"{alert.idClient}_{alert.datetime.replace(' ', '_').replace(':', '-')}" 
                          for alert in alerts if alert.idClient == idClient]
        
        plt.figure(figsize=(10, 6))
        plt.plot(transaction_ids, mse_values, marker='o', linestyle='-', color='b', label='MSE')
        if mse_threshold is not None:
            plt.axhline(y=mse_threshold, color='r', linestyle='--', label=f'Umbral MSE ({mse_threshold:.6f})')
        plt.yscale('log')  # Escala logarítmica para manejar valores extremos
        plt.xlabel('ID de Transacción')
        plt.ylabel('MSE (escala logarítmica)')
        plt.title(f'MSE de Transacciones del Cliente {idClient}')
        plt.xticks(rotation=45, ha='right')
        plt.grid(True, which="both", ls="--")
        plt.legend()
        
        # Resaltar la transacción actual
        if current_mse is not None:
            plt.annotate(f'Transacción {transaction_id}\nMSE={current_mse:.3f}', 
                         xy=(transaction_id, current_mse), 
                         xytext=(transaction_id, current_mse * 0.5), 
                         arrowprops=dict(facecolor='black', shrink=0.05))
        
        # Guardar gráfica
        plot_path = os.path.join(DATA_DIR, f"mse_plot_{transaction_id}.png")
        plt.tight_layout()
        plt.savefig(plot_path)
        plt.close()
        
        # Guardar alerta en JSON
        alert_path = os.path.join(DATA_DIR, f"alert_{transaction_id}.json")
        with open(alert_path, "w") as f:
            json.dump({
                "transaction_id": transaction_id,
                "is_fraud": is_fraud,
                "reason": reason,
                "mse": current_mse,
                "mse_threshold": mse_threshold,
                "plot_path": plot_path
            }, f, indent=4)
        
        return {
            "transaction_id": transaction_id,
            "is_fraud": is_fraud,
            "reason": reason,
            "mse": current_mse,
            "mse_threshold": mse_threshold,
            "plot_path": plot_path
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/alert/{transaction_id}")
async def get_alert(transaction_id: str):
    alert_path = os.path.join(DATA_DIR, f"alert_{transaction_id}.json")
    if not os.path.exists(alert_path):
        raise HTTPException(status_code=404, detail="Alert not found")
    return FileResponse(alert_path)

@app.get("/plot/{transaction_id}")
async def get_plot(transaction_id: str):
    plot_path = os.path.join(DATA_DIR, f"mse_plot_{transaction_id}.png")
    if not os.path.exists(plot_path):
        raise HTTPException(status_code=404, detail="Plot not found")
    return FileResponse(plot_path)

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)