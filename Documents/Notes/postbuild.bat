
C:\SysGCC\arm-eabi\bin\arm-none-eabi-objdump -d -EL -S %1  >  %1.lst

C:\SysGCC\arm-eabi\bin\arm-none-eabi-size -A -x %1 > %1.size
C:\SysGCC\arm-eabi\bin\arm-none-eabi-nm -C  -l %1 > %1.nm



