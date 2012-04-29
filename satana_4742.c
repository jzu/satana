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


// Plugin-specific defines

#define HGH_CNVL  11
#define HLF_CNVL  12
#define LEN_CNVL  (2*HLF_CNVL+1)


// Functions applying/mixing the convolution
// F(x) for filtered
// G(x) for dry

#define F(x) (pow (x, selec))
#define G(x) (1 - F(x))


// Convolution vectors
// Savitzky-Golay filter coefficients
// http://www.statistics4u.com/fundstat_eng/cc_filter_savgolay.html

LADSPA_Data matrix [HGH_CNVL][LEN_CNVL] = {
  { 0,       0,       0,       0,       0,       0,       0,       // 5
    0,       0,       0,      -0.0857,  0.3429,  0.4857,  0.3429, 
   -0.0857,  0,       0,       0,       0,       0,       0, 
    0,       0,       0,       0},
  { 0,       0,       0,       0,       0,       0,       0,       // 7
    0,       0,      -0.0952,  0.1429,  0.2857,  0.3333,  0.2857, 
    0.1429, -0.0952,  0,       0,       0,       0,       0, 
    0,       0,       0,       0}, 
  { 0,       0,       0,       0,       0,       0,       0,       // 9
    0,      -0.0909,  0.0606,  0.1688,  0.2338,  0.2554,  0.2338,  
    0.1688,  0.0606, -0.0909,  0,       0,       0,       0, 
    0,       0,       0,       0}, 
  { 0,       0,       0,       0,       0,       0,       0,       // 11
   -0.0839,  0.0210,  0.1026,  0.1608,  0.1958,  0.2075,  0.1958, 
    0.1608,  0.1026,  0.0210, -0.0839,  0,       0,       0, 
    0,       0,       0,       0}, 
  { 0,       0,       0,       0,       0,       0,      -0.0769,  // 13
    0,       0.0629,  0.1119,  0.1469,  0.1678,  0.1748,  0.1678, 
    0.1469,  0.1119, 0.0629,   0,      -0.0769,  0,       0,
    0,       0,       0,       0},
  { 0,       0,       0,       0,       0,      -0.0706, -0.0118,  // 15
    0.0380,  0.0787,  0.1104,  0.1330,  0.1466,  0.1511,  0.1466, 
    0.1330,  0.1104,  0.0787,  0.0380, -0.0118, -0.0706,  0, 
    0,       0,       0,       0},
  { 0,       0,       0,       0,      -0.0650, -0.0186,  0.0217,  // 17
    0.0557,  0.0836,  0.1053,  0.1207,  0.1300,  0.1331,  0.1300, 
    0.1207,  0.1053,  0.0836,  0.0557,  0.0217, -0.0186, -0.0650, 
    0,       0,       0,       0},
  { 0,       0,       0,       0,      -0.0827,  0.0106,  0.0394,  // 19
    0.0637,  0.0836,  0.0991,  0.1101,  0.1168,  0.1190,  0.1168, 
    0.1101,  0.0991,  0.0836,  0.0637,  0.0394,  0.0106, -0.0827, 
    0,       0,       0,       0},
  { 0,       0,       0,      -0.0807,  0.0029,  0.0275,  0.0487,  // 21
    0.0667,  0.0814,  0.0928,  0.1010,  0.1059,  0.1076,  0.1059, 
    0.1010,  0.0928,  0.0814,  0.0667,  0.0487,  0.0275,  0.0029, 
   -0.0807,  0,       0,       0},      
  { 0,      -0.0522, -0.0261, -0.0025,  0.0186,  0.0373,  0.0534,  // 23
    0.0671,  0.0783,  0.0870,  0.0932,  0.0969,  0.0981,  0.0969, 
    0.0932, 0.0870,   0.0783,  0.0671,  0.0534,  0.0373,  0.0186, 
   -0.0025, -0.0261, -0.0522,  0},
  {-0.0489, -0.0267, -0.0064,  0.0120,  0.0284,  0.0429,  0.0555,  // 25
    0.0663,  0.0748,  0.0815,  0.0864,  0.0893,  0.0902,  0.0893,  
    0.0864,  0.0815,  0.0748,  0.0663,  0.0555,  0.0429,  0.0284,  
    0.0120, -0.0064, -0.0267, -0.0489}
};



