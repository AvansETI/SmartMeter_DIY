EESchema Schematic File Version 4
LIBS:SmartMeter-cache
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Smart Meter"
Date "2019-07-02"
Rev "v1.0"
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
P 1100 5200
F 0 "J1" H 770 5204 50  0000 R CNN
F 1 "RJ12" H 770 5295 50  0000 R CNN
F 2 "Connector_RJ:RJ12_Amphenol_54601" V 1100 5225 50  0001 C CNN
F 3 "~" V 1100 5225 50  0001 C CNN
	1    1100 5200
	1    0    0    1   
$EndComp
NoConn ~ 1500 5000
$Comp
L power:GND #PWR04
U 1 1 5D1A015D
P 2000 5400
F 0 "#PWR04" H 2000 5150 50  0001 C CNN
F 1 "GND" H 2005 5227 50  0000 C CNN
F 2 "" H 2000 5400 50  0001 C CNN
F 3 "" H 2000 5400 50  0001 C CNN
	1    2000 5400
	1    0    0    -1  
$EndComp
Wire Wire Line
	1500 5200 2000 5200
Wire Wire Line
	2000 5200 2000 5400
$Comp
L power:+5V #PWR03
U 1 1 5D1A01B1
P 2000 4950
F 0 "#PWR03" H 2000 4800 50  0001 C CNN
F 1 "+5V" H 2015 5123 50  0000 C CNN
F 2 "" H 2000 4950 50  0001 C CNN
F 3 "" H 2000 4950 50  0001 C CNN
	1    2000 4950
	1    0    0    -1  
$EndComp
Wire Wire Line
	1500 5100 2000 5100
Wire Wire Line
	2000 5100 2000 4950
Text GLabel 1500 5400 2    50   Input ~ 0
TXD
NoConn ~ 1500 5300
NoConn ~ 1500 5500
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
P 1800 6700
F 0 "Q1" H 1991 6746 50  0000 L CNN
F 1 "BC547" H 1991 6655 50  0000 L CNN
F 2 "Package_TO_SOT_THT:TO-92_Inline" H 2000 6625 50  0001 L CIN
F 3 "http://www.fairchildsemi.com/ds/BC/BC547.pdf" H 1800 6700 50  0001 L CNN
	1    1800 6700
	1    0    0    -1  
$EndComp
Text GLabel 850  6700 0    50   Input ~ 0
TXD
$Comp
L power:+5V #PWR01
U 1 1 5D1A0651
P 1050 6300
F 0 "#PWR01" H 1050 6150 50  0001 C CNN
F 1 "+5V" H 1065 6473 50  0000 C CNN
F 2 "" H 1050 6300 50  0001 C CNN
F 3 "" H 1050 6300 50  0001 C CNN
	1    1050 6300
	1    0    0    -1  
$EndComp
$Comp
L Device:R_Small R1
U 1 1 5D1A06D2
P 1300 6700
F 0 "R1" V 1104 6700 50  0000 C CNN
F 1 "1kΩ" V 1195 6700 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 1300 6700 50  0001 C CNN
F 3 "~" H 1300 6700 50  0001 C CNN
	1    1300 6700
	0    1    1    0   
$EndComp
Wire Wire Line
	1400 6700 1500 6700
$Comp
L Diode:1N4148 D1
U 1 1 5D1A08CB
P 1500 7050
F 0 "D1" V 1450 6800 50  0000 L CNN
F 1 "1N4148" V 1550 6650 50  0000 L CNN
F 2 "Diode_THT:D_DO-35_SOD27_P7.62mm_Horizontal" H 1500 6875 50  0001 C CNN
F 3 "http://www.nxp.com/documents/data_sheet/1N4148_1N4448.pdf" H 1500 7050 50  0001 C CNN
	1    1500 7050
	0    1    1    0   
$EndComp
Wire Wire Line
	1500 6700 1500 6900
Connection ~ 1500 6700
Wire Wire Line
	1500 6700 1600 6700
