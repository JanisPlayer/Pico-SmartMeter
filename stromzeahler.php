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

$apiPassword = "";

// Prüfe Passwort
if (!(isset($_GET['password']) && $_GET['password'] === $apiPassword)) {
    die("Falsches Passwort.");
}

// Verbindung zur Datenbank herstellen (ohne DB damit CREATE DATABASE möglich ist)
$conn = new mysqli($servername, $username, $password);

// Verbindung überprüfen
if ($conn->connect_error) {
    die("Verbindung fehlgeschlagen: " . $conn->connect_error);
}

// DB erstellen falls nicht vorhanden
$conn->query("CREATE DATABASE IF NOT EXISTS `$database`");
$conn->select_db($database);

// Tabelle erstellen
$sql = "CREATE TABLE IF NOT EXISTS power_consumption (
    id INT AUTO_INCREMENT PRIMARY KEY,
    date DATE NOT NULL,
    time TIME NOT NULL,
    power_consumption DECIMAL(10, 2) NOT NULL,
    counter_reading BIGINT NOT NULL
)";

if ($conn->query($sql) !== TRUE) {
    echo "Fehler beim Erstellen der Tabelle: " . $conn->error;
    exit();
}

// INDEX (wie von dir gewünscht)
$sql = "CREATE INDEX IF NOT EXISTS idx_date_time 
        ON power_consumption (date, time)";

if ($conn->query($sql) !== TRUE) {
    echo "Fehler beim erstellen des INDEX: " . $conn->error;
    exit();
}

// Werte
$date = gmdate("Y-m-d");
$time = gmdate("H:i:s");

$power_consumption = null;
if (isset($_GET['power_consumption']) && is_numeric($_GET['power_consumption'])) {
    $power_consumption = intval($_GET['power_consumption']);
    if ($power_consumption < 0 || $power_consumption > 4294967295) {
        $power_consumption = null;
    }
}

$counter_reading = null;
if (isset($_GET['counter_reading']) && is_numeric($_GET['counter_reading'])) {
    $counter_reading = intval($_GET['counter_reading']);
    if ($counter_reading < 0 || $counter_reading > 4294967295) {
        $counter_reading = null;
    }
}

// Insert
if ($power_consumption !== null && $counter_reading !== null) {

    $sql = "INSERT INTO power_consumption (date, time, power_consumption, counter_reading)
            VALUES ('$date', '$time', '$power_consumption', '$counter_reading')";

    if ($conn->query($sql) === TRUE) {
        echo "Daten erfolgreich in die Datenbank eingefügt.";
    } else {
        echo "Fehler beim Einfügen der Daten: " . $conn->error;
    }

} else {
    echo "Ungültige Werte für Stromverbrauch oder Zählerstand.";
}

$conn->close();
?>