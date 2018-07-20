#!/bin/sh
# Work around menuman crash by kicking off this work in the background and returning instantly
if [ "$3" == "" ]; then
  /anki/menuman/system.sh $1 $2 async &
  exit 0
fi

# Try to stop everything - if it fails, restart the wlan module
systemctl stop ankibluetoothd.service
if ! iwpriv wlan0 tx 0; then
  rmmod wlan.ko
  insmod /usr/lib/modules/3.18.66-perf/extra/wlan.ko con_mode=5
  iwpriv wlan0 ftm 1
  iwpriv wlan0 ena_chain 2
fi
iwpriv wlan0 tx_cw_rf_gen 0             # Carrier off
/anki/menuman/btconfig --soc smd /dev/smd3 rawcmd 0x08 0x001F   # BLE off

if [ $1 == STOP ]; then
  echo Stopped
  exit 0
fi
if [ $1 == BLE ]; then
  echo BLE pseudo-random TX on BLE channel $2 - hex 0x0..0x27
  rmmod wlan # Must stop WLAN before BLE test can take over
  /anki/menuman/btconfig --soc smd /dev/smd3 rawcmd 0x08 0x001E $2 0x23 0x00  # See KBA-170629211501 - 0x23=60% duty cycle
  exit 0
fi
if [ $1 == RX ]; then
  echo BLE RX on BLE channel $2 - hex 0x0..0x27
  rmmod wlan # Must stop WLAN before BLE test can take over
  /anki/menuman/btconfig --soc smd /dev/smd3 rawcmd 0x08 0x001D $2 0x23 0x00  # See KBA-170629211501
  exit 0
fi

echo Selecting WiFi channel $2
iwpriv wlan0 set_channel $2              #select Wifi channel 1-11
iwpriv wlan0 pwr_cntl_mode 1             #select SCPC control mode
iwpriv wlan0 set_txpower 16              #SCPC power setting

if [ $1 == CW ]; then
  echo TX carrier
  iwpriv wlan0 tx_cw_rf_gen 1
  exit 0
fi

echo TX $1
iwpriv wlan0 set_txrate $1               #TX mode (n-mode for wide, worst-case bandedge waveform)
iwpriv wlan0 set_txpktcnt 0              #Continuously transmit
iwpriv wlan0 set_txpktlen 1500           #packet length
iwpriv wlan0 set_txifs 10                #interval between packets (uSeconds)
iwpriv wlan0 tx 1                        #Start Transmit

exit 0
