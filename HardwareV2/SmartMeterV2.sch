EESchema Schematic File Version 4
LIBS:SmartMeterV2-cache
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "DIY Smart Meter Interface for DSMR P1 port"
Date "2020-02-02"
Rev "v2.1"
Comp "Avans Hogeschool Breda"
Comment1 "For more info: dm.kroeske@avans.nl"
Comment2 "Expertisecentrum Technische Innovatie"
Comment3 "Lectoraat Smart Energy"
Comment4 ""
$EndDescr
$Comp
L Connector:RJ12 J1
U 1 1 5D19FFB2
P 1600 4700
F 0 "J1" H 1270 4704 50  0000 R CNN
F 1 "RJ12" H 1270 4795 50  0000 R CNN
F 2 "Connector_RJ:RJ12_Amphenol_54601" V 1600 4725 50  0001 C CNN
F 3 "~" V 1600 4725 50  0001 C CNN
	1    1600 4700
	1    0    0    1   
$EndComp
Text GLabel 2100 4900 2    50   Input ~ 0
SM_TxD
$Comp
L power:+3.3V #PWR010
U 1 1 5D1A03CB
P 10050 4300
F 0 "#PWR010" H 10050 4150 50  0001 C CNN
F 1 "+3.3V" H 10065 4473 50  0000 C CNN
F 2 "" H 10050 4300 50  0001 C CNN
F 3 "" H 10050 4300 50  0001 C CNN
	1    10050 4300
	1    0    0    -1  
$EndComp
Wire Wire Line
	9750 4500 10050 4500
Wire Wire Line
	10050 4500 10050 4300
$Comp
L Transistor_BJT:BC547 Q1
U 1 1 5D1A04B1
P 4800 4650
F 0 "Q1" H 4991 4696 50  0000 L CNN
F 1 "BC547" H 4991 4605 50  0000 L CNN
F 2 "Package_TO_SOT_THT:TO-92_Wide" H 5000 4575 50  0001 L CIN
F 3 "http://www.fairchildsemi.com/ds/BC/BC547.pdf" H 4800 4650 50  0001 L CNN
	1    4800 4650
	1    0    0    -1  
$EndComp
Text GLabel 3850 4650 0    50   Input ~ 0
SM_TxD
$Comp
L power:+5V #PWR01
U 1 1 5D1A0651
P 4050 4200
F 0 "#PWR01" H 4050 4050 50  0001 C CNN
F 1 "+5V" H 4065 4373 50  0000 C CNN
F 2 "" H 4050 4200 50  0001 C CNN
F 3 "" H 4050 4200 50  0001 C CNN
	1    4050 4200
	1    0    0    -1  
$EndComp
$Comp
L Device:R_Small R1
U 1 1 5D1A06D2
P 4300 4650
F 0 "R1" V 4104 4650 50  0000 C CNN
F 1 "1kΩ" V 4195 4650 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 4300 4650 50  0001 C CNN
F 3 "~" H 4300 4650 50  0001 C CNN
	1    4300 4650
	0    1    1    0   
$EndComp
Wire Wire Line
	4400 4650 4500 4650
$Comp
L Diode:1N4148 D1
U 1 1 5D1A08CB
P 4500 5000
F 0 "D1" V 4450 4750 50  0000 L CNN
F 1 "1N4148" V 4550 4600 50  0000 L CNN
F 2 "Diode_THT:D_DO-35_SOD27_P7.62mm_Horizontal" H 4500 4825 50  0001 C CNN
F 3 "http://www.nxp.com/documents/data_sheet/1N4148_1N4448.pdf" H 4500 5000 50  0001 C CNN
	1    4500 5000
	0    1    1    0   
$EndComp
Wire Wire Line
	4500 4650 4500 4850
Connection ~ 4500 4650
Wire Wire Line
	4500 4650 4600 4650
$Comp
L power:GND #PWR02
U 1 1 5D1A09EE
P 4500 5300
F 0 "#PWR02" H 4500 5050 50  0001 C CNN
F 1 "GND" H 4505 5127 50  0000 C CNN
F 2 "" H 4500 5300 50  0001 C CNN
F 3 "" H 4500 5300 50  0001 C CNN
	1    4500 5300
	1    0    0    -1  
$EndComp
$Comp
L Device:R_Small R2
U 1 1 5D1A0B02
P 4900 5100
F 0 "R2" H 4841 5054 50  0000 R CNN
F 1 "1kΩ" H 4841 5145 50  0000 R CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 4900 5100 50  0001 C CNN
F 3 "~" H 4900 5100 50  0001 C CNN
	1    4900 5100
	-1   0    0    1   
