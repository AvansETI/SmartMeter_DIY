EESchema Schematic File Version 4
LIBS:SmartMeter-cache
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Smart Meter"
Date "2019-07-02"
Rev "v2.0"
Comp "Avans Hogeschool"
Comment1 "Expertisecentrum Technische Innovatie"
Comment2 "Lectoraat Smart Energy"
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L wemos_mini:WeMos_mini U1
U 1 1 5D587898
P 2300 3850
F 0 "U1" H 2300 4500 60  0000 C CNN
F 1 "WeMos_mini" H 2300 4400 60  0000 C CNN
F 2 "WEMOS_D1_V3:D1-MINI" H 2850 3150 60  0001 C CNN
F 3 "" H 2300 4381 60  0000 C CNN
	1    2300 3850
	1    0    0    -1  
$EndComp
$Comp
L Connector:RJ12 J1
U 1 1 5D19FFB2
P 7150 2850
F 0 "J1" H 6820 2854 50  0000 R CNN
F 1 "RJ12" H 6820 2945 50  0000 R CNN
F 2 "Connector_RJ:RJ12_Amphenol_54601" V 7150 2875 50  0001 C CNN
F 3 "~" V 7150 2875 50  0001 C CNN
	1    7150 2850
	1    0    0    1   
$EndComp
NoConn ~ 7550 2650
$Comp
L power:GND #PWR04
U 1 1 5D1A015D
P 8050 3050
F 0 "#PWR04" H 8050 2800 50  0001 C CNN
F 1 "GND" H 8055 2877 50  0000 C CNN
F 2 "" H 8050 3050 50  0001 C CNN
F 3 "" H 8050 3050 50  0001 C CNN
	1    8050 3050
	1    0    0    -1  
$EndComp
Wire Wire Line
	7550 2850 8050 2850
Wire Wire Line
	8050 2850 8050 3050
$Comp
L power:+5V #PWR03
U 1 1 5D1A01B1
P 8050 2600
F 0 "#PWR03" H 8050 2450 50  0001 C CNN
F 1 "+5V" H 8065 2773 50  0000 C CNN
F 2 "" H 8050 2600 50  0001 C CNN
F 3 "" H 8050 2600 50  0001 C CNN
	1    8050 2600
	1    0    0    -1  
$EndComp
Wire Wire Line
	7550 2750 8050 2750
Wire Wire Line
	8050 2750 8050 2600
Text GLabel 7550 3050 2    50   Input ~ 0
TXD
NoConn ~ 7550 2950
NoConn ~ 7550 3150
$Comp
L power:+5V #PWR07
U 1 1 5D1A02F6
P 1550 3350
F 0 "#PWR07" H 1550 3200 50  0001 C CNN
F 1 "+5V" H 1565 3523 50  0000 C CNN
F 2 "" H 1550 3350 50  0001 C CNN
F 3 "" H 1550 3350 50  0001 C CNN
	1    1550 3350
	1    0    0    -1  
$EndComp
Wire Wire Line
	1800 3500 1550 3500
Wire Wire Line
	1550 3500 1550 3350
$Comp
L power:+3.3V #PWR010
U 1 1 5D1A03CB
P 3100 3300
F 0 "#PWR010" H 3100 3150 50  0001 C CNN
F 1 "+3.3V" H 3115 3473 50  0000 C CNN
F 2 "" H 3100 3300 50  0001 C CNN
F 3 "" H 3100 3300 50  0001 C CNN
	1    3100 3300
	1    0    0    -1  
$EndComp
Wire Wire Line
	2800 3500 3100 3500
Wire Wire Line
	3100 3500 3100 3300
$Comp
L Transistor_BJT:BC547 Q1
U 1 1 5D1A04B1
P 1800 6200
F 0 "Q1" H 1991 6246 50  0000 L CNN
F 1 "BC547" H 1991 6155 50  0000 L CNN
F 2 "Package_TO_SOT_THT:TO-92_Wide" H 2000 6125 50  0001 L CIN
F 3 "http://www.fairchildsemi.com/ds/BC/BC547.pdf" H 1800 6200 50  0001 L CNN
	1    1800 6200
	1    0    0    -1  
