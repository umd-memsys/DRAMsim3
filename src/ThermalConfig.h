#ifndef THERMALCONFIG_H
#define THERMALCONFIG_H

static double T0 = 273.15; // [C]

static double R_TSV = 5e-6; // [m]

/* Thermal conductance */
static double Ksi = 148.0; // Silicon
static double Kcu = 401.0; // Copper
static double Kin = 1.5;  // insulator 
static double Khs = 5.0; // Heat sink 

/* Thermal capacitance */
static double Csi = 1.66e6; // Silicon
static double Ccu = 3.2e6; // Copper 
static double Cin = 1.65e6; // insulator 
static double Chs = 2.42e6; // Heat sink 

/* Layer Hight */
static double Hsi = 400e-6; // Silicon 
static double Hcu = 5e-6; // Copper 
static double Hin = 20e-6; // Insulator 
static double Hhs = 1000e-6; // Heat sink 

#endif