$EndComp
Wire Wire Line
	4900 4850 4900 4950
$Comp
L power:GND #PWR06
U 1 1 5D1A0C99
P 4900 5300
F 0 "#PWR06" H 4900 5050 50  0001 C CNN
F 1 "GND" H 4905 5127 50  0000 C CNN
F 2 "" H 4900 5300 50  0001 C CNN
F 3 "" H 4900 5300 50  0001 C CNN
	1    4900 5300
	1    0    0    -1  
$EndComp
Wire Wire Line
	4900 5200 4900 5300
$Comp
L Device:R_Small R3
U 1 1 5D1A0DFA
P 5600 4950
F 0 "R3" V 5404 4950 50  0000 C CNN
F 1 "1kΩ" V 5495 4950 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 5600 4950 50  0001 C CNN
F 3 "~" H 5600 4950 50  0001 C CNN
	1    5600 4950
	0    1    1    0   
$EndComp
Wire Wire Line
	4900 4950 5500 4950
Connection ~ 4900 4950
Wire Wire Line
	4900 4950 4900 5000
$Comp
L Diode:1N4148 D2
U 1 1 5D1A0F8D
P 5950 4600
F 0 "D2" V 5900 4750 50  0000 L CNN
F 1 "1N4148" V 6000 4750 50  0000 L CNN
F 2 "Diode_THT:D_DO-35_SOD27_P7.62mm_Horizontal" H 5950 4425 50  0001 C CNN
F 3 "http://www.nxp.com/documents/data_sheet/1N4148_1N4448.pdf" H 5950 4600 50  0001 C CNN
	1    5950 4600
	0    1    1    0   
$EndComp
$Comp
L Transistor_BJT:BC547 Q2
U 1 1 5D1A1035
P 6400 4950
F 0 "Q2" H 6591 4996 50  0000 L CNN
F 1 "BC547" H 6591 4905 50  0000 L CNN
F 2 "Package_TO_SOT_THT:TO-92_Wide" H 6600 4875 50  0001 L CIN
F 3 "http://www.fairchildsemi.com/ds/BC/BC547.pdf" H 6400 4950 50  0001 L CNN
	1    6400 4950
	1    0    0    -1  
$EndComp
Wire Wire Line
	5700 4950 5950 4950
Wire Wire Line
	5950 4750 5950 4950
Connection ~ 5950 4950
Wire Wire Line
	5950 4950 6200 4950
$Comp
L power:+3.3V #PWR08
U 1 1 5D1A140E
P 6500 3700
F 0 "#PWR08" H 6500 3550 50  0001 C CNN
F 1 "+3.3V" H 6515 3873 50  0000 C CNN
F 2 "" H 6500 3700 50  0001 C CNN
F 3 "" H 6500 3700 50  0001 C CNN
	1    6500 3700
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR09
U 1 1 5D1A170A
P 6500 5250
F 0 "#PWR09" H 6500 5000 50  0001 C CNN
F 1 "GND" H 6505 5077 50  0000 C CNN
F 2 "" H 6500 5250 50  0001 C CNN
F 3 "" H 6500 5250 50  0001 C CNN
	1    6500 5250
	1    0    0    -1  
$EndComp
Wire Wire Line
	6500 5150 6500 5250
Wire Wire Line
	5950 4450 5950 4250
Wire Wire Line
	5950 4250 6500 4250
Wire Wire Line
	6500 4250 6500 4450
$Comp
L Device:R_Small R4
U 1 1 5D1A2002
P 6500 4000
F 0 "R4" H 6441 3954 50  0000 R CNN
F 1 "1kΩ" H 6441 4045 50  0000 R CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 6500 4000 50  0001 C CNN
F 3 "~" H 6500 4000 50  0001 C CNN
	1    6500 4000
	-1   0    0    1   
$EndComp
Wire Wire Line
	6500 3700 6500 3900
Wire Wire Line
	6500 4100 6500 4250
Connection ~ 6500 4250
Text GLabel 6650 4450 2    50   Input ~ 0
RxD
Wire Wire Line
	6500 4450 6650 4450
Connection ~ 6500 4450
Wire Wire Line
	6500 4450 6500 4750