$EndComp
Text GLabel 850  6200 0    50   Input ~ 0
TXD
$Comp
L power:+5V #PWR01
U 1 1 5D1A0651
P 1050 5800
F 0 "#PWR01" H 1050 5650 50  0001 C CNN
F 1 "+5V" H 1065 5973 50  0000 C CNN
F 2 "" H 1050 5800 50  0001 C CNN
F 3 "" H 1050 5800 50  0001 C CNN
	1    1050 5800
	1    0    0    -1  
$EndComp
$Comp
L Device:R_Small R1
U 1 1 5D1A06D2
P 1300 6200
F 0 "R1" V 1104 6200 50  0000 C CNN
F 1 "1kΩ" V 1195 6200 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 1300 6200 50  0001 C CNN
F 3 "~" H 1300 6200 50  0001 C CNN
	1    1300 6200
	0    1    1    0   
$EndComp
Wire Wire Line
	1400 6200 1500 6200
$Comp
L Diode:1N4148 D1
U 1 1 5D1A08CB
P 1500 6550
F 0 "D1" V 1450 6300 50  0000 L CNN
F 1 "1N4148" V 1550 6150 50  0000 L CNN
F 2 "Diode_THT:D_DO-35_SOD27_P7.62mm_Horizontal" H 1500 6375 50  0001 C CNN
F 3 "http://www.nxp.com/documents/data_sheet/1N4148_1N4448.pdf" H 1500 6550 50  0001 C CNN
	1    1500 6550
	0    1    1    0   
$EndComp
Wire Wire Line
	1500 6200 1500 6400
Connection ~ 1500 6200
Wire Wire Line
	1500 6200 1600 6200
$Comp
L power:GND #PWR02
U 1 1 5D1A09EE
P 1500 6850
F 0 "#PWR02" H 1500 6600 50  0001 C CNN
F 1 "GND" H 1505 6677 50  0000 C CNN
F 2 "" H 1500 6850 50  0001 C CNN
F 3 "" H 1500 6850 50  0001 C CNN
	1    1500 6850
	1    0    0    -1  
$EndComp
Wire Wire Line
	1500 6700 1500 6850
$Comp
L Device:R_Small R2
U 1 1 5D1A0B02
P 1900 6650
F 0 "R2" H 1841 6604 50  0000 R CNN
F 1 "1kΩ" H 1841 6695 50  0000 R CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 1900 6650 50  0001 C CNN
F 3 "~" H 1900 6650 50  0001 C CNN
	1    1900 6650
	-1   0    0    1   
$EndComp
Wire Wire Line
	1900 6400 1900 6500
$Comp
L power:GND #PWR06
U 1 1 5D1A0C99
P 1900 6850
F 0 "#PWR06" H 1900 6600 50  0001 C CNN
F 1 "GND" H 1905 6677 50  0000 C CNN
F 2 "" H 1900 6850 50  0001 C CNN
F 3 "" H 1900 6850 50  0001 C CNN
	1    1900 6850
	1    0    0    -1  
$EndComp
Wire Wire Line
	1900 6750 1900 6850
$Comp
L Device:R_Small R3
U 1 1 5D1A0DFA
P 2600 6500
F 0 "R3" V 2404 6500 50  0000 C CNN
F 1 "1kΩ" V 2495 6500 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 2600 6500 50  0001 C CNN
F 3 "~" H 2600 6500 50  0001 C CNN
	1    2600 6500
	0    1    1    0   
$EndComp
Wire Wire Line
	1900 6500 2500 6500
Connection ~ 1900 6500
Wire Wire Line
	1900 6500 1900 6550
$Comp
L Diode:1N4148 D2
U 1 1 5D1A0F8D
P 2950 6150
F 0 "D2" V 2900 6300 50  0000 L CNN
F 1 "1N4148" V 3000 6300 50  0000 L CNN
F 2 "Diode_THT:D_DO-35_SOD27_P7.62mm_Horizontal" H 2950 5975 50  0001 C CNN
F 3 "http://www.nxp.com/documents/data_sheet/1N4148_1N4448.pdf" H 2950 6150 50  0001 C CNN
	1    2950 6150
	0    1    1    0   