/*****************************************************************************
 * Copy input to output while compressing then applying the progressive filter
 * Precompression is time-independent ( sin(sin...(x)) )
 * Application function F(x) is #define'd as x^selec ; G(x) as 1 - x^selec
 *****************************************************************************/

void runSatana (LADSPA_Handle Instance,
                unsigned long SampleCount) {
  

  Satana       *psSatana;
  LADSPA_Data  *in;
  LADSPA_Data  *out;
  LADSPA_Data   selec;
  unsigned long effic;
  unsigned long compr;
  unsigned long i;
  long          c;
  LADSPA_Data   sum;
  LADSPA_Data  *cnvl;

  psSatana = (Satana *) Instance;

  in    = psSatana->m_pfInputBuffer1;
  out   = psSatana->m_pfOutputBuffer1;
  compr = lroundf (*(psSatana->m_pfControlValue1));
  selec = *(psSatana->m_pfControlValue2);
  effic = lroundf (*(psSatana->m_pfControlValue3) / 2 - 3);
  cnvl  = matrix [effic] + HLF_CNVL;

printf ("[Satana] compr=%lu, selec=%f, effic=%lu\n", compr, selec, effic);

  for (i = HLF_CNVL; i < SampleCount-HLF_CNVL; i++)       // Precompression
    for (c = 0; c < compr; c++)
      in [i] = sin (in [i] * M_PI_2);

  for (i = HLF_CNVL; i < SampleCount-HLF_CNVL; i++) {     // Convolution
    sum = 0;
    for (c = -HLF_CNVL ; c <= HLF_CNVL; c++)
      sum += in [i+c] * cnvl [c];
    out [i] = F (fabs (in [i])) * fabs (sum) 
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

__attribute__((constructor)) void _init() {

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
    pcPortNames [SATANA_CONTROL1] = strdup ("Precomp\n(sin repetition)     ");
    pcPortNames [SATANA_CONTROL2] = strdup ("Selectivity\n(all<->only a few)");
    pcPortNames [SATANA_CONTROL3] = strdup ("Efficiency\n(5,7,9..25 coeffs) ");

    psPortRangeHints 
      = ((LADSPA_PortRangeHint *) calloc (5, sizeof (LADSPA_PortRangeHint)));
    g_psSatanaDescr->PortRangeHints
      = (const LADSPA_PortRangeHint *) psPortRangeHints;

    psPortRangeHints [SATANA_INPUT].HintDescriptor  = 0;
    psPortRangeHints [SATANA_OUTPUT].HintDescriptor = 0;

    psPortRangeHints [SATANA_CONTROL1].HintDescriptor      // Precomp
      = (LADSPA_HINT_BOUNDED_BELOW
         | LADSPA_HINT_BOUNDED_ABOVE
         | LADSPA_HINT_INTEGER
         | LADSPA_HINT_DEFAULT_0);
    psPortRangeHints [SATANA_CONTROL1].LowerBound   = 0;
    psPortRangeHints [SATANA_CONTROL1].UpperBound   = 5;

    psPortRangeHints [SATANA_CONTROL2].HintDescriptor      // Selectivity
      = (LADSPA_HINT_BOUNDED_BELOW 
         | LADSPA_HINT_BOUNDED_ABOVE
         | LADSPA_HINT_LOGARITHMIC
         | LADSPA_HINT_DEFAULT_MIDDLE);
    psPortRangeHints [SATANA_CONTROL2].LowerBound   = 0;
    psPortRangeHints [SATANA_CONTROL2].UpperBound   = 10;

    psPortRangeHints [SATANA_CONTROL3].HintDescriptor      // Efficiency
      = (LADSPA_HINT_BOUNDED_BELOW 
         | LADSPA_HINT_BOUNDED_ABOVE
         | LADSPA_HINT_INTEGER
         | LADSPA_HINT_DEFAULT_MIDDLE);
    psPortRangeHints [SATANA_CONTROL3].LowerBound   = 5;
    psPortRangeHints [SATANA_CONTROL3].UpperBound   = 25;

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

__attribute__((destructor)) void _fini() {

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