$Comp
L power:GND #PWR02
U 1 1 5D1A09EE
P 1500 7350
F 0 "#PWR02" H 1500 7100 50  0001 C CNN
F 1 "GND" H 1505 7177 50  0000 C CNN
F 2 "" H 1500 7350 50  0001 C CNN
F 3 "" H 1500 7350 50  0001 C CNN
	1    1500 7350
	1    0    0    -1  
$EndComp
Wire Wire Line
	1500 7200 1500 7350
$Comp
L Device:R_Small R2
U 1 1 5D1A0B02
P 1900 7150
F 0 "R2" H 1841 7104 50  0000 R CNN
F 1 "1kΩ" H 1841 7195 50  0000 R CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 1900 7150 50  0001 C CNN
F 3 "~" H 1900 7150 50  0001 C CNN
	1    1900 7150
	-1   0    0    1   
$EndComp
Wire Wire Line
	1900 6900 1900 7000
$Comp
L power:GND #PWR06
U 1 1 5D1A0C99
P 1900 7350
F 0 "#PWR06" H 1900 7100 50  0001 C CNN
F 1 "GND" H 1905 7177 50  0000 C CNN
F 2 "" H 1900 7350 50  0001 C CNN
F 3 "" H 1900 7350 50  0001 C CNN
	1    1900 7350
	1    0    0    -1  
$EndComp
Wire Wire Line
	1900 7250 1900 7350
$Comp
L Device:R_Small R3
U 1 1 5D1A0DFA
P 2600 7000
F 0 "R3" V 2404 7000 50  0000 C CNN
F 1 "1kΩ" V 2495 7000 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 2600 7000 50  0001 C CNN
F 3 "~" H 2600 7000 50  0001 C CNN
	1    2600 7000
	0    1    1    0   
$EndComp
Wire Wire Line
	1900 7000 2500 7000
Connection ~ 1900 7000
Wire Wire Line
	1900 7000 1900 7050
$Comp
L Diode:1N4148 D2
U 1 1 5D1A0F8D
P 2950 6650
F 0 "D2" V 2900 6800 50  0000 L CNN
F 1 "1N4148" V 3000 6800 50  0000 L CNN
F 2 "Diode_THT:D_DO-35_SOD27_P7.62mm_Horizontal" H 2950 6475 50  0001 C CNN
F 3 "http://www.nxp.com/documents/data_sheet/1N4148_1N4448.pdf" H 2950 6650 50  0001 C CNN
	1    2950 6650
	0    1    1    0   
$EndComp
$Comp
L Transistor_BJT:BC547 Q2
U 1 1 5D1A1035
P 3400 7000
F 0 "Q2" H 3591 7046 50  0000 L CNN
F 1 "BC547" H 3591 6955 50  0000 L CNN
F 2 "Package_TO_SOT_THT:TO-92_Inline" H 3600 6925 50  0001 L CIN
F 3 "http://www.fairchildsemi.com/ds/BC/BC547.pdf" H 3400 7000 50  0001 L CNN
	1    3400 7000
	1    0    0    -1  
$EndComp
Wire Wire Line
	2700 7000 2950 7000
Wire Wire Line
	2950 6800 2950 7000
Connection ~ 2950 7000
Wire Wire Line
	2950 7000 3200 7000
$Comp
L power:+3.3V #PWR08
U 1 1 5D1A140E
P 3500 5750
F 0 "#PWR08" H 3500 5600 50  0001 C CNN
F 1 "+3.3V" H 3515 5923 50  0000 C CNN
F 2 "" H 3500 5750 50  0001 C CNN
F 3 "" H 3500 5750 50  0001 C CNN
	1    3500 5750
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR09
U 1 1 5D1A170A
P 3500 7300
F 0 "#PWR09" H 3500 7050 50  0001 C CNN
F 1 "GND" H 3505 7127 50  0000 C CNN
F 2 "" H 3500 7300 50  0001 C CNN
F 3 "" H 3500 7300 50  0001 C CNN
	1    3500 7300
	1    0    0    -1  
$EndComp
Wire Wire Line
	3500 7200 3500 7300
Wire Wire Line
	2950 6500 2950 6300
Wire Wire Line
	2950 6300 3500 6300
