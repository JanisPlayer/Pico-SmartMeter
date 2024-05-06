<?php
// Fehlerausgabe aktivieren
ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);
error_reporting(E_ALL);

// Zugangsdaten zur Datenbank
$servername = "localhost";
$username = "stromzeahleruser";
$password = "";
$database = "stromzeahler";

// Pr�fe Passwort
if (!(isset($_GET['password']) && $_GET['password'] === $password)) {
    die("Falsches Passwort.");
}

// Verbindung zur Datenbank herstellen
$conn = new mysqli($servername, $username, $password, $database);

// SQL-Befehl zum Erstellen der Tabelle
$sql = "CREATE TABLE IF NOT EXISTS power_consumption (
    id INT AUTO_INCREMENT PRIMARY KEY,
    date DATE NOT NULL,
    time TIME NOT NULL,
    power_consumption DECIMAL(10, 2) NOT NULL,
    counter_reading BIGINT NOT NULL
)";

// Versuche das Statement auszuf�hren
if ($conn->query($sql) === TRUE) {
} else {
    echo "Fehler beim Erstellen der Tabelle: " . $conn->error;
}

// Verbindung �berpr�fen
if ($conn->connect_error) {
    die("Verbindung fehlgeschlagen: " . $conn->connect_error);
}

// Werte aus dem GET-Array oder anderer Quelle abrufen
//$date = date("Y-m-d"); // Aktuelles Datum
//$time = date("H:i:s"); // Aktuelle Uhrzeit
$date = gmdate("Y-m-d"); // Aktuelles Datum im UTC-Format
$time = gmdate("H:i:s"); // Aktuelle Uhrzeit im UTC-Format


$power_consumption = null;
// �berpr�fen, ob ein GET-Parameter �bergeben wurde und ob er eine g�ltige Zahl ist
if (isset($_GET['power_consumption']) && is_numeric($_GET['power_consumption'])) {
    $power_consumption = intval($_GET['power_consumption']);
    // �berpr�fen, ob der Wert im akzeptablen Bereich liegt
    if ($power_consumption < 0 || $power_consumption > 4294967295) {
        $power_consumption = null;
    }
}

$counter_reading = null;
// �berpr�fen, ob ein GET-Parameter �bergeben wurde und ob er eine g�ltige Zahl ist
if (isset($_GET['counter_reading']) && is_numeric($_GET['counter_reading'])) {
    $counter_reading = intval($_GET['counter_reading']);
    // �berpr�fen, ob der Wert im akzeptablen Bereich liegt
    if ($counter_reading < 0 || $counter_reading > 4294967295) {
        $counter_reading = null;
    }
}

// �berpr�fe, ob die Variablen g�ltige Werte enthalten
if ($power_consumption !== null && $counter_reading !== null) {
    // SQL-Statement zum Einf�gen der Werte in die Tabelle
    $sql = "INSERT INTO power_consumption (date, time, power_consumption, counter_reading) 
            VALUES ('$date', '$time', '$power_consumption', '$counter_reading')";

    // Versuche das Statement auszuf�hren
    if ($conn->query($sql) === TRUE) {
        echo "Daten erfolgreich in die Datenbank eingef�gt.";
    } else {
        echo "Fehler beim Einf�gen der Daten: " . $conn->error;
    }
} else {
    echo "Ung�ltige Werte f�r Stromverbrauch oder Z�hlerstand.";
}
// Verbindung schlie�en
$conn->close();
?>