Text GLabel 9800 4700 2    50   Input ~ 0
RxD
$Comp
L Switch:SW_Push SW1
U 1 1 5D1A2C32
P 2300 1950
F 0 "SW1" V 2346 1902 50  0000 R CNN
F 1 "SW_Push" V 2255 1902 50  0000 R CNN
F 2 "Button_Switch_THT:SW_PUSH_6mm" H 2300 2150 50  0001 C CNN
F 3 "" H 2300 2150 50  0001 C CNN
	1    2300 1950
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR012
U 1 1 5D1A36ED
P 2300 2350
F 0 "#PWR012" H 2300 2100 50  0001 C CNN
F 1 "GND" H 2305 2177 50  0000 C CNN
F 2 "" H 2300 2350 50  0001 C CNN
F 3 "" H 2300 2350 50  0001 C CNN
	1    2300 2350
	1    0    0    -1  
$EndComp
Wire Wire Line
	2300 2150 2300 2350
$Comp
L power:+5V #PWR05
U 1 1 5D1A3FD1
P 4900 4200
F 0 "#PWR05" H 4900 4050 50  0001 C CNN
F 1 "+5V" H 4915 4373 50  0000 C CNN
F 2 "" H 4900 4200 50  0001 C CNN
F 3 "" H 4900 4200 50  0001 C CNN
	1    4900 4200
	1    0    0    -1  
$EndComp
Text GLabel 2500 1650 2    50   Input ~ 0
FR
Wire Wire Line
	2300 1650 2500 1650
Wire Wire Line
	2300 1650 2300 1750
NoConn ~ 8750 5100
NoConn ~ 8750 5200
NoConn ~ 9750 5200
NoConn ~ 9750 5100
NoConn ~ 9750 5000
Wire Wire Line
	3850 4650 4050 4650
$Comp
L Device:R_Small R9
U 1 1 5D1CCB87
P 4050 4450
F 0 "R9" H 4200 4400 50  0000 R CNN
F 1 "1kΩ" H 4250 4500 50  0000 R CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 4050 4450 50  0001 C CNN
F 3 "~" H 4050 4450 50  0001 C CNN
	1    4050 4450
	-1   0    0    1   
$EndComp
Wire Wire Line
	4050 4550 4050 4650
Connection ~ 4050 4650
Wire Wire Line
	4050 4650 4200 4650
Text Notes 5100 4550 0    50   ~ 0
<--- BUFFER
Text Notes 5200 5150 0    50   ~ 0
LEVELSHIFT + INVERTER --->
$Comp
L Device:LED_RAGB D3
U 1 1 5D5D69FB
P 6150 1850
F 0 "D3" H 6150 2347 50  0000 C CNN
F 1 "ADA-302" H 6150 2256 50  0000 C CNN
F 2 "LED_THT:LED_D5.0mm-4_RGB_Wide_Pins" H 6150 1800 50  0001 C CNN
F 3 "~" H 6150 1800 50  0001 C CNN
	1    6150 1850
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR015
U 1 1 5D5D7508
P 6500 1450
F 0 "#PWR015" H 6500 1300 50  0001 C CNN
F 1 "+3.3V" H 6515 1623 50  0000 C CNN
F 2 "" H 6500 1450 50  0001 C CNN
F 3 "" H 6500 1450 50  0001 C CNN
	1    6500 1450
	1    0    0    -1  
$EndComp
Text GLabel 8700 5000 0    50   Input ~ 0
LED_G
Text GLabel 9800 4900 2    50   Input ~ 0
LED_B
Text GLabel 5350 2050 0    50   Input ~ 0
LED_B
Text GLabel 5350 1650 0    50   Input ~ 0
LED_R
Text GLabel 5350 1850 0    50   Input ~ 0
LED_G
$Comp
L Device:R_Small R6
U 1 1 5D67F26C
P 5550 1650
F 0 "R6" V 5650 1600 50  0000 L CNN
F 1 "100Ω" V 5600 1350 50  0000 L CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 5550 1650 50  0001 C CNN
F 3 "~" H 5550 1650 50  0001 C CNN
	1    5550 1650
	0    -1   -1   0   
$EndComp
$Comp
L Device:R_Small R7
U 1 1 5D67F2E9
P 5550 1850
F 0 "R7" V 5650 1800 50  0000 L CNN
F 1 "100Ω" V 5600 1550 50  0000 L CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 5550 1850 50  0001 C CNN
F 3 "~" H 5550 1850 50  0001 C CNN
	1    5550 1850
	0    -1   -1   0   
$EndComp
$Comp
L Device:R_Small R8
U 1 1 5D67F321
P 5550 2050
F 0 "R8" V 5650 2000 50  0000 L CNN
F 1 "100Ω" V 5600 1750 50  0000 L CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P12.70mm_Horizontal" H 5550 2050 50  0001 C CNN
F 3 "~" H 5550 2050 50  0001 C CNN
	1    5550 2050
	0    -1   -1   0   
