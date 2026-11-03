<?php

// Definiere das Passwort
$requiredPassword = 'password';

// JSON-Caching in Dateien aktivieren oder deaktivieren (Standardwert: true / false)
$useJsonCache = false;
//if (isset($_GET['cache'])) {
//    $useJsonCache = ($_GET['cache'] === 'true' || $_GET['cache'] === '1');
//}

// Header für JSON-Antwort setzen
header('Content-Type: application/json; charset=utf-8');

// Überprüfe, ob das Passwort im GET-Request vorhanden ist
if (!isset($_GET['password']) || $_GET['password'] !== $requiredPassword) {
    echo json_encode(['error' => 'wrong password']);
    exit;
}

if (isset($_GET['check_password_info'])) {
    echo json_encode(['success' => 'password successful']);
    exit;
}

// Zugangsdaten zur Datenbank
$servername = "localhost";
$username = "stromzeahleruser";
$password = "";
$database = "stromzeahler";

// Verbindung zur Datenbank herstellen
$conn = new mysqli($servername, $username, $password, $database);

// Verbindung überprüfen
if ($conn->connect_error) {
    echo json_encode(['error' => 'Verbindung fehlgeschlagen: ' . $conn->connect_error]);
    exit;
}

// SQL-Befehl zum Erstellen der Tabelle
$sql = "CREATE TABLE IF NOT EXISTS power_consumption (
    id INT AUTO_INCREMENT PRIMARY KEY,
    date DATE NOT NULL,
    time TIME NOT NULL,
    power_consumption DECIMAL(10, 2) NOT NULL,
    counter_reading BIGINT NOT NULL
)";
$conn->query($sql);

// Index sicherstellen (für schnelle Bereichsabfragen auf date und time)
$checkIndex = $conn->query("SHOW INDEX FROM power_consumption WHERE Key_name = 'idx_date_time'");
if ($checkIndex && $checkIndex->num_rows == 0) {
    $conn->query("CREATE INDEX idx_date_time ON power_consumption (date, time)");
}

function getSystemTimezone() {
    $timezone = null;
    if (file_exists('/etc/timezone')) {
        $timezone = trim(file_get_contents('/etc/timezone'));
    } elseif (file_exists('/etc/localtime')) {
        $tz = timezone_name_from_abbr('', @filemtime('/etc/localtime'), 0);
        if ($tz !== false) {
            $timezone = $tz;
        }
    }
    return $timezone ?: 'Europe/Berlin';
}

date_default_timezone_set(getSystemTimezone());
$tzLocal = new DateTimeZone(date_default_timezone_get());
$tzUtc   = new DateTimeZone('UTC');

// Datei-Caching für bereits abgeschlossene Zeiträume (nur wenn $useJsonCache === true)
$cacheDir = __DIR__ . '/cache';
if ($useJsonCache && !is_dir($cacheDir)) {
    @mkdir($cacheDir, 0755, true);
}

function getCache($key) {
    global $cacheDir, $useJsonCache;
    if (!$useJsonCache) {
        return null;
    }
    $file = $cacheDir . '/' . md5($key) . '.json';
    if (file_exists($file)) {
        return file_get_contents($file);
    }
    return null;
}

function setCache($key, $dataJson) {
    global $cacheDir, $useJsonCache;
    if (!$useJsonCache) {
        return;
    }
    if (is_dir($cacheDir) && is_writable($cacheDir)) {
        $file = $cacheDir . '/' . md5($key) . '.json';
        @file_put_contents($file, $dataJson);
    }
}

/**
 * Hilfsfunktion: Holt den ersten oder letzten Datensatz innerhalb eines lokalen Zeitintervalls [startLocal, endLocal)
 * Nutzt den Index (date, time) optimal aus, ohne CONVERT_TZ über die gesamte Tabelle zu rechnen.
 * Ignoriert fehlerhafte 0-Zählerstände (counter_reading > 0), die z.B. bei Lesefehlern am Ende eines Tages/Monats auftreten können.
 */
