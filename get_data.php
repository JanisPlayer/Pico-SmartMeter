<?php
// Zugangsdaten zur Datenbank
$servername = "localhost";
$username = "stromzeahleruser";
$password = "";
$database = "stromzeahler";

// Verbindung zur Datenbank herstellen
$conn = new mysqli($servername, $username, $password, $database);

// Verbindung überprüfen
if ($conn->connect_error) {
    die("Verbindung fehlgeschlagen: " . $conn->connect_error);
}

// SQL-Befehl zum Erstellen der Tabelle
$sql = "CREATE TABLE IF NOT EXISTS power_consumption (
    id INT AUTO_INCREMENT PRIMARY KEY,
    date DATE NOT NULL,
    time TIME NOT NULL,
    power_consumption DECIMAL(10, 2) NOT NULL,
    counter_reading BIGINT NOT NULL
)";

// Versuche das Statement auszuführen
if ($conn->query($sql) === TRUE) {
} else {
    echo "Fehler beim Erstellen der Tabelle: " . $conn->error;
}

// SQL-Statement zum Abrufen der Daten aus der Tabelle für die letzten 24 Stunden
//$sql = "SELECT date, time, power_consumption, counter_reading FROM power_consumption WHERE date >= DATE_SUB(CURDATE(), INTERVAL 1 DAY) ORDER BY date ASC, time ASC";

/*$sql = "SELECT date, time, power_consumption, counter_reading
        FROM power_consumption
        WHERE CONCAT(date, ' ', time) >= DATE_SUB(NOW(), INTERVAL 24 HOUR)
        ORDER BY date ASC, time ASC";*/


// Standardwert für die Anzahl der Tage
$days = 0;

// Überprüfen, ob ein GET-Parameter übergeben wurde und ob er eine gültige Zahl ist
if (isset($_GET['days']) && is_numeric($_GET['days'])) {
    $days = intval($_GET['days']);
    if ($days < 0) {
        $days = 0;
    }
}

$months = 0;

//Muss noch gebaut werden, das der erste und letzte Wert ausgeben wird, zusammengerechnet und dann in eine andere Tabelle geschrieben als Cache.
if (isset($_GET['months']) && is_numeric($_GET['months'])) {
    $months = intval($_GET['months']);
    if ($months < 0) {
        $months = 0;
    }
$sql_last = "SELECT date, time, power_consumption, counter_reading
             FROM power_consumption
             WHERE DATE(CONVERT_TZ(CONCAT(date, ' ', time), '+00:00', '+02:00')) >= LAST_DAY(CURDATE() - INTERVAL $months MONTH) - INTERVAL DAY(LAST_DAY(CURDATE() - INTERVAL $months MONTH)) DAY
             AND DATE(CONVERT_TZ(CONCAT(date, ' ', time), '+00:00', '+02:00')) < LAST_DAY(CURDATE() - INTERVAL $months - 1 MONTH) - INTERVAL DAY(LAST_DAY(CURDATE() - INTERVAL $months - 1 MONTH)) DAY
             ORDER BY date DESC, time DESC
             LIMIT 1";

$sql_first = "SELECT date, time, power_consumption, counter_reading
              FROM power_consumption
              WHERE DATE(CONVERT_TZ(CONCAT(date, ' ', time), '+00:00', '+02:00')) >= CURDATE() - INTERVAL $months +1 MONTH
              AND DATE(CONVERT_TZ(CONCAT(date, ' ', time), '+00:00', '+02:00')) < CURDATE() - INTERVAL ($months -1) MONTH
              ORDER BY date ASC, time ASC
              LIMIT 1";

$sql_sec = "SELECT date, time, power_consumption, counter_reading
              FROM power_consumption
              WHERE DATE(CONVERT_TZ(CONCAT(date, ' ', time), '+00:00', '+02:00')) >= CURDATE() - INTERVAL $months MONTH
              AND DATE(CONVERT_TZ(CONCAT(date, ' ', time), '+00:00', '+02:00')) < CURDATE() - INTERVAL ($months - 1) MONTH
              ORDER BY date ASC, time ASC
              LIMIT 1";

$sql = $sql_first;

// Daten aus der ersten Abfrage abrufen
$result_first = $conn->query($sql_first);
$data = array();
if ($result_first->num_rows > 0) {
    while ($row = $result_first->fetch_assoc()) {
        $data[] = $row;
    }
}

// Daten aus der zweiten Abfrage abrufen
$result_sec = $conn->query($sql_sec);
if ($result_sec->num_rows > 0) {
    while ($row = $result_sec->fetch_assoc()) {
        $data[] = $row;
    }
}

// JSON-Ausgabe der gesammelten Daten
echo json_encode($data);

// Verbindung schließen
$conn->close();
die();
}

if (!(isset($_GET['months']))) {
$sql = "SELECT date, time, power_consumption, counter_reading
        FROM power_consumption
        WHERE DATE(CONVERT_TZ(CONCAT(date, ' ', time), '+00:00', '+02:00')) = CURDATE() - INTERVAL $days DAY
        ORDER BY date ASC, time ASC";
}

// Versuche das Statement auszufÜhren
$result = $conn->query($sql);

// Überprüfe, ob Daten vorhanden sind
if ($result->num_rows > 0) {
    $data = array();
    // Daten in Array speichern
    while($row = $result->fetch_assoc()) {
        $data[] = $row;
    }
    echo json_encode($data);
} else {
    echo json_encode(array());
}

// Verbindung schließen
$conn->close();
?>
