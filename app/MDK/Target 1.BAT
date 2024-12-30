SET PATH=C:\Keil_v5\ARM\ARMCC\Bin;C:\Program Files\Common Files\Oracle\Java\javapath;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\;C:\Program Files (x86)\NVIDIA Corporation\PhysX\Common;C:\Program Files\NVIDIA Corporation\NVIDIA NvDLISR;C:\Program Files\Common Files\Autodesk Shared\;C:\Program Files\Microsoft SQL Server\120\Tools\Binn\;C:\Program Files\OpenSSL-Win64\bin;C:\Program Files\dotnet\;C:\Program Files\Git\cmd;C:\Users\Administrator\AppData\Local\Programs\Python\Python310\Scripts\;C:\Users\Administrator\AppData\Local\Programs\Python\Python310\;C:\Users\Administrator\AppData\Local\Microsoft\WindowsApps;C:\Program Files\JetBrains\PyCharm 2022.1\bin;C:\flutter\bin;C:\AndroidSdk;C:\Qt\QtIFW-3.1.0\bin
SET ARMCC5_ASMOPT=--diag_suppress=9931
SET ARMCC5_CCOPT=--diag_suppress=9931
SET ARMCC5_LINKOPT=--diag_suppress=9931
SET CPU_TYPE=HC32L130J8TA
SET CPU_VENDOR=HDSC
SET UV2_TARGET=Target 1
SET CPU_CLOCK=0x00B71B00
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\interrupts_hc32l13x.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\system_hc32l13x.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmAsm" --Via ".\output\release\startup_hc32l13x._ia"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\main.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\spi.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\ddl.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\gpio.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\sysctrl.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\rtc.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\flash.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\bt.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\tm1652.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\uart.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\i2c.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\sht3x.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\ds3231.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\adt.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\adc.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\bgr.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\timer3.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\lpm.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\bsp_delay.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\bsp_gpio.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\bsp_flash.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\bsp_uart.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\bsp_pwm.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\bsp_adc.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\bsp_timer.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\segment.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\release\pama.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmLink" --Via ".\output\release\HC32L130_BaseV1.lnp"
"C:\Keil_v5\ARM\ARMCC\Bin\fromelf.exe" ".\output\release\HC32L130_BaseV1.0" --i32combined --output ".\output\release\HC32L130_BaseV1.hex"