$EndComp
$Comp
L Transistor_BJT:BC547 Q2
U 1 1 5D1A1035
P 3400 6500
F 0 "Q2" H 3591 6546 50  0000 L CNN
F 1 "BC547" H 3591 6455 50  0000 L CNN
F 2 "Package_TO_SOT_THT:TO-92_Wide" H 3600 6425 50  0001 L CIN
F 3 "http://www.fairchildsemi.com/ds/BC/BC547.pdf" H 3400 6500 50  0001 L CNN
	1    3400 6500
	1    0    0    -1  
$EndComp
Wire Wire Line
	2700 6500 2950 6500
Wire Wire Line
	2950 6300 2950 6500
Connection ~ 2950 6500
Wire Wire Line
	2950 6500 3200 6500
$Comp
L power:+3.3V #PWR08
U 1 1 5D1A140E
P 3500 5250
F 0 "#PWR08" H 3500 5100 50  0001 C CNN
F 1 "+3.3V" H 3515 5423 50  0000 C CNN
F 2 "" H 3500 5250 50  0001 C CNN
F 3 "" H 3500 5250 50  0001 C CNN
	1    3500 5250
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR09
U 1 1 5D1A170A
P 3500 6800
F 0 "#PWR09" H 3500 6550 50  0001 C CNN
F 1 "GND" H 3505 6627 50  0000 C CNN
F 2 "" H 3500 6800 50  0001 C CNN
F 3 "" H 3500 6800 50  0001 C CNN
	1    3500 6800
	1    0    0    -1  
$EndComp
Wire Wire Line
	3500 6700 3500 6800
Wire Wire Line
	2950 6000 2950 5800
Wire Wire Line
	2950 5800 3500 5800
Wire Wire Line
	3500 5800 3500 6000
$Comp
L Device:R_Small R4
U 1 1 5D1A2002
P 3500 5550
F 0 "R4" H 3441 5504 50  0000 R CNN
F 1 "1kΩ" H 3441 5595 50  0000 R CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 3500 5550 50  0001 C CNN
F 3 "~" H 3500 5550 50  0001 C CNN
	1    3500 5550
	-1   0    0    1   
$EndComp
Wire Wire Line
	3500 5250 3500 5450
Wire Wire Line
	3500 5650 3500 5800
Connection ~ 3500 5800
Text GLabel 3650 6000 2    50   Input ~ 0
RXD
Wire Wire Line
	3500 6000 3650 6000
Connection ~ 3500 6000
Wire Wire Line
	3500 6000 3500 6300
Text GLabel 2800 3700 2    50   Input ~ 0
RXD
$Comp
L Switch:SW_Push SW1
U 1 1 5D1A2C32
P 2150 1750
F 0 "SW1" V 2196 1702 50  0000 R CNN
F 1 "SW_Push" V 2105 1702 50  0000 R CNN
F 2 "Button_Switch_THT:SW_PUSH_6mm" H 2150 1950 50  0001 C CNN
F 3 "" H 2150 1950 50  0001 C CNN
	1    2150 1750
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR012
U 1 1 5D1A36ED
P 2150 2150
F 0 "#PWR012" H 2150 1900 50  0001 C CNN
F 1 "GND" H 2155 1977 50  0000 C CNN
F 2 "" H 2150 2150 50  0001 C CNN
F 3 "" H 2150 2150 50  0001 C CNN
	1    2150 2150
	1    0    0    -1  
$EndComp
Wire Wire Line
	2150 1950 2150 2150
$Comp
L power:+5V #PWR05
U 1 1 5D1A3FD1
P 1900 5750
F 0 "#PWR05" H 1900 5600 50  0001 C CNN
F 1 "+5V" H 1915 5923 50  0000 C CNN
F 2 "" H 1900 5750 50  0001 C CNN
F 3 "" H 1900 5750 50  0001 C CNN
	1    1900 5750
	1    0    0    -1  
$EndComp
Wire Wire Line
	1900 5750 1900 6000
Text GLabel 2350 1450 2    50   Input ~ 0
RST
Wire Wire Line
	2150 1450 2350 1450
