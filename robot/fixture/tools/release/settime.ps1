param([string]$unixlocal="INVALID") #Must be the first statement in your script
#usage: settime.ps1 -unixlocal 1234567890

$cmdstr = "settime $unixlocal"
[Byte[]] $prefix = 0x1B,0x1B,0x0D,0x0D
[Byte[]] $suffix = 0x0D,0x0D
[Byte[]] $cmdbytes = [system.Text.Encoding]::UTF8.GetBytes($cmdstr);
write "$cmdstr"

$COMportList = [System.IO.Ports.SerialPort]::getportnames(); 
ForEach ($COMport in $COMportList)
{
  write "" "$COMport..."
  $com = New-Object System.IO.Ports.SerialPort $COMport,1000000,None,8,one
  $com.ReadTimeout = 500 #ms
  #$com.NewLine = '\n' #linefeed is default
  
  try {
    $com.open()
    #write "$($com.PortName) $($com.BaudRate) $($com.Parity) $($com.DataBits) $($com.StopBits) $($com.isopen) $($com.encoding) $($([system.Text.Encoding]::UTF8.GetBytes($com.NewLine)))"
    
    try {
      $com.Write($prefix, 0, $prefix.Count)
      $com.Write($cmdbytes, 0, $cmdbytes.Count)
      $com.Write($suffix, 0, $suffix.Count)
      
      #print reply
      try { 
        while(1) { 
          $str = $com.ReadLine().Trim(); 
          if( $str.length -gt 0 ) { write "  $str"; } 
        } 
      } catch [TimeoutException] { } #write "readline timeout"; } 
      
      #DEBUG
      try {
        $cnt=0
        while( $com.BytesToRead -gt 0 ) { $temp = $com.ReadByte(); $cnt = $cnt+1; }
        if( $cnt -gt 0 ) { write "  ---- $($cnt) bytes remaining in rx buffer" }
      } catch { write "  ---- RX ERROR"; }
      
      try {
        $com.close()
        #write "  $COMport closed"
        
      } catch { write "PORT CLOSE ERROR"; }
    } catch { write "WRITE ERROR"; }
  } catch { write "port busy"; }
  finally 
  { 
    $com.Dispose() 
  }
}

