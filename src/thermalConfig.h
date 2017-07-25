#ifndef THERMALCONFIG_H
#define THERMALCONFIG_H

extern double T0;
extern double Tamb;

extern double ChipX;
extern double ChipZ;
extern double R_TSV;

/* Thermal conductance */
extern double Ksi;
extern double Kcu;
extern double Kin;
extern double Khs; 

/* Thermal capacitance */
extern double Csi;
extern double Ccu;
extern double Cin;
extern double Chs;

/* Layer Hight */
extern double Hsi;
extern double Hcu;
extern double Hin;
extern double Hhs;

/* trasient control parameters */
extern int TimeIter0;

#endif