function getBoundaryRow($conn, DateTime $startLocal, DateTime $endLocal, $first = true) {
    global $tzUtc;
    $startUtc = (clone $startLocal)->setTimezone($tzUtc);
    $endUtc   = (clone $endLocal)->setTimezone($tzUtc);

    $startDate = $startUtc->format('Y-m-d');
    $startTime = $startUtc->format('H:i:s');
    $endDate   = $endUtc->format('Y-m-d');
    $endTime   = $endUtc->format('H:i:s');

    $order = $first ? "ASC" : "DESC";

    $stmt = $conn->prepare("
        SELECT date, time, power_consumption, counter_reading
        FROM power_consumption
        WHERE (date > ? OR (date = ? AND time >= ?))
          AND (date < ? OR (date = ? AND time < ?))
          AND counter_reading > 0
        ORDER BY date $order, time $order
        LIMIT 1
    ");
    $stmt->bind_param("ssssss", $startDate, $startDate, $startTime, $endDate, $endDate, $endTime);
    $stmt->execute();
    $res = $stmt->get_result();
    $row = $res->fetch_assoc();
    $stmt->close();
    return $row ?: null;
}

// ---------------------------------------------------------------------------
// 1. Abfrage: Erster & letzter Wert eines Monats (?months=X) für Monatsstatistik
// ---------------------------------------------------------------------------
if (isset($_GET['months']) && is_numeric($_GET['months'])) {
    $months = max(0, intval($_GET['months']));

    $monthStart = new DateTime('first day of this month 00:00:00', $tzLocal);
    if ($months > 0) {
        $monthStart->modify("-$months months");
    }
    $monthEnd = (clone $monthStart)->modify('+1 month');
    $cacheKey = "months_summary_v3_" . $monthStart->format('Y-m');

    if ($months > 0) {
        $cached = getCache($cacheKey);
        if ($cached !== null) {
            echo $cached;
            $conn->close();
            exit;
        }
    }

    $data = [];
    $firstRow = getBoundaryRow($conn, $monthStart, $monthEnd, true);
    $lastRow  = getBoundaryRow($conn, $monthStart, $monthEnd, false);

    if ($firstRow) $data[] = $firstRow;
    if ($lastRow)  $data[] = $lastRow;

    $json = json_encode($data);
    if ($months > 0 && count($data) >= 2) {
        setCache($cacheKey, $json);
    }

    echo $json;
    $conn->close();
    exit;
}

// ---------------------------------------------------------------------------
// 1b. Abfrage: Erster & letzter Wert eines Kalenderjahres (?year_summary=X)
//     X = 0 (aktuelles Jahr), X = 1 (vorheriges Jahr) oder direkte Jahreszahl (z.B. 2026)
// ---------------------------------------------------------------------------
if (isset($_GET['year_summary']) && is_numeric($_GET['year_summary'])) {
    $param = intval($_GET['year_summary']);
    $now = new DateTime('now', $tzLocal);
    $currentYear = intval($now->format('Y'));

    if ($param >= 2000) {
        $targetYear = $param;
    } else {
        $targetYear = $currentYear - max(0, $param);
    }

    $yearStart = new DateTime("$targetYear-01-01 00:00:00", $tzLocal);
    $yearEnd   = new DateTime(($targetYear + 1) . "-01-01 00:00:00", $tzLocal);
    $isPastYear = ($targetYear < $currentYear);
    $cacheKey = "year_summary_v3_" . $targetYear;

    if ($isPastYear) {
        $cached = getCache($cacheKey);
        if ($cached !== null) {
            echo $cached;
            $conn->close();
            exit;
        }
    }

    $data = [];
    $firstRow = getBoundaryRow($conn, $yearStart, $yearEnd, true);
    $lastRow  = getBoundaryRow($conn, $yearStart, $yearEnd, false);

    if ($firstRow) {
        $firstRow['year'] = $targetYear;
        $data[] = $firstRow;
    }
    if ($lastRow) {
        $lastRow['year'] = $targetYear;
        $data[] = $lastRow;
    }

    $json = json_encode($data);
    if ($isPastYear && count($data) >= 2) {
        setCache($cacheKey, $json);
    }

    echo $json;
    $conn->close();
    exit;
}

// ---------------------------------------------------------------------------
// 2. Abfrage: Monatsansicht Tag 0 bis Tag 31 (?view=month&months_ago=X oder &days=X)
//    Tag 0 = Erster Wert von Tag 1 im Monat
//    Tag 1..31 = Jeweils der letzte Wert (Endwert) von Tag 1 bis Tag 31
//    Auch Tage ohne neue Messung werden als Kalendertag ausgegeben (Zählerstand bleibt gleich).
// ---------------------------------------------------------------------------
if (isset($_GET['view']) && $_GET['view'] === 'month') {
    if (isset($_GET['months_ago']) && is_numeric($_GET['months_ago'])) {
        $monthsAgo = max(0, intval($_GET['months_ago']));
        $targetDate = new DateTime('first day of this month 00:00:00', $tzLocal);
        if ($monthsAgo > 0) {
            $targetDate->modify("-$monthsAgo months");
        }
    } else {
        $days = isset($_GET['days']) && is_numeric($_GET['days']) ? max(0, intval($_GET['days'])) : 0;
        $targetDate = new DateTime('now', $tzLocal);
        if ($days > 0) {
            $targetDate->modify("-$days days");
        }
        $targetDate->modify('first day of this month')->setTime(0, 0, 0);
    }

    $now = new DateTime('now', $tzLocal);
    $isPastMonth = ($targetDate->format('Y-m') < $now->format('Y-m'));
    $cacheKey = "view_month_v3_" . $targetDate->format('Y-m');

    if ($isPastMonth) {
        $cached = getCache($cacheKey);
        if ($cached !== null) {
            echo $cached;
            $conn->close();
            exit;
        }
    }

    $daysInMonth = intval($targetDate->format('t'));
    $monthEnd = (clone $targetDate)->modify('+1 month');

    $data = [];

    // Tag 0: Erster Wert im Monat (ideal von Tag 1, ansonsten erster verfügbarer Wert im Monat)
    $firstRowInMonth = getBoundaryRow($conn, $targetDate, $monthEnd, true);
    if (!$firstRowInMonth) {
        echo json_encode([]);
        $conn->close();
        exit;
    }

    $firstRowInMonth['day_index'] = 0;
    $firstRowInMonth['days_in_month'] = $daysInMonth;
    $firstRowInMonth['label'] = 'Tag 0 (Start)';
    $data[] = $firstRowInMonth;

    $lastKnownRow = $firstRowInMonth;

    // Tag 1 bis Tag 31 (bzw. Anzahl Tage im Monat): Jeweils der Endwert des Tages
    for ($d = 1; $d <= $daysInMonth; $d++) {
        $dayStart = (clone $targetDate)->setDate(intval($targetDate->format('Y')), intval($targetDate->format('m')), $d)->setTime(0, 0, 0);
        if ($dayStart > $now) {
            break; // Zukünftige Tage im aktuellen Monat überspringen
        }
        $dayEnd = (clone $dayStart)->modify('+1 day');

        $dayLastCacheKey = "day_last_v3_" . $dayStart->format('Y-m-d');
        $isPastDay = ($dayEnd <= $now);
        $lastRow = null;

        if ($isPastDay) {
            $cachedDay = getCache($dayLastCacheKey);
            if ($cachedDay !== null) {
                $lastRow = json_decode($cachedDay, true);
            }
        }

        if ($lastRow === null) {
            $lastRow = getBoundaryRow($conn, $dayStart, $dayEnd, false);
            if ($isPastDay && $lastRow !== null) {
                setCache($dayLastCacheKey, json_encode($lastRow));
            }
        }

        // Plausibilitätsprüfung: Zählerstand darf nicht kleiner als der Vortag sein (verhindert Ausreißer durch fehlerhafte Pakete)
        if ($lastRow && intval($lastRow['counter_reading']) >= intval($lastKnownRow['counter_reading'])) {
            $lastKnownRow = $lastRow;
            $entry = $lastRow;
            $entry['has_data'] = true;
        } else {
            // Falls an diesem Tag keine gültige Messung stattfand: Zählerstand vom Vortag fortführen (0 kWh Tagesverbrauch)
            $entry = [
                'date'              => $dayStart->format('Y-m-d'),
                'time'              => '23:59:59',
                'power_consumption' => 0,
                'counter_reading'   => $lastKnownRow['counter_reading'],
                'has_data'          => false
            ];
        }

        $entry['day_index'] = $d;
        $entry['days_in_month'] = $daysInMonth;
        $entry['label'] = sprintf('%02d.%02d.%s', $d, $targetDate->format('m'), $targetDate->format('Y'));
        $data[] = $entry;
    }

    $json = json_encode($data);
    if ($isPastMonth && count($data) > 1) {
        setCache($cacheKey, $json);
    }

    echo $json;
    $conn->close();
    exit;
}

// ---------------------------------------------------------------------------
// 3. Abfrage: Letzte 12 Monate vom ausgewählten Zeitpunkt (?view=months)
//    Monat 0 = Startwert des ersten Monats (vor 11 Monaten)
//    Monat 1..12 = Jeweils der Endwert der 12 Monate bis zum gewählten Monat
// ---------------------------------------------------------------------------
if (isset($_GET['view']) && $_GET['view'] === 'months') {
    if (isset($_GET['months_ago']) && is_numeric($_GET['months_ago'])) {
        $monthsAgo = max(0, intval($_GET['months_ago']));
        $endMonthStart = new DateTime('first day of this month 00:00:00', $tzLocal);
        if ($monthsAgo > 0) {
            $endMonthStart->modify("-$monthsAgo months");
        }
    } else {
        $days = isset($_GET['days']) && is_numeric($_GET['days']) ? max(0, intval($_GET['days'])) : 0;
        $endMonthStart = new DateTime('now', $tzLocal);
        if ($days > 0) {
            $endMonthStart->modify("-$days days");
        }
        $endMonthStart->modify('first day of this month')->setTime(0, 0, 0);
    }

    $now = new DateTime('now', $tzLocal);
    // Startmonat ist 11 Monate vor dem ausgewählten Monat (ergibt insgesamt 12 Monate)
    $startMonth = (clone $endMonthStart)->modify('-11 months');
    $periodEnd  = (clone $endMonthStart)->modify('+1 month');

    $data = [];

    // Punkt 0 (Startwert zu Beginn des 12-Monats-Zeitraums)
    $firstRow = getBoundaryRow($conn, $startMonth, $periodEnd, true);
    if (!$firstRow) {
        echo json_encode([]);
        $conn->close();
        exit;
    }

    $firstRow['month_index'] = 0;
    $firstRow['label'] = 'Start (' . $startMonth->format('m/Y') . ')';
    $data[] = $firstRow;

    $lastKnownRow = $firstRow;

    // Punkte 1..12: Jeweils der Endwert jedes der 12 Monate
    for ($m = 0; $m < 12; $m++) {
        $mStart = (clone $startMonth)->modify("+$m months");
        if ($mStart > $now) {
            break;
        }
        $mEnd = (clone $mStart)->modify('+1 month');
        $isPastMonth = ($mEnd <= $now);
        $mCacheKey = "month_last_v3_" . $mStart->format('Y-m');
        $lastRow = null;

        if ($isPastMonth) {
            $cachedMonth = getCache($mCacheKey);
            if ($cachedMonth !== null) {
                $lastRow = json_decode($cachedMonth, true);
            }
        }

        if ($lastRow === null) {
            $lastRow = getBoundaryRow($conn, $mStart, $mEnd, false);
            if ($isPastMonth && $lastRow !== null) {
                setCache($mCacheKey, json_encode($lastRow));
            }
        }

        if ($lastRow && intval($lastRow['counter_reading']) >= intval($lastKnownRow['counter_reading'])) {
            $lastKnownRow = $lastRow;
            $entry = $lastRow;
            $entry['has_data'] = true;
        } else {
            $entry = [
                'date'              => $mStart->format('Y-m-t'),
                'time'              => '23:59:59',
                'power_consumption' => 0,
                'counter_reading'   => $lastKnownRow['counter_reading'],
                'has_data'          => false
            ];
        }

        $entry['month_index'] = $m + 1;
        $entry['label'] = $mStart->format('m/Y');
        $data[] = $entry;
    }

    echo json_encode($data);
    $conn->close();
    exit;
}

// ---------------------------------------------------------------------------
// 4. Abfrage: Alle letzten Jahre (?view=years)
//    Jahr 0 = Erster bekannter Wert überhaupt (Startwert)
//    Jahr 1..N = Jeweils der Endwert jedes Kalenderjahres
// ---------------------------------------------------------------------------
if (isset($_GET['view']) && $_GET['view'] === 'years') {
    $now = new DateTime('now', $tzLocal);
    $currentYear = intval($now->format('Y'));

    // Ermittle das erste gültige Datum in der Datenbank (counter_reading > 0)
    $resMin = $conn->query("SELECT date, time, power_consumption, counter_reading FROM power_consumption WHERE counter_reading > 0 ORDER BY date ASC, time ASC LIMIT 1");
    $firstEver = $resMin ? $resMin->fetch_assoc() : null;

    if (!$firstEver) {
        echo json_encode([]);
        $conn->close();
        exit;
    }

    $firstUtc = new DateTime($firstEver['date'] . ' ' . $firstEver['time'], $tzUtc);
    $firstLocal = (clone $firstUtc)->setTimezone($tzLocal);
    $startYear = intval($firstLocal->format('Y'));

    $yearsAgo = isset($_GET['years_ago']) && is_numeric($_GET['years_ago']) ? max(0, intval($_GET['years_ago'])) : 0;
    $endYear = max($startYear, $currentYear - $yearsAgo);

    $data = [];

    // Punkt 0: Startwert des ersten Jahres
    $firstEver['year_index'] = 0;
    $firstEver['label'] = 'Start (' . $startYear . ')';
    $data[] = $firstEver;

    $lastKnownRow = $firstEver;
    $idx = 1;

    for ($y = $startYear; $y <= $endYear; $y++) {
        $yStart = new DateTime("$y-01-01 00:00:00", $tzLocal);
        $yEnd   = new DateTime(($y + 1) . "-01-01 00:00:00", $tzLocal);
        $isPastYear = ($yEnd <= $now);
        $yCacheKey = "year_last_v3_" . $y;
        $lastRow = null;

        if ($isPastYear) {
            $cachedYear = getCache($yCacheKey);
            if ($cachedYear !== null) {
                $lastRow = json_decode($cachedYear, true);
            }
        }

        if ($lastRow === null) {
            $lastRow = getBoundaryRow($conn, $yStart, $yEnd, false);
            if ($isPastYear && $lastRow !== null) {
                setCache($yCacheKey, json_encode($lastRow));
            }
        }

        if ($lastRow && intval($lastRow['counter_reading']) >= intval($lastKnownRow['counter_reading'])) {
            $lastKnownRow = $lastRow;
            $entry = $lastRow;
            $entry['has_data'] = true;
        } else {
            $entry = [
                'date'              => "$y-12-31",
                'time'              => '23:59:59',
                'power_consumption' => 0,
                'counter_reading'   => $lastKnownRow['counter_reading'],
                'has_data'          => false
            ];
        }

        $entry['year_index'] = $idx++;
        $entry['label'] = (string)$y;
        $data[] = $entry;
    }

    echo json_encode($data);
    $conn->close();
    exit;
}

// ---------------------------------------------------------------------------
// 5. Standardabfrage: Tagesansicht (?days=X)
// ---------------------------------------------------------------------------
$days = 0;
if (isset($_GET['days']) && is_numeric($_GET['days'])) {
    $days = max(0, intval($_GET['days']));
}

$day = new DateTime('now', $tzLocal);
if ($days > 0) {
    $day->modify("-$days days");
}

$startLocal = (clone $day)->setTime(0, 0, 0);
$endLocal   = (clone $startLocal)->modify('+1 day');

$cacheKeyDay = "view_day_v3_" . $startLocal->format('Y-m-d');
if ($days > 0) {
    $cached = getCache($cacheKeyDay);
    if ($cached !== null) {
        echo $cached;
        $conn->close();
        exit;
    }
}

$startUtc = (clone $startLocal)->setTimezone($tzUtc);
$endUtc   = (clone $endLocal)->setTimezone($tzUtc);

$startDate = $startUtc->format('Y-m-d');
$startTime = $startUtc->format('H:i:s');
$endDate   = $endUtc->format('Y-m-d');
$endTime   = $endUtc->format('H:i:s');

$stmt = $conn->prepare("
    SELECT date, time, power_consumption, counter_reading
    FROM power_consumption
    WHERE (date > ? OR (date = ? AND time >= ?))
      AND (date < ? OR (date = ? AND time < ?))
      AND counter_reading > 0
    ORDER BY date ASC, time ASC
");
$stmt->bind_param("ssssss", $startDate, $startDate, $startTime, $endDate, $endDate, $endTime);
$stmt->execute();
$result = $stmt->get_result();

$data = [];
if ($result && $result->num_rows > 0) {
    while ($row = $result->fetch_assoc()) {
        $data[] = $row;
    }
}
$stmt->close();

$json = json_encode($data);
if ($days > 0 && count($data) > 0) {
    setCache($cacheKeyDay, $json);
}

echo $json;
$conn->close();
?>
