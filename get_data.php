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

// Überprüfen, ob ein GET-Parameter übergeben wurde und ob er eine gültige Zahl ist
if (isset($_GET['months']) && is_numeric($_GET['months'])) {
    $months = intval($_GET['months']);
    if ($months < 0) {
        $months = 0;
    }

    // SQL-Abfragen für die ersten und letzten Werte des Monats
    $sql_first = "SELECT date, time, power_consumption, counter_reading
                  FROM power_consumption
                  WHERE DATE(CONVERT_TZ(CONCAT(date, ' ', time), '+00:00', @@session.time_zone)) >= LAST_DAY(CURDATE() - INTERVAL $months MONTH) + INTERVAL 1 DAY - INTERVAL 1 MONTH
                  AND DATE(CONVERT_TZ(CONCAT(date, ' ', time), '+00:00', @@session.time_zone)) < LAST_DAY(CURDATE() - INTERVAL $months MONTH) + INTERVAL 1 DAY
                  ORDER BY date ASC, time ASC
                  LIMIT 1";

    $sql_last = "SELECT date, time, power_consumption, counter_reading
                 FROM power_consumption
                 WHERE DATE(CONVERT_TZ(CONCAT(date, ' ', time), '+00:00', @@session.time_zone)) >= LAST_DAY(CURDATE() - INTERVAL $months MONTH) + INTERVAL 1 DAY - INTERVAL 1 MONTH
                 AND DATE(CONVERT_TZ(CONCAT(date, ' ', time), '+00:00', @@session.time_zone)) < LAST_DAY(CURDATE() - INTERVAL $months MONTH) + INTERVAL 1 DAY
                 ORDER BY date DESC, time DESC
                 LIMIT 1";

    $data = array();

    // Daten aus der ersten Abfrage abrufen
    $result_first = $conn->query($sql_first);
    if ($result_first->num_rows > 0) {
        while ($row = $result_first->fetch_assoc()) {
            $data[] = $row;
        }
    }

    // Daten aus der zweiten Abfrage abrufen
    $result_last = $conn->query($sql_last);
    if ($result_last->num_rows > 0) {
        while ($row = $result_last->fetch_assoc()) {
            $data[] = $row;
        }
    }

    // JSON-Ausgabe der gesammelten Daten
    echo json_encode($data);

    // Verbindung schließen
    $conn->close();
    die();
}

// Standardabfrage, wenn keine Monate angegeben sind
$sql = "SELECT date, time, power_consumption, counter_reading
        FROM power_consumption
        WHERE DATE(CONVERT_TZ(CONCAT(date, ' ', time), '+00:00', @@session.time_zone)) = CURDATE() - INTERVAL $days DAY
        ORDER BY date ASC, time ASC";

// Versuche das Statement auszuführen
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
