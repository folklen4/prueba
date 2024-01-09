#include <SPI.h>
#include <SD.h>
#include <DHT.h>
#include <sqlite3.h>
#include <RTClib.h>

#define DHTPIN_SENSOR1 32  // Pin al que está conectado el primer DHT (cambia según tu configuración)
#define DHTPIN_SENSOR2 33  // Pin al que está conectado el segundo DHT (cambia según tu configuración)
#define BUTTON 34

DHT dht1(DHTPIN_SENSOR1, DHT22);
DHT dht2(DHTPIN_SENSOR2, DHT11);

sqlite3* db;
RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  rtc.begin();

  pinMode(BUTTON, INPUT);
  pinMode(5, OUTPUT);
  if (!SD.begin(5)) {
    Serial.println("Error al iniciar tarjeta SD");
    return;
  }

  int rc = sqlite3_open("/sd/mydb.db", &db);  // Ruta donde se creará la base de datos en la tarjeta microSD
  if (rc) {
    Serial.printf("Error al abrir la base de datos: %s\n", sqlite3_errmsg(db));
  } else {
    Serial.println("Base de datos abierta con éxito");
    const char* createTableSQL = "CREATE TABLE IF NOT EXISTS data (id INTEGER PRIMARY KEY, sensor TEXT, temp REAL, hum REAL, time INTEGER);";
    rc = sqlite3_exec(db, createTableSQL, 0, 0, 0);
    if (rc != SQLITE_OK) {
      Serial.printf("Error al crear la tabla: %s\n", sqlite3_errmsg(db));
    }
  }

  dht1.begin();
  dht2.begin();
}

void loop() {
  delay(1000);

  DateTime now = rtc.now();
  long unixTime = now.unixtime();

  float temp1 = dht1.readTemperature();
  float hum1 = dht1.readHumidity();

  float temp2 = dht2.readTemperature();
  float hum2 = dht2.readHumidity();

  insertData("sensor1", temp1, hum1, unixTime);
  insertData("sensor2", temp2, hum2, unixTime);
  if (digitalRead(BUTTON) == HIGH) {
    lectura();
  }
}

void insertData(const char* sensor, float temperature, float humidity, unsigned long timestamp) {
  char sql[200];
  snprintf(sql, sizeof(sql), "INSERT INTO data (sensor, temp, hum, time) VALUES ('%s', %.2f, %.2f, %lu);", sensor, temperature, humidity, timestamp);

  int rc = sqlite3_exec(db, sql, 0, 0, 0);
  if (rc != SQLITE_OK) {
    Serial.printf("Error al insertar datos: %s\n", sqlite3_errmsg(db));
  } else {
    Serial.println("Datos insertados correctamente.");
  }
}


void lectura() {
  // Consulta SQL para seleccionar todos los datos de la tabla "data"
  const char* sql = "SELECT * FROM data";


  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK) {
    Serial.printf("Error al preparar la consulta: %s\n", sqlite3_errmsg(db));
  } else {
    Serial.println("Datos almacenados en la base de datos:");

    // Recorremos los resultados y los mostramos en el puerto serie
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      int id = sqlite3_column_int(stmt, 0);
      const char* sensor = (const char*)sqlite3_column_text(stmt, 1);
      float temp = sqlite3_column_double(stmt, 2);
      float hum = sqlite3_column_double(stmt, 3);
      int time = sqlite3_column_int(stmt, 4);
      StaticJsonDocument<200> doc;
      doc["sensor"] = "gps";

      doc["time"] = 1351824120;
      serializeJson(doc, Serial);
      Serial.println("");
      delay(1000);
      Serial.print(id);
      Serial.print(" | ");
      Serial.print(sensor);
      Serial.print(" | ");
      Serial.print(temp);
      Serial.print(" | ");
      Serial.print(hum);
      Serial.print(" | ");
      Serial.println(time);
    }
    // Elimina todos los datos después de mostrarlos
    const char* deleteAllDataSQL = "DELETE FROM data;";
    rc = sqlite3_exec(db, deleteAllDataSQL, 0, 0, 0);
    if (rc != SQLITE_OK) {
      Serial.printf("Error al eliminar datos: %s\n", sqlite3_errmsg(db));
    } else {
      Serial.println("Datos eliminados correctamente.");
    }
    sqlite3_finalize(stmt);  // Liberar recursos después de la consulta
  }

  delay(1000);  // Puedes ajustar el tiempo de espera según tus necesidades
}