Wire Wire Line
	2150 1450 2150 1550
$Comp
L power:GND #PWR0101
U 1 1 5D1A54D7
P 1300 3800
F 0 "#PWR0101" H 1300 3550 50  0001 C CNN
F 1 "GND" H 1305 3627 50  0000 C CNN
F 2 "" H 1300 3800 50  0001 C CNN
F 3 "" H 1300 3800 50  0001 C CNN
	1    1300 3800
	1    0    0    -1  
$EndComp
Wire Wire Line
	1800 3600 1300 3600
Wire Wire Line
	1300 3600 1300 3800
NoConn ~ 1800 4100
NoConn ~ 1800 4200
NoConn ~ 2800 4200
NoConn ~ 2800 4100
NoConn ~ 2800 4000
$Comp
L power:PWR_FLAG #FLG0101
U 1 1 5D1A95AD
P 1300 3600
F 0 "#FLG0101" H 1300 3675 50  0001 C CNN
F 1 "PWR_FLAG" V 1300 3728 50  0000 L CNN
F 2 "" H 1300 3600 50  0001 C CNN
F 3 "~" H 1300 3600 50  0001 C CNN
	1    1300 3600
	0    -1   -1   0   
$EndComp
Connection ~ 1300 3600
$Comp
L power:PWR_FLAG #FLG0102
U 1 1 5D1A95FB
P 1550 3500
F 0 "#FLG0102" H 1550 3575 50  0001 C CNN
F 1 "PWR_FLAG" V 1550 3628 50  0000 L CNN
F 2 "" H 1550 3500 50  0001 C CNN
F 3 "~" H 1550 3500 50  0001 C CNN
	1    1550 3500
	0    -1   -1   0   
$EndComp
Connection ~ 1550 3500
Wire Notes Line
	550  7700 4050 7700
Text Notes 3200 4700 0    50   ~ 0
P1 PORT INTERFACE
Wire Notes Line
	4050 600  4050 7700
Wire Notes Line
	550  600  550  7700
Text Notes 3250 2900 0    50   ~ 0
MICROCONTROLLER
Wire Wire Line
	850  6200 1050 6200
$Comp
L Device:R_Small R9
U 1 1 5D1CCB87
P 1050 6000
F 0 "R9" H 1200 5950 50  0000 R CNN
F 1 "1kΩ" H 1250 6050 50  0000 R CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 1050 6000 50  0001 C CNN
F 3 "~" H 1050 6000 50  0001 C CNN
	1    1050 6000
	-1   0    0    1   
$EndComp
Wire Wire Line
	1050 5800 1050 5900
Wire Wire Line
	1050 6100 1050 6200
Connection ~ 1050 6200
Wire Wire Line
	1050 6200 1200 6200
Text Notes 3750 750  0    50   ~ 0
RESET
Text Notes 800  1550 0    50   ~ 0
Reset the configuration\nsettings by holding this\nbutton during system startup.
Text Notes 2100 6100 0    50   ~ 0
<--- BUFFER
Text Notes 2650 6700 0    50   ~ 0
INVERTER --->
Text Notes 4100 750  0    50   ~ 0
STATUS LED
$Comp
L Device:LED_RAGB D3
U 1 1 5D5D69FB
P 5100 1950
F 0 "D3" H 5100 2447 50  0000 C CNN
F 1 "LED_RAGB" H 5100 2356 50  0000 C CNN
F 2 "LED_THT:LED_D5.0mm-4_RGB_Wide_Pins" H 5100 1900 50  0001 C CNN
F 3 "~" H 5100 1900 50  0001 C CNN
	1    5100 1950
	1    0    0    -1  
$EndComp
Wire Wire Line
	5300 1950 5650 1950
$Comp
L power:+3.3V #PWR015
U 1 1 5D5D7508
P 5650 1550
F 0 "#PWR015" H 5650 1400 50  0001 C CNN
F 1 "+3.3V" H 5665 1723 50  0000 C CNN
F 2 "" H 5650 1550 50  0001 C CNN
F 3 "" H 5650 1550 50  0001 C CNN
	1    5650 1550
	1    0    0    -1  
