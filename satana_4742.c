/****************************************************************************
 * Satana
 *
 * Copyright (C) Jean Zundel <jzu@free.fr> 2012 
 *
 * Plugin structure inherited from amp.c:
 * This file has poor memory protection. Failures during malloc() will
 * not recover nicely. 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "ladspa.h"


typedef struct {
  LADSPA_Data * m_pfInputBuffer1;
  LADSPA_Data * m_pfOutputBuffer1;
  LADSPA_Data * m_pfControlValue1;
  LADSPA_Data * m_pfControlValue2;
  LADSPA_Data * m_pfControlValue3;
} Satana;

#define SATANA_INPUT  0
#define SATANA_OUTPUT 1
#define SATANA_CONTROL1 2
#define SATANA_CONTROL2 3
#define SATANA_CONTROL3 4


// Yeuch

#ifdef WIN32
 #define _WINDOWS_DLL_EXPORT_ __declspec(dllexport)
 int bIsFirstTime = 1; 
 void _init(); // forward declaration
#else
 #define _WINDOWS_DLL_EXPORT_ 
#endif

#ifdef TARGET_OS_MAC
 #define MACCONS  __attribute__((constructor))
 #define MACDEST  __attribute__((destructor))
#else
 #define MACCONS
 #define MACDEST
#endif

// Plugin-specific defines

#define HLF_CNVL  8
#define LEN_CNVL  (2*HLF_CNVL+1)


// Functions applying/mixing the convolution
// F(x) for filtered
// G(x) for dry

#define F(x) (pow (x, select))
#define G(x) (1 - F(x))


// Convolution matrix
// If 16 surrounding points act on a given point, then the cutoff frequency
// could well be something like 22050/8 = 2756 Hz, but observations with
// Audacity's spectrum display show it could be around 1800 Hz @ ~6 dB/octave
// with the current matrix. To be confirmed.
// Experimental feature: its contents are re-computed at the beginning of
// runSatana() from a gaussian function (1/(2*sqrt(s*M_PI))*exp(-x*x/(2*s*s))
// whose "s" factor (sigma) comes from the second control.

LADSPA_Data matrix [LEN_CNVL] = {                  // Mean should be 1 
  0.2, 0.4, 0.6, 0.8,   1, 1.2, 1.4, 1.6,
  2.6,
  1.6, 1.4, 1.2,   1, 0.8, 0.6, 0.4, 0.2
};


LADSPA_Data *cnvl = matrix + HLF_CNVL;             // Center pointer


/*****************************************************************************
 * Copy input to output while applying the progressive filter
 * (whose values are first computed as a gaussian)
 * Application function F(x) is #define'd as x^select
 *****************************************************************************/

void runSatana (LADSPA_Handle Instance,
                unsigned long SampleCount) {
  

  LADSPA_Data  *in;
  LADSPA_Data  *out;
  LADSPA_Data   select;
  LADSPA_Data   sigma;
  Satana       *psSatana;
  unsigned long i;
  long          c;
  LADSPA_Data   sum;

  psSatana = (Satana *) Instance;
  in  = psSatana->m_pfInputBuffer1;
  out = psSatana->m_pfOutputBuffer1;
  select = *(psSatana->m_pfControlValue1);
  sigma  = *(psSatana->m_pfControlValue2);
                                                   // Matrix is gaussian
  for (c = -HLF_CNVL; c <= HLF_CNVL; c++) 
    cnvl [c] = 17 * pow (M_E, -c * c / (2 * sigma * sigma)) / 
                    (sigma * sqrt (2 * M_PI));

  for (i = HLF_CNVL; i < SampleCount-HLF_CNVL; i++) {
    sum = 0;
    for (c = -HLF_CNVL ; c <= HLF_CNVL; c++)       // Convolve
      sum += in [i+c] * cnvl [c];
    out [i] = F (fabs (in [i])) * fabs (sum/LEN_CNVL) 
            + G (fabs (in [i])) * fabs (in [i]);
    if (in [i] < 0)
      out [i] = -out [i];
  }
}


/*****************************************************************************/

LADSPA_Handle instantiateSatana (const LADSPA_Descriptor * Descriptor,
                                 unsigned long             SampleRate) {
  return malloc (sizeof (Satana));
}

/*****************************************************************************/

void connectPortToSatana (LADSPA_Handle Instance,
                          unsigned long Port,
                          LADSPA_Data * DataLocation) {

  Satana * psSatana;

  psSatana = (Satana *) Instance;
  switch (Port) {
    case SATANA_CONTROL1:
      psSatana->m_pfControlValue1 = DataLocation;
      break;
    case SATANA_CONTROL2:
      psSatana->m_pfControlValue2 = DataLocation;
      break;
    case SATANA_CONTROL3:
      psSatana->m_pfControlValue3 = DataLocation;
      break;
    case SATANA_INPUT:
      psSatana->m_pfInputBuffer1  = DataLocation;
      break;
    case SATANA_OUTPUT:
      psSatana->m_pfOutputBuffer1 = DataLocation;
      break;
  }
}

/*****************************************************************************/

void cleanupSatana (LADSPA_Handle Instance) {

  free (Instance);
}

/*****************************************************************************/

LADSPA_Descriptor * g_psSatanaDescr = NULL;

