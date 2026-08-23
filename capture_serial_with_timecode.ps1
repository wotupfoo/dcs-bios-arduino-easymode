$port = [System.IO.Ports.SerialPort]::new("COM6", 250000, "None", 8, "One")
$port.NewLine = "`n"
$port.ReadTimeout = 1000
$port.DtrEnable = $true
$port.RtsEnable = $true

$port.Open()

try {
    while ($true) {
        try {
            $line = $port.ReadLine().TrimEnd("`r", "`n")
            "{0:yyyy-MM-dd HH:mm:ss.fff} {1}" -f (Get-Date), $line |
                Tee-Object -FilePath .\serial.log -Append
        }
        catch [System.TimeoutException] {
            # No complete line yet.
        }
        catch [System.IO.IOException] {
            break
        }
    }
}
finally {
    if ($port.IsOpen) {
        $port.Close()
    }
}
