$port = [System.IO.Ports.SerialPort]::new("COM6",250000,"None",8,"One")
$port.NewLine = "`n"
$port.ReadTimeout = 1000
$port.DtrEnable = $true
$port.RtsEnable = $true

$port.Open()

try {
    while ($true) {
        try {
            $line = $port.ReadLine().TrimEnd("`r", "`n")
            $line | Tee-Object -FilePath .\serial.log -Append
        }
        catch [System.TimeoutException] {
            # no complete line yet
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