$EndComp
Wire Wire Line
	6500 1450 6500 1850
Wire Wire Line
	5450 2050 5350 2050
Wire Wire Line
	5350 1850 5450 1850
Wire Wire Line
	5450 1650 5350 1650
NoConn ~ 8750 4800
Text GLabel 9800 4800 2    50   Input ~ 0
LED_R
Text GLabel 8700 4900 0    50   Input ~ 0
FR
$Comp
L WeMos:WeMos_mini U1
U 1 1 5D587898
P 9250 4850
F 0 "U1" H 9250 5500 60  0000 C CNN
F 1 "WeMos_mini" H 9250 5400 60  0000 C CNN
F 2 "WEMOS_D1_V3:D1-MINI" H 9800 4150 60  0001 C CNN
F 3 "" H 9250 5381 60  0000 C CNN
	1    9250 4850
	1    0    0    -1  
$EndComp
Text Notes 1200 1500 0    50   ~ 0
Factory Reset - all setting restored to default.\nPress switch during power-up, Red led will flash.\nConfigure with smartphone after Factory Reset.
Text Notes 4750 2700 0    50   ~ 0
RED flash: Factory Reset. Release switch to complete\nBLUE ON: configure with smartphone. Connect with Wifi.\nGREEN ON: during 2 seconds: All fine\nGREEN FLASH: successful published smartmeter information. Thx.
Wire Wire Line
	6500 1850 6350 1850
Wire Wire Line
	5650 1650 5950 1650
Wire Wire Line
	5650 1850 5950 1850
Wire Wire Line
	5650 2050 5950 2050
Text Notes 1050 5300 0    50   ~ 0
Direct connect To DSMR port (P1)
Wire Wire Line
	9750 4700 9800 4700
Wire Wire Line
	9800 4800 9750 4800
Wire Wire Line
	9800 4900 9750 4900
Wire Wire Line
	8750 5000 8700 5000
Wire Wire Line
	8750 4900 8700 4900
NoConn ~ 9750 4600
NoConn ~ 8750 4700
$Comp
L power:+5V #PWR0102
U 1 1 5E3C6C1E
P 2650 4500
F 0 "#PWR0102" H 2650 4350 50  0001 C CNN
F 1 "+5V" H 2665 4673 50  0000 C CNN
F 2 "" H 2650 4500 50  0001 C CNN
F 3 "" H 2650 4500 50  0001 C CNN
	1    2650 4500
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0103
U 1 1 5E3C72BB
P 2650 4800
F 0 "#PWR0103" H 2650 4550 50  0001 C CNN
F 1 "GND" H 2655 4627 50  0000 C CNN
F 2 "" H 2650 4800 50  0001 C CNN
F 3 "" H 2650 4800 50  0001 C CNN
	1    2650 4800
	1    0    0    -1  
$EndComp
Wire Wire Line
	2000 4900 2100 4900
Wire Wire Line
	2000 4600 2650 4600
Wire Wire Line
	2650 4600 2650 4500
Wire Wire Line
	2000 4700 2650 4700
Wire Wire Line
	2650 4700 2650 4800
NoConn ~ 2000 4500
NoConn ~ 2000 4800
NoConn ~ 2000 5000
Wire Wire Line
	4050 4200 4050 4350
Wire Wire Line
	4500 5300 4500 5150
Wire Wire Line
	4900 4200 4900 4450
$Comp
L power:GND #PWR?
U 1 1 5E3E6632
P 8450 4700
F 0 "#PWR?" H 8450 4450 50  0001 C CNN
F 1 "GND" H 8455 4527 50  0000 C CNN
F 2 "" H 8450 4700 50  0001 C CNN
F 3 "" H 8450 4700 50  0001 C CNN
	1    8450 4700
	1    0    0    -1  
$EndComp
Wire Wire Line
	8750 4600 8450 4600
Wire Wire Line
	8450 4600 8450 4700
$Comp
L power:+5V #PWR?
U 1 1 5E3E847E
P 8450 4400
F 0 "#PWR?" H 8450 4250 50  0001 C CNN
F 1 "+5V" H 8465 4573 50  0000 C CNN
F 2 "" H 8450 4400 50  0001 C CNN
F 3 "" H 8450 4400 50  0001 C CNN
	1    8450 4400
	1    0    0    -1  
$EndComp
Wire Wire Line
	8750 4500 8450 4500
Wire Wire Line
	8450 4500 8450 4400
$EndSCHEMATC