/*****************************************************************************/
/* _init() is called automatically when the plugin library is first
   loaded. */

MACCONS void _init() {

  char ** pcPortNames;
  LADSPA_PortDescriptor * piPortDescriptors;
  LADSPA_PortRangeHint * psPortRangeHints;

  g_psSatanaDescr
    = (LADSPA_Descriptor *) malloc (sizeof (LADSPA_Descriptor));

  if (g_psSatanaDescr) {
  
    g_psSatanaDescr->UniqueID         = 4742;
    g_psSatanaDescr->Label            = strdup ("Satana");
    g_psSatanaDescr->Properties
      = LADSPA_PROPERTY_HARD_RT_CAPABLE;
    g_psSatanaDescr->Name             = strdup ("Satana");
    g_psSatanaDescr->Maker            = strdup ("Jean Zundel");
    g_psSatanaDescr->Copyright        = strdup ("GPL v3");
    g_psSatanaDescr->PortCount        = 5;

    piPortDescriptors
      = (LADSPA_PortDescriptor *) calloc (5, sizeof (LADSPA_PortDescriptor));
    g_psSatanaDescr->PortDescriptors
      = (const LADSPA_PortDescriptor *) piPortDescriptors;
    piPortDescriptors [SATANA_INPUT]
      = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors [SATANA_OUTPUT]
      = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors [SATANA_CONTROL1]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    piPortDescriptors [SATANA_CONTROL2]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    piPortDescriptors [SATANA_CONTROL3]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;

    pcPortNames = (char **) calloc (5, sizeof(char *));
    g_psSatanaDescr->PortNames = (const char **) pcPortNames;
    pcPortNames [SATANA_INPUT]    = strdup ("Input");
    pcPortNames [SATANA_OUTPUT]   = strdup ("Output");
    pcPortNames [SATANA_CONTROL1] = strdup ("Selectivity");
    pcPortNames [SATANA_CONTROL2] = strdup ("Filtering");
    pcPortNames [SATANA_CONTROL3] = strdup ("reserved");

    psPortRangeHints 
      = ((LADSPA_PortRangeHint *) calloc (5, sizeof (LADSPA_PortRangeHint)));
    g_psSatanaDescr->PortRangeHints
      = (const LADSPA_PortRangeHint *) psPortRangeHints;

    psPortRangeHints [SATANA_INPUT].HintDescriptor  = 0;
    psPortRangeHints [SATANA_OUTPUT].HintDescriptor = 0;

    psPortRangeHints [SATANA_CONTROL1].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW 
         | LADSPA_HINT_LOGARITHMIC
         | LADSPA_HINT_DEFAULT_MIDDLE);
    psPortRangeHints [SATANA_CONTROL1].LowerBound   = 0;
    psPortRangeHints [SATANA_CONTROL1].UpperBound   = 10;

    psPortRangeHints [SATANA_CONTROL2].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW 
         | LADSPA_HINT_LOGARITHMIC
         | LADSPA_HINT_DEFAULT_MIDDLE);
    psPortRangeHints [SATANA_CONTROL2].LowerBound   = 0;
    psPortRangeHints [SATANA_CONTROL2].UpperBound   = 10;

    psPortRangeHints [SATANA_CONTROL3].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW 
         | LADSPA_HINT_LOGARITHMIC
         | LADSPA_HINT_DEFAULT_MIDDLE);
    psPortRangeHints [SATANA_CONTROL3].LowerBound   = 0;
    psPortRangeHints [SATANA_CONTROL3].UpperBound   = 10;

    g_psSatanaDescr->instantiate  = instantiateSatana;
    g_psSatanaDescr->connect_port = connectPortToSatana;
    g_psSatanaDescr->activate     = NULL;
    g_psSatanaDescr->run          = runSatana;
    g_psSatanaDescr->run_adding   = NULL;
    g_psSatanaDescr->deactivate   = NULL;
    g_psSatanaDescr->cleanup      = cleanupSatana;
  }
}

/*****************************************************************************/

void deleteDescriptor (LADSPA_Descriptor * psDescriptor) {

  unsigned long lIndex;

  if (psDescriptor) {
    free ((char *)psDescriptor->Label);
    free ((char *)psDescriptor->Name);
    free ((char *)psDescriptor->Maker);
    free ((char *)psDescriptor->Copyright);
    free ((LADSPA_PortDescriptor *)psDescriptor->PortDescriptors);
    for (lIndex = 0; lIndex < psDescriptor->PortCount; lIndex++)
      free ((char *)(psDescriptor->PortNames [lIndex]));
    free ((char **)psDescriptor->PortNames);
    free ((LADSPA_PortRangeHint *)psDescriptor->PortRangeHints);
    free (psDescriptor);
  }
}

/*****************************************************************************/
/* _fini() is called automatically when the library is unloaded. */

MACDEST void _fini() {

  deleteDescriptor (g_psSatanaDescr);
}

/*****************************************************************************/

_WINDOWS_DLL_EXPORT_
const LADSPA_Descriptor *ladspa_descriptor (unsigned long Index) {

#ifdef WIN32
  if (bIsFirstTime) {
    _init();
    bIsFirstTime = 0;
  }
#endif

  if (Index == 0)
    return g_psSatanaDescr;
  else
    return NULL;
}