$EndComp
Text GLabel 2800 3600 2    50   Input ~ 0
LED_R
Text GLabel 1800 4000 0    50   Input ~ 0
LED_G
Text GLabel 2800 3900 2    50   Input ~ 0
LED_B
Text GLabel 4500 2150 0    50   Input ~ 0
LED_B
Text GLabel 4500 1750 0    50   Input ~ 0
LED_R
Text GLabel 4500 1950 0    50   Input ~ 0
LED_G
Wire Notes Line
	550  2750 6350 2750
$Comp
L Connector:Conn_01x06_Female FTDI1
U 1 1 5D5DA3CA
P 5300 3500
F 0 "FTDI1" H 5328 3476 50  0000 L CNN
F 1 "FTDI" H 5328 3385 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical" H 5300 3500 50  0001 C CNN
F 3 "~" H 5300 3500 50  0001 C CNN
	1    5300 3500
	1    0    0    -1  
$EndComp
NoConn ~ 5100 3400
NoConn ~ 5100 3500
NoConn ~ 5100 3600
Text GLabel 1800 3700 0    50   Input ~ 0
RST
Text GLabel 1800 3900 0    50   Input ~ 0
ESP_TX1
Text GLabel 5100 3700 0    50   Input ~ 0
ESP_TX1
$Comp
L power:GND #PWR013
U 1 1 5D5DD6F4
P 4800 3350
F 0 "#PWR013" H 4800 3100 50  0001 C CNN
F 1 "GND" H 4805 3177 50  0000 C CNN
F 2 "" H 4800 3350 50  0001 C CNN
F 3 "" H 4800 3350 50  0001 C CNN
	1    4800 3350
	1    0    0    -1  
$EndComp
Wire Wire Line
	5100 3300 4800 3300
Wire Wire Line
	4800 3300 4800 3350
NoConn ~ 5100 3800
Wire Notes Line
	550  4550 6350 4550
Text Notes 6100 2900 0    50   ~ 0
FTDI
Wire Notes Line
	4050 5750 6350 5750
Wire Notes Line
	6350 600  6350 5750
Text Notes 5700 4700 0    50   ~ 0
SERIAL SELECT
Text Notes 7450 1950 0    50   ~ 0
RJ11 CONNECTOR
Wire Notes Line
	550  600  6350 600 
Wire Notes Line
	6350 3600 8300 3600
Wire Notes Line
	8300 3600 8300 1800
Wire Notes Line
	8300 1800 6350 1800
$Comp
L Device:R_Small R6
U 1 1 5D67F26C
P 4700 1750
F 0 "R6" V 4850 1750 50  0000 L CNN
F 1 "100Ω" V 4759 1705 50  0000 L CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 4700 1750 50  0001 C CNN
F 3 "~" H 4700 1750 50  0001 C CNN
	1    4700 1750
	0    -1   -1   0   
$EndComp
$Comp
L Device:R_Small R7
U 1 1 5D67F2E9
P 4700 1950
F 0 "R7" V 4750 1800 50  0000 L CNN
F 1 "100Ω" V 4759 1905 50  0000 L CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 4700 1950 50  0001 C CNN
F 3 "~" H 4700 1950 50  0001 C CNN
	1    4700 1950
	0    -1   -1   0   
$EndComp
$Comp
L Device:R_Small R8
U 1 1 5D67F321
P 4700 2150
F 0 "R8" V 4600 2100 50  0000 L CNN
F 1 "100Ω" V 4550 2050 50  0000 L CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 4700 2150 50  0001 C CNN
F 3 "~" H 4700 2150 50  0001 C CNN
	1    4700 2150
	0    -1   -1   0   
$EndComp
Wire Wire Line
	5650 1550 5650 1950
Wire Wire Line
	4900 1750 4800 1750
Wire Wire Line
	4900 1950 4800 1950
Wire Wire Line
	4900 2150 4800 2150
Wire Wire Line
	4600 2150 4500 2150
Wire Wire Line
	4500 1950 4600 1950
Wire Wire Line
	4600 1750 4500 1750
NoConn ~ 1800 3800
NoConn ~ 2800 3800
$EndSCHEMATC
