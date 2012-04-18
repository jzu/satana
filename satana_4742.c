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
} Satana;

#define DESPIKER_INPUT1  0
#define DESPIKER_OUTPUT1 1


/* Yeuch */

#ifdef WIN32
#define _WINDOWS_DLL_EXPORT_ __declspec(dllexport)
int bIsFirstTime = 1; 
void _init(); // forward declaration
#else
#define _WINDOWS_DLL_EXPORT_ 
#endif


/* Plugin-specific defines */

#define HLF_CNVL 8
#define LEN_CNVL (2*HLF_CNVL+1)
//#define F(x) sin (2*x * M_PI_2)
#define F(x) sin (x * M_PI_2)

/* Convolution matrix */

// If 16 surrounding points act on a given point, then the cutoff frequency
// could well be something like 22050/8 = 2756 Hz. Perhaps. Slope is unknown.

LADSPA_Data matrix [LEN_CNVL] = {                  // Mean should be 1 
  0.2, 0.4, 0.6, 0.8,   1, 1.2, 1.4, 1.6,
  2.6,
  1.6, 1.4, 1.2,   1, 0.8, 0.6, 0.4, 0.2
};


LADSPA_Data *cnvl = matrix + HLF_CNVL;             // Center pointer


/*****************************************************************************
 * Copy input to output while applying the progressive filter
 * F(x) is #define'd as sin(x) by default
 *****************************************************************************/

void runSatana (LADSPA_Handle Instance,
                unsigned long SampleCount) {
  

  LADSPA_Data  *in;
  LADSPA_Data  *out;
  Satana       *psSatana;
  unsigned long i;
  long          c;
  LADSPA_Data   sum;

  psSatana = (Satana *) Instance;
  in = psSatana->m_pfInputBuffer1;
  out = psSatana->m_pfOutputBuffer1;

  for (i = HLF_CNVL; i < SampleCount-HLF_CNVL; i++) {
    sum = 0;
    for (c = -HLF_CNVL ; c <= HLF_CNVL; c++)
      sum += in [i+c] * cnvl [c];
      out [i] = in [i] * (1 - fabs (F (in [i])))
              + (sum/LEN_CNVL) * fabs (F (in [i]));
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
    case DESPIKER_INPUT1:
      psSatana->m_pfInputBuffer1 = DataLocation;
      break;
    case DESPIKER_OUTPUT1:
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

void _init() {

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
    g_psSatanaDescr->PortCount        = 2;
    piPortDescriptors
      = (LADSPA_PortDescriptor *) calloc (2, sizeof (LADSPA_PortDescriptor));
    g_psSatanaDescr->PortDescriptors
      = (const LADSPA_PortDescriptor *) piPortDescriptors;
    piPortDescriptors [DESPIKER_INPUT1]
      = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors [DESPIKER_OUTPUT1]
      = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
    pcPortNames
      = (char **) calloc (2, sizeof(char *));
    g_psSatanaDescr->PortNames 
      = (const char **) pcPortNames;
    pcPortNames [DESPIKER_INPUT1]  = strdup ("Input");
    pcPortNames [DESPIKER_OUTPUT1] = strdup ("Output");
    psPortRangeHints 
      = ((LADSPA_PortRangeHint *) calloc (2, sizeof (LADSPA_PortRangeHint)));
    g_psSatanaDescr->PortRangeHints
      = (const LADSPA_PortRangeHint *) psPortRangeHints;
    psPortRangeHints [DESPIKER_INPUT1].HintDescriptor  = 0;
    psPortRangeHints [DESPIKER_OUTPUT1].HintDescriptor = 0;
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
      free ((char *)(psDescriptor->PortNames[lIndex]));
    free ((char **)psDescriptor->PortNames);
    free ((LADSPA_PortRangeHint *)psDescriptor->PortRangeHints);
    free (psDescriptor);
  }
}

/*****************************************************************************/
/* _fini() is called automatically when the library is unloaded. */

void _fini() {

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