Wire Wire Line
	3500 6300 3500 6500
$Comp
L Device:R_Small R4
U 1 1 5D1A2002
P 3500 6050
F 0 "R4" H 3441 6004 50  0000 R CNN
F 1 "1kΩ" H 3441 6095 50  0000 R CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 3500 6050 50  0001 C CNN
F 3 "~" H 3500 6050 50  0001 C CNN
	1    3500 6050
	-1   0    0    1   
$EndComp
Wire Wire Line
	3500 5750 3500 5950
Wire Wire Line
	3500 6150 3500 6300
Connection ~ 3500 6300
Text GLabel 3650 6500 2    50   Input ~ 0
RXD
Wire Wire Line
	3500 6500 3650 6500
Connection ~ 3500 6500
Wire Wire Line
	3500 6500 3500 6800
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
L Device:R_Small R5
U 1 1 5D1A2DA6
P 2150 1250
F 0 "R5" H 2091 1204 50  0000 R CNN
F 1 "1kΩ" H 2091 1295 50  0000 R CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 2150 1250 50  0001 C CNN
F 3 "~" H 2150 1250 50  0001 C CNN
	1    2150 1250
	-1   0    0    1   
$EndComp
$Comp
L power:+3.3V #PWR011
U 1 1 5D1A2E40
P 2150 1050
F 0 "#PWR011" H 2150 900 50  0001 C CNN
F 1 "+3.3V" H 2165 1223 50  0000 C CNN
F 2 "" H 2150 1050 50  0001 C CNN
F 3 "" H 2150 1050 50  0001 C CNN
	1    2150 1050
	1    0    0    -1  
$EndComp
Wire Wire Line
	2150 1050 2150 1150
Wire Wire Line
	2150 1350 2150 1450
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
P 1900 6250
F 0 "#PWR05" H 1900 6100 50  0001 C CNN
F 1 "+5V" H 1915 6423 50  0000 C CNN
F 2 "" H 1900 6250 50  0001 C CNN
F 3 "" H 1900 6250 50  0001 C CNN
	1    1900 6250
	1    0    0    -1  
$EndComp
Wire Wire Line
	1900 6250 1900 6500
Text GLabel 2350 1450 2    50   Input ~ 0
RST
Wire Wire Line
	2150 1450 2350 1450
Connection ~ 2150 1450
Wire Wire Line
	2150 1450 2150 1550
Text GLabel 1800 3900 0    50   Input ~ 0
RST
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
NoConn ~ 1800 3700
NoConn ~ 1800 3800
NoConn ~ 1800 4100
NoConn ~ 1800 4200
NoConn ~ 2800 4200
NoConn ~ 2800 4100
NoConn ~ 2800 4000
NoConn ~ 2800 3900
NoConn ~ 2800 3800
NoConn ~ 2800 3600
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
	4050 4550 550  4550
Wire Notes Line
	550  7700 4050 7700
Text Notes 3200 4700 0    50   ~ 0
P1 PORT INTERFACE
Wire Notes Line
	4050 2750 550  2750
Wire Notes Line
	4050 600  4050 7700
Wire Notes Line
	550  600  550  7700
$Comp
L Timer:TLC555CP U2
U 1 1 5D1B0EA6
P 6450 1750
F 0 "U2" H 6700 2250 50  0000 C CNN
F 1 "TLC555CP" H 6700 2150 50  0000 C CNN
F 2 "Package_DIP:DIP-8_W7.62mm" H 6450 1750 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/tlc555.pdf" H 6450 1750 50  0001 C CNN
	1    6450 1750
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR017
U 1 1 5D1B115B
P 6450 1050
F 0 "#PWR017" H 6450 900 50  0001 C CNN
F 1 "+3.3V" H 6465 1223 50  0000 C CNN
F 2 "" H 6450 1050 50  0001 C CNN
F 3 "" H 6450 1050 50  0001 C CNN
	1    6450 1050
	1    0    0    -1  
$EndComp
Wire Wire Line
	6450 1350 6450 1150
$Comp
L Device:C_Small C2
U 1 1 5D1B1829
P 5700 2250
F 0 "C2" H 5792 2296 50  0000 L CNN
F 1 "100nF" H 5792 2205 50  0000 L CNN
F 2 "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P2.50mm" H 5700 2250 50  0001 C CNN
F 3 "~" H 5700 2250 50  0001 C CNN
	1    5700 2250
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C1
U 1 1 5D1B1885
P 5400 2250
F 0 "C1" H 5200 2300 50  0000 L CNN
F 1 "100nF" H 5100 2200 50  0000 L CNN
F 2 "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P2.50mm" H 5400 2250 50  0001 C CNN
F 3 "~" H 5400 2250 50  0001 C CNN
	1    5400 2250
	1    0    0    -1  
$EndComp
Wire Wire Line
	5950 1900 5700 1900
Wire Wire Line
	5700 1900 5700 2150
$Comp
L power:GND #PWR016
U 1 1 5D1B204B
P 5700 2450
F 0 "#PWR016" H 5700 2200 50  0001 C CNN
F 1 "GND" H 5705 2277 50  0000 C CNN
F 2 "" H 5700 2450 50  0001 C CNN
F 3 "" H 5700 2450 50  0001 C CNN
	1    5700 2450
	1    0    0    -1  
$EndComp
Wire Wire Line
	5700 2350 5700 2450
Wire Wire Line
	5950 1600 5700 1600
Wire Wire Line
	5700 1600 5700 1900
Connection ~ 5700 1900
$Comp
L power:+3.3V #PWR014
U 1 1 5D1B2F31
P 5100 1050
F 0 "#PWR014" H 5100 900 50  0001 C CNN
F 1 "+3.3V" H 5115 1223 50  0000 C CNN
F 2 "" H 5100 1050 50  0001 C CNN
F 3 "" H 5100 1050 50  0001 C CNN
	1    5100 1050
	1    0    0    -1  
$EndComp
Wire Wire Line
	5100 1350 5100 1050
$Comp
L Device:R_PHOTO R7
U 1 1 5D1B3815
P 4850 2100
F 0 "R7" H 4550 2200 50  0000 L CNN
F 1 "R_PHOTO" H 4350 2050 50  0000 L CNN
F 2 "OptoDevice:R_LDR_7x6mm_P5.1mm_Vertical" V 4900 1850 50  0001 L CNN
F 3 "~" H 4850 2050 50  0001 C CNN
	1    4850 2100
	1    0    0    -1  
$EndComp
Wire Wire Line
	4850 1950 4850 1900
Wire Wire Line
	4850 2250 4850 2450
Wire Wire Line
	5400 2350 5400 2450
$Comp
L power:GND #PWR015
U 1 1 5D1B55CC
P 5400 2450
F 0 "#PWR015" H 5400 2200 50  0001 C CNN
F 1 "GND" H 5405 2277 50  0000 C CNN
F 2 "" H 5400 2450 50  0001 C CNN
F 3 "" H 5400 2450 50  0001 C CNN
	1    5400 2450
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR013
U 1 1 5D1B55F7
P 4850 2450
F 0 "#PWR013" H 4850 2200 50  0001 C CNN
F 1 "GND" H 4855 2277 50  0000 C CNN
F 2 "" H 4850 2450 50  0001 C CNN
F 3 "" H 4850 2450 50  0001 C CNN
	1    4850 2450
	1    0    0    -1  
$EndComp
Wire Wire Line
	5700 1900 4850 1900
Connection ~ 4850 1900
Wire Wire Line
	4850 1900 4850 1750
$Comp
L Device:R_Small R6
U 1 1 5D1B761A
P 4850 1650
F 0 "R6" H 4909 1696 50  0000 L CNN
F 1 "100kΩ" H 4909 1605 50  0000 L CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 4850 1650 50  0001 C CNN
F 3 "~" H 4850 1650 50  0001 C CNN
	1    4850 1650
	1    0    0    -1  
$EndComp
Wire Wire Line
	4850 1550 4850 1350
Wire Wire Line
	4850 1350 5100 1350
Wire Wire Line
	5100 1350 5400 1350
