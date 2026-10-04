param(
  [Parameter(Mandatory=$true)][string]$OutputCsv,
  [Parameter(Mandatory=$true)][string]$StopFile
)
$ErrorActionPreference = 'Stop'
if (Test-Path -LiteralPath $OutputCsv) { throw 'refusing to overwrite monitor output' }
'utc_ms,available_physical_bytes,page_reads_per_sec,page_writes_per_sec,vmmem_working_set_bytes,vmmem_private_bytes,c_free_bytes' |
  Set-Content -LiteralPath $OutputCsv -Encoding ascii
while (-not (Test-Path -LiteralPath $StopFile)) {
  $counters = (Get-Counter '\Memory\Page Reads/sec','\Memory\Page Writes/sec' -SampleInterval 1 -MaxSamples 1).CounterSamples
  $reads = ($counters | Where-Object Path -Like '*page reads/sec').CookedValue
  $writes = ($counters | Where-Object Path -Like '*page writes/sec').CookedValue
  $os = Get-CimInstance Win32_OperatingSystem
  $disk = Get-CimInstance Win32_LogicalDisk -Filter "DeviceID='C:'"
  $vm = Get-Process vmmemWSL -ErrorAction SilentlyContinue
  $utc = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
  $line = '{0},{1},{2:R},{3:R},{4},{5},{6}' -f $utc,([int64]$os.FreePhysicalMemory*1024),
    [double]$reads,[double]$writes,[int64]$vm.WorkingSet64,[int64]$vm.PrivateMemorySize64,
    [int64]$disk.FreeSpace
  Add-Content -LiteralPath $OutputCsv -Value $line -Encoding ascii
  Start-Sleep -Seconds 4
}
