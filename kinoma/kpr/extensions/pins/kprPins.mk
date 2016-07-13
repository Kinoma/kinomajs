<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2016 Marvell International Ltd.
|     Copyright (C) 2002-2010 Kinoma, Inc.
|
|     Licensed under the Apache License, Version 2.0 (the "License");
|     you may not use this file except in compliance with the License.
|     You may obtain a copy of the License at
|
|      http://www.apache.org/licenses/LICENSE-2.0
|
|     Unless required by applicable law or agreed to in writing, software
|     distributed under the License is distributed on an "AS IS" BASIS,
|     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
|     See the License for the specific language governing permissions and
|     limitations under the License.
-->
<makefile>

<input name="$(F_HOME)/core/base"/>
<input name="$(F_HOME)/kinoma/kpr/"/>
<input name="$(F_HOME)/kinoma/kpr/sources/"/>

<header name="kpr.h"/>
<header name="kprPins.h"/>

<platform name="linux/aspen,linux/poky">
<source name="i2cdev.c"/>
<source name="K4Gen2.c"/>
<source name="k4Gen2a2d.c"/>
<source name="k4Gen2gpio.c"/>
<source name="k4pwm.c"/>
<source name="ttySerial.c"/>
<source name="k4spi.c"/>
<source name="spidev.c"/>
<common>
C_OPTIONS += -DDEVICE -DUSEA2D -DUSEGPIO -DUSEI2C -DK4GEN2 -DUSEPWM -DUSESERIAL -DUSESPI
XSC_OPTIONS += -t DEVICE -t USEA2D -t USEGPIO -t USEI2C -t K4GEN2 -t USEPWM -t USESERIAL -t USESPI
</common>
</platform>

<platform name="linux/iap140,linux/linkit7688,linux/pine64,linux/edison,linux/pi_gl,linux/pi,linux/beaglebone,linux/andro,linux/guitar,linux/hikey,linux/odroidc2,linux/nanopim1">
<source name="i2c.c"/>
<source name="i2cdev.c"/>
<source name="a2d.c"/>
<source name="gpio.c"/>
<source name="pwm.c"/>
<source name="serial.c"/>
<!-- source name="ttySerial.c"/-->
<!-- source name="spi.c"/ -->
<!-- source name="spidev.c"/ -->
<common>
C_OPTIONS += -DDEVICE -DUSEA2D -DUSEGPIO -DUSEI2C -DUSEPWM -DUSESERIAL
XSC_OPTIONS += -t DEVICE -t USEA2D -t USEGPIO -t USEI2C -t USEPWM -t USESERIAL
</common>
</platform>

<platform name="mac">
<common>
C_OPTIONS += -DMOCKUP -DUSERANDOM
XSC_OPTIONS += -t MOCKUP -t USERANDOM
</common>
</platform>
<platform name="win">
<common>
C_OPTIONS += /DMOCKUP /DUSERANDOM
XSC_OPTIONS += -t MOCKUP -t USERANDOM
</common>
</platform>
<platform name="linux/gtk">
<common>
C_OPTIONS += -DMOCKUP -DUSERANDOM
XSC_OPTIONS += -t MOCKUP -t USERANDOM
</common>
</platform>

</makefile>