Wire Wire Line
	5400 1350 5400 2150
Connection ~ 5100 1350
Text GLabel 1800 4000 0    50   Input ~ 0
TRIGGER
Text GLabel 7450 1550 2    50   Input ~ 0
TRIGGER
$Comp
L Device:LED D3
U 1 1 5D1BCDBC
P 7200 2150
F 0 "D3" V 7238 2033 50  0000 R CNN
F 1 "LED" V 7147 2033 50  0000 R CNN
F 2 "LED_THT:LED_D5.0mm" H 7200 2150 50  0001 C CNN
F 3 "~" H 7200 2150 50  0001 C CNN
	1    7200 2150
	0    -1   -1   0   
$EndComp
$Comp
L Device:R_Small R8
U 1 1 5D1BD9CA
P 7200 1750
F 0 "R8" H 7259 1796 50  0000 L CNN
F 1 "270Ω" H 7259 1705 50  0000 L CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 7200 1750 50  0001 C CNN
F 3 "~" H 7200 1750 50  0001 C CNN
	1    7200 1750
	1    0    0    -1  
$EndComp
Wire Wire Line
	6950 1550 7200 1550
Wire Wire Line
	7200 1550 7200 1650
Wire Wire Line
	7200 1550 7450 1550
Connection ~ 7200 1550
Wire Wire Line
	7200 1850 7200 2000
$Comp
L power:GND #PWR018
U 1 1 5D1BFE50
P 6450 2300
F 0 "#PWR018" H 6450 2050 50  0001 C CNN
F 1 "GND" H 6455 2127 50  0000 C CNN
F 2 "" H 6450 2300 50  0001 C CNN
F 3 "" H 6450 2300 50  0001 C CNN
	1    6450 2300
	1    0    0    -1  
$EndComp
Wire Wire Line
	6450 2150 6450 2300
$Comp
L power:GND #PWR019
U 1 1 5D1C0B63
P 7200 2450
F 0 "#PWR019" H 7200 2200 50  0001 C CNN
F 1 "GND" H 7205 2277 50  0000 C CNN
F 2 "" H 7200 2450 50  0001 C CNN
F 3 "" H 7200 2450 50  0001 C CNN
	1    7200 2450
	1    0    0    -1  
$EndComp
Wire Wire Line
	7200 2300 7200 2450
Wire Wire Line
	6950 1750 7050 1750
Wire Wire Line
	7050 1750 7050 1150
Wire Wire Line
	7050 1150 6450 1150
Connection ~ 6450 1150
Wire Wire Line
	6450 1150 6450 1050
NoConn ~ 6950 1900
NoConn ~ 5950 1750
Wire Notes Line
	8300 600  8300 3350
Wire Notes Line
	8300 3350 4050 3350
Wire Notes Line
	550  600  8300 600 
Text Notes 3250 2900 0    50   ~ 0
MICROCONTROLLER
Text Notes 7250 750  0    50   ~ 0
PHOTO RESISTIVE TRIGGER
Wire Wire Line
	850  6700 1050 6700
$Comp
L Device:R_Small R9
U 1 1 5D1CCB87
P 1050 6500
F 0 "R9" H 1200 6450 50  0000 R CNN
F 1 "1kΩ" H 1250 6550 50  0000 R CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 1050 6500 50  0001 C CNN
F 3 "~" H 1050 6500 50  0001 C CNN
	1    1050 6500
	-1   0    0    1   
$EndComp
Wire Wire Line
	1050 6300 1050 6400
Wire Wire Line
	1050 6600 1050 6700
Connection ~ 1050 6700
Wire Wire Line
	1050 6700 1200 6700
Text Notes 3750 750  0    50   ~ 0
RESET
Text Notes 7550 2300 0    50   ~ 0
This LED is\ntriggered every\npulse.
Text Notes 800  1550 0    50   ~ 0
Reset the configuration\nsettings by holding this\nbutton during system startup.
Text Notes 2100 6600 0    50   ~ 0
<--- BUFFER
Text Notes 2650 7200 0    50   ~ 0
INVERTER --->
$EndSCHEMATC
