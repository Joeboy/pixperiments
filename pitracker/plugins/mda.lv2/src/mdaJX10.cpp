/*
  Copyright 2008-2011 David Robillard <http://drobilla.net>
  Copyright 1999-2000 Paul Kellett (Maxim Digital Audio)

  This is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License,
  or (at your option) any later version.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this software. If not, see <http://www.gnu.org/licenses/>.
*/

#include "mdaJX10.h"

#include "lv2/lv2plug.in/ns/ext/atom/util.h"

#include <stdio.h>
#include <stdlib.h> //rand()
#include <math.h>


AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaJX10(audioMaster);
}


mdaJX10Program::mdaJX10Program()
{
  param[0]  = 0.00f; //OSC Mix
  param[1]  = 0.25f; //OSC Tune
  param[2]  = 0.50f; //OSC Fine

  param[3]  = 0.00f; //OSC Mode
  param[4]  = 0.35f; //OSC Rate
  param[5]  = 0.50f; //OSC Bend

  param[6]  = 1.00f; //VCF Freq
  param[7]  = 0.15f; //VCF Reso
  param[8]  = 0.75f; //VCF <Env

  param[9]  = 0.00f; //VCF <LFO
  param[10] = 0.50f; //VCF <Vel
  param[11] = 0.00f; //VCF Att

  param[12] = 0.30f; //VCF Dec
  param[13] = 0.00f; //VCF Sus
  param[14] = 0.25f; //VCF Rel

  param[15] = 0.00f; //ENV Att
	param[16] = 0.50f; //ENV Dec
  param[17] = 1.00f; //ENV Sus
	
  param[18] = 0.30f; //ENV Rel
	param[19] = 0.81f; //LFO Rate
	param[20] = 0.50f; //Vibrato
  
  param[21] = 0.00f; //Noise   - not present in original patches
  param[22] = 0.50f; //Octave
  param[23] = 0.50f; //Tuning
  strcpy (name, "Empty Patch");  
}


mdaJX10::mdaJX10(audioMasterCallback audioMaster) : AudioEffectX(audioMaster, NPROGS, NPARAMS)
{
  int32_t i=0;
  Fs = 44100.0f;

  programs = new mdaJX10Program[NPROGS];
	if(programs)
  {
    fillpatch(i++, "5th Sweep Pad", 1.0f, 0.37f, 0.25f, 0.3f, 0.32f, 0.5f, 0.9f, 0.6f, 0.12f, 0.0f, 0.5f, 0.9f, 0.89f, 0.9f, 0.73f, 0.0f, 0.5f, 1.0f, 0.71f, 0.81f, 0.65f, 0.0f, 0.5f, 0.5f);
    fillpatch(i++, "Echo Pad [SA]", 0.88f, 0.51f, 0.5f, 0.0f, 0.49f, 0.5f, 0.46f, 0.76f, 0.69f, 0.1f, 0.69f, 1.0f, 0.86f, 0.76f, 0.57f, 0.3f, 0.8f, 0.68f, 0.66f, 0.79f, 0.13f, 0.25f, 0.45f, 0.5f);
    fillpatch(i++, "Space Chimes [SA]", 0.88f, 0.51f, 0.5f, 0.16f, 0.49f, 0.5f, 0.49f, 0.82f, 0.66f, 0.08f, 0.89f, 0.85f, 0.69f, 0.76f, 0.47f, 0.12f, 0.22f, 0.55f, 0.66f, 0.89f, 0.34f, 0.0f, 1.0f, 0.5f);
    fillpatch(i++, "Solid Backing", 1.0f, 0.26f, 0.14f, 0.0f, 0.35f, 0.5f, 0.3f, 0.25f, 0.7f, 0.0f, 0.63f, 0.0f, 0.35f, 0.0f, 0.25f, 0.0f, 0.5f, 1.0f, 0.3f, 0.81f, 0.5f, 0.5f, 0.5f, 0.5f);
    fillpatch(i++, "Velocity Backing [SA]", 0.41f, 0.5f, 0.79f, 0.0f, 0.08f, 0.32f, 0.49f, 0.01f, 0.34f, 0.0f, 0.93f, 0.61f, 0.87f, 1.0f, 0.93f, 0.11f, 0.48f, 0.98f, 0.32f, 0.81f, 0.5f, 0.0f, 0.5f, 0.5f);
    fillpatch(i++, "Rubber Backing [ZF]", 0.29f, 0.76f, 0.26f, 0.0f, 0.18f, 0.76f, 0.35f, 0.15f, 0.77f, 0.14f, 0.54f, 0.0f, 0.42f, 0.13f, 0.21f, 0.0f, 0.56f, 0.0f, 0.32f, 0.2f, 0.58f, 0.22f, 0.53f, 0.5f);
    fillpatch(i++, "808 State Lead", 1.0f, 0.65f, 0.24f, 0.4f, 0.34f, 0.85f, 0.65f, 0.63f, 0.75f, 0.16f, 0.5f, 0.0f, 0.3f, 0.0f, 0.25f, 0.17f, 0.5f, 1.0f, 0.03f, 0.81f, 0.5f, 0.0f, 0.68f, 0.5f);
    fillpatch(i++, "Mono Glide", 0.0f, 0.25f, 0.5f, 1.0f, 0.46f, 0.5f, 0.51f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.25f, 0.37f, 0.5f, 1.0f, 0.38f, 0.81f, 0.62f, 0.0f, 0.5f, 0.5f);
    fillpatch(i++, "Detuned Techno Lead", 0.84f, 0.51f, 0.15f, 0.45f, 0.41f, 0.42f, 0.54f, 0.01f, 0.58f, 0.21f, 0.67f, 0.0f, 0.09f, 1.0f, 0.25f, 0.2f, 0.85f, 1.0f, 0.3f, 0.83f, 0.09f, 0.4f, 0.49f, 0.5f);
    fillpatch(i++, "Hard Lead [SA]", 0.71f, 0.75f, 0.53f, 0.18f, 0.24f, 1.0f, 0.56f, 0.52f, 0.69f, 0.19f, 0.7f, 1.0f, 0.14f, 0.65f, 0.95f, 0.07f, 0.91f, 1.0f, 0.15f, 0.84f, 0.33f, 0.0f, 0.49f, 0.5f);
    fillpatch(i++, "Bubble", 0.0f, 0.25f, 0.43f, 0.0f, 0.71f, 0.48f, 0.23f, 0.77f, 0.8f, 0.32f, 0.63f, 0.4f, 0.18f, 0.66f, 0.14f, 0.0f, 0.38f, 0.65f, 0.16f, 0.48f, 0.5f, 0.0f, 0.67f, 0.5f);
    fillpatch(i++, "Monosynth", 0.62f, 0.26f, 0.51f, 0.79f, 0.35f, 0.54f, 0.64f, 0.39f, 0.51f, 0.65f, 0.0f, 0.07f, 0.52f, 0.24f, 0.84f, 0.13f, 0.3f, 0.76f, 0.21f, 0.58f, 0.3f, 0.0f, 0.36f, 0.5f);
    fillpatch(i++, "Moogcury Lite", 0.81f, 1.0f, 0.21f, 0.78f, 0.15f, 0.35f, 0.39f, 0.17f, 0.69f, 0.4f, 0.62f, 0.0f, 0.47f, 0.19f, 0.37f, 0.0f, 0.5f, 0.2f, 0.33f, 0.38f, 0.53f, 0.0f, 0.12f, 0.5f);
    fillpatch(i++, "Gangsta Whine", 0.0f, 0.51f, 0.52f, 0.96f, 0.44f, 0.5f, 0.41f, 0.46f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.25f, 0.15f, 0.5f, 1.0f, 0.32f, 0.81f, 0.49f, 0.0f, 0.83f, 0.5f);
    fillpatch(i++, "Higher Synth [ZF]", 0.48f, 0.51f, 0.22f, 0.0f, 0.0f, 0.5f, 0.5f, 0.47f, 0.73f, 0.3f, 0.8f, 0.0f, 0.1f, 0.0f, 0.07f, 0.0f, 0.42f, 0.0f, 0.22f, 0.21f, 0.59f, 0.16f, 0.98f, 0.5f);
    fillpatch(i++, "303 Saw Bass", 0.0f, 0.51f, 0.5f, 0.83f, 0.49f, 0.5f, 0.55f, 0.75f, 0.69f, 0.35f, 0.5f, 0.0f, 0.56f, 0.0f, 0.56f, 0.0f, 0.8f, 1.0f, 0.24f, 0.26f, 0.49f, 0.0f, 0.07f, 0.5f);
    fillpatch(i++, "303 Square Bass", 0.75f, 0.51f, 0.5f, 0.83f, 0.49f, 0.5f, 0.55f, 0.75f, 0.69f, 0.35f, 0.5f, 0.14f, 0.49f, 0.0f, 0.39f, 0.0f, 0.8f, 1.0f, 0.24f, 0.26f, 0.49f, 0.0f, 0.07f, 0.5f);
    fillpatch(i++, "Analog Bass", 1.0f, 0.25f, 0.2f, 0.81f, 0.19f, 0.5f, 0.3f, 0.51f, 0.85f, 0.09f, 0.0f, 0.0f, 0.88f, 0.0f, 0.21f, 0.0f, 0.5f, 1.0f, 0.46f, 0.81f, 0.5f, 0.0f, 0.27f, 0.5f);
    fillpatch(i++, "Analog Bass 2", 1.0f, 0.25f, 0.2f, 0.72f, 0.19f, 0.86f, 0.48f, 0.43f, 0.94f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.61f, 1.0f, 0.32f, 0.81f, 0.5f, 0.0f, 0.27f, 0.5f);
    fillpatch(i++, "Low Pulses", 0.97f, 0.26f, 0.3f, 0.0f, 0.35f, 0.5f, 0.8f, 0.4f, 0.52f, 0.0f, 0.5f, 0.0f, 0.77f, 0.0f, 0.25f, 0.0f, 0.5f, 1.0f, 0.3f, 0.81f, 0.16f, 0.0f, 0.0f, 0.5f);
    fillpatch(i++, "Sine Infra-Bass", 0.0f, 0.25f, 0.5f, 0.65f, 0.35f, 0.5f, 0.33f, 0.76f, 0.53f, 0.0f, 0.5f, 0.0f, 0.3f, 0.0f, 0.25f, 0.0f, 0.55f, 0.25f, 0.3f, 0.81f, 0.52f, 0.0f, 0.14f, 0.5f);
    fillpatch(i++, "Wobble Bass [SA]", 1.0f, 0.26f, 0.22f, 0.64f, 0.82f, 0.59f, 0.72f, 0.47f, 0.34f, 0.34f, 0.82f, 0.2f, 0.69f, 1.0f, 0.15f, 0.09f, 0.5f, 1.0f, 0.07f, 0.81f, 0.46f, 0.0f, 0.24f, 0.5f);
    fillpatch(i++, "Squelch Bass", 1.0f, 0.26f, 0.22f, 0.71f, 0.35f, 0.5f, 0.67f, 0.7f, 0.26f, 0.0f, 0.5f, 0.48f, 0.69f, 1.0f, 0.15f, 0.0f, 0.5f, 1.0f, 0.07f, 0.81f, 0.46f, 0.0f, 0.24f, 0.5f);
    fillpatch(i++, "Rubber Bass [ZF]", 0.49f, 0.25f, 0.66f, 0.81f, 0.35f, 0.5f, 0.36f, 0.15f, 0.75f, 0.2f, 0.5f, 0.0f, 0.38f, 0.0f, 0.25f, 0.0f, 0.6f, 1.0f, 0.22f, 0.19f, 0.5f, 0.0f, 0.17f, 0.5f);
    fillpatch(i++, "Soft Pick Bass", 0.37f, 0.51f, 0.77f, 0.71f, 0.22f, 0.5f, 0.33f, 0.47f, 0.71f, 0.16f, 0.59f, 0.0f, 0.0f, 0.0f, 0.25f, 0.04f, 0.58f, 0.0f, 0.22f, 0.15f, 0.44f, 0.33f, 0.15f, 0.5f);
    fillpatch(i++, "Fretless Bass", 0.5f, 0.51f, 0.17f, 0.8f, 0.34f, 0.5f, 0.51f, 0.0f, 0.58f, 0.0f, 0.67f, 0.0f, 0.09f, 0.0f, 0.25f, 0.2f, 0.85f, 0.0f, 0.3f, 0.81f, 0.7f, 0.0f, 0.0f, 0.5f);
    fillpatch(i++, "Whistler", 0.23f, 0.51f, 0.38f, 0.0f, 0.35f, 0.5f, 0.33f, 1.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.29f, 0.0f, 0.25f, 0.68f, 0.39f, 0.58f, 0.36f, 0.81f, 0.64f, 0.38f, 0.92f, 0.5f);
    fillpatch(i++, "Very Soft Pad", 0.39f, 0.51f, 0.27f, 0.38f, 0.12f, 0.5f, 0.35f, 0.78f, 0.5f, 0.0f, 0.5f, 0.0f, 0.3f, 0.0f, 0.25f, 0.35f, 0.5f, 0.8f, 0.7f, 0.81f, 0.5f, 0.0f, 0.5f, 0.5f);
    fillpatch(i++, "Pizzicato", 0.0f, 0.25f, 0.5f, 0.0f, 0.35f, 0.5f, 0.23f, 0.2f, 0.75f, 0.0f, 0.5f, 0.0f, 0.22f, 0.0f, 0.25f, 0.0f, 0.47f, 0.0f, 0.3f, 0.81f, 0.5f, 0.8f, 0.5f, 0.5f);
    fillpatch(i++, "Synth Strings", 1.0f, 0.51f, 0.24f, 0.0f, 0.0f, 0.35f, 0.42f, 0.26f, 0.75f, 0.14f, 0.69f, 0.0f, 0.67f, 0.55f, 0.97f, 0.82f, 0.7f, 1.0f, 0.42f, 0.84f, 0.67f, 0.3f, 0.47f, 0.5f);
    fillpatch(i++, "Synth Strings 2", 0.75f, 0.51f, 0.29f, 0.0f, 0.49f, 0.5f, 0.55f, 0.16f, 0.69f, 0.08f, 0.2f, 0.76f, 0.29f, 0.76f, 1.0f, 0.46f, 0.8f, 1.0f, 0.39f, 0.79f, 0.27f, 0.0f, 0.68f, 0.5f);
    fillpatch(i++, "Leslie Organ", 0.0f, 0.5f, 0.53f, 0.0f, 0.13f, 0.39f, 0.38f, 0.74f, 0.54f, 0.2f, 0.0f, 0.0f, 0.55f, 0.52f, 0.31f, 0.0f, 0.17f, 0.73f, 0.28f, 0.87f, 0.24f, 0.0f, 0.29f, 0.5f);
    fillpatch(i++, "Click Organ", 0.5f, 0.77f, 0.52f, 0.0f, 0.35f, 0.5f, 0.44f, 0.5f, 0.65f, 0.16f, 0.0f, 0.0f, 0.0f, 0.18f, 0.0f, 0.0f, 0.75f, 0.8f, 0.0f, 0.81f, 0.49f, 0.0f, 0.44f, 0.5f);
    fillpatch(i++, "Hard Organ", 0.89f, 0.91f, 0.37f, 0.0f, 0.35f, 0.5f, 0.51f, 0.62f, 0.54f, 0.0f, 0.0f, 0.0f, 0.37f, 0.0f, 1.0f, 0.04f, 0.08f, 0.72f, 0.04f, 0.77f, 0.49f, 0.0f, 0.58f, 0.5f);
    fillpatch(i++, "Bass Clarinet", 1.0f, 0.51f, 0.51f, 0.37f, 0.0f, 0.5f, 0.51f, 0.1f, 0.5f, 0.11f, 0.5f, 0.0f, 0.0f, 0.0f, 0.25f, 0.35f, 0.65f, 0.65f, 0.32f, 0.79f, 0.49f, 0.2f, 0.35f, 0.5f);
    fillpatch(i++, "Trumpet", 0.0f, 0.51f, 0.51f, 0.82f, 0.06f, 0.5f, 0.57f, 0.0f, 0.32f, 0.15f, 0.5f, 0.21f, 0.15f, 0.0f, 0.25f, 0.24f, 0.6f, 0.8f, 0.1f, 0.75f, 0.55f, 0.25f, 0.69f, 0.5f);
    fillpatch(i++, "Soft Horn", 0.12f, 0.9f, 0.67f, 0.0f, 0.35f, 0.5f, 0.5f, 0.21f, 0.29f, 0.12f, 0.6f, 0.0f, 0.35f, 0.36f, 0.25f, 0.08f, 0.5f, 1.0f, 0.27f, 0.83f, 0.51f, 0.1f, 0.25f, 0.5f);
    fillpatch(i++, "Brass Section", 0.43f, 0.76f, 0.23f, 0.0f, 0.28f, 0.36f, 0.5f, 0.0f, 0.59f, 0.0f, 0.5f, 0.24f, 0.16f, 0.91f, 0.08f, 0.17f, 0.5f, 0.8f, 0.45f, 0.81f, 0.5f, 0.0f, 0.58f, 0.5f);
    fillpatch(i++, "Synth Brass", 0.4f, 0.51f, 0.25f, 0.0f, 0.3f, 0.28f, 0.39f, 0.15f, 0.75f, 0.0f, 0.5f, 0.39f, 0.3f, 0.82f, 0.25f, 0.33f, 0.74f, 0.76f, 0.41f, 0.81f, 0.47f, 0.23f, 0.5f, 0.5f);
    fillpatch(i++, "Detuned Syn Brass [ZF]", 0.68f, 0.5f, 0.93f, 0.0f, 0.31f, 0.62f, 0.26f, 0.07f, 0.85f, 0.0f, 0.66f, 0.0f, 0.83f, 0.0f, 0.05f, 0.0f, 0.75f, 0.54f, 0.32f, 0.76f, 0.37f, 0.29f, 0.56f, 0.5f);
    fillpatch(i++, "Power PWM", 1.0f, 0.27f, 0.22f, 0.0f, 0.35f, 0.5f, 0.82f, 0.13f, 0.75f, 0.0f, 0.0f, 0.24f, 0.3f, 0.88f, 0.34f, 0.0f, 0.5f, 1.0f, 0.48f, 0.71f, 0.37f, 0.0f, 0.35f, 0.5f);
    fillpatch(i++, "Water Velocity [SA]", 0.76f, 0.51f, 0.35f, 0.0f, 0.49f, 0.5f, 0.87f, 0.67f, 1.0f, 0.32f, 0.09f, 0.95f, 0.56f, 0.72f, 1.0f, 0.04f, 0.76f, 0.11f, 0.46f, 0.88f, 0.72f, 0.0f, 0.38f, 0.5f);
    fillpatch(i++, "Ghost [SA]", 0.75f, 0.51f, 0.24f, 0.45f, 0.16f, 0.48f, 0.38f, 0.58f, 0.75f, 0.16f, 0.81f, 0.0f, 0.3f, 0.4f, 0.31f, 0.37f, 0.5f, 1.0f, 0.54f, 0.85f, 0.83f, 0.43f, 0.46f, 0.5f);
    fillpatch(i++, "Soft E.Piano", 0.31f, 0.51f, 0.43f, 0.0f, 0.35f, 0.5f, 0.34f, 0.26f, 0.53f, 0.0f, 0.63f, 0.0f, 0.22f, 0.0f, 0.39f, 0.0f, 0.8f, 0.0f, 0.44f, 0.81f, 0.51f, 0.0f, 0.5f, 0.5f);
    fillpatch(i++, "Thumb Piano", 0.72f, 0.82f, 1.0f, 0.0f, 0.35f, 0.5f, 0.37f, 0.47f, 0.54f, 0.0f, 0.5f, 0.0f, 0.45f, 0.0f, 0.39f, 0.0f, 0.39f, 0.0f, 0.48f, 0.81f, 0.6f, 0.0f, 0.71f, 0.5f);
    fillpatch(i++, "Steel Drums [ZF]", 0.81f, 0.76f, 0.19f, 0.0f, 0.18f, 0.7f, 0.4f, 0.3f, 0.54f, 0.17f, 0.4f, 0.0f, 0.42f, 0.23f, 0.47f, 0.12f, 0.48f, 0.0f, 0.49f, 0.53f, 0.36f, 0.34f, 0.56f, 0.5f);       
    
    fillpatch(58,  "Car Horn", 0.57f, 0.49f, 0.31f, 0.0f, 0.35f, 0.5f, 0.46f, 0.0f, 0.68f, 0.0f, 0.5f, 0.46f, 0.3f, 1.0f, 0.23f, 0.3f, 0.5f, 1.0f, 0.31f, 1.0f, 0.38f, 0.0f, 0.5f, 0.5f);
    fillpatch(59,  "Helicopter", 0.0f, 0.25f, 0.5f, 0.0f, 0.35f, 0.5f, 0.08f, 0.36f, 0.69f, 1.0f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.96f, 0.5f, 1.0f, 0.92f, 0.97f, 0.5f, 1.0f, 0.0f, 0.5f);
    fillpatch(60,  "Arctic Wind", 0.0f, 0.25f, 0.5f, 0.0f, 0.35f, 0.5f, 0.16f, 0.85f, 0.5f, 0.28f, 0.5f, 0.37f, 0.3f, 0.0f, 0.25f, 0.89f, 0.5f, 1.0f, 0.89f, 0.24f, 0.5f, 1.0f, 1.0f, 0.5f);
    fillpatch(61,  "Thip", 1.0f, 0.37f, 0.51f, 0.0f, 0.35f, 0.5f, 0.0f, 1.0f, 0.97f, 0.0f, 0.5f, 0.02f, 0.2f, 0.0f, 0.2f, 0.0f, 0.46f, 0.0f, 0.3f, 0.81f, 0.5f, 0.78f, 0.48f, 0.5f);
    fillpatch(62,  "Synth Tom", 0.0f, 0.25f, 0.5f, 0.0f, 0.76f, 0.94f, 0.3f, 0.33f, 0.76f, 0.0f, 0.68f, 0.0f, 0.59f, 0.0f, 0.59f, 0.1f, 0.5f, 0.0f, 0.5f, 0.81f, 0.5f, 0.7f, 0.0f, 0.5f);
    fillpatch(63,  "Squelchy Frog", 0.5f, 0.41f, 0.23f, 0.45f, 0.77f, 0.0f, 0.4f, 0.65f, 0.95f, 0.0f, 0.5f, 0.33f, 0.5f, 0.0f, 0.25f, 0.0f, 0.7f, 0.65f, 0.18f, 0.32f, 1.0f, 0.0f, 0.06f, 0.5f);
    
    //for testing...
    //fillpatch(0, "Monosynth", 0.62f, 0.26f, 0.51f, 0.79f, 0.35f, 0.54f, 0.64f, 0.39f, 0.51f, 0.65f, 0.0f, 0.07f, 0.52f, 0.24f, 0.84f, 0.13f, 0.3f, 0.76f, 0.21f, 0.58f, 0.3f, 0.0f, 0.36f, 0.5f);

    setProgram(0);
  }
		
  setUniqueID("mdaJX10");

  if(audioMaster)
	{
		setNumInputs(0);				
		setNumOutputs(NOUTS);
		canProcessReplacing();
		isSynth();
	}

  //initialise...
  for(int32_t v=0; v<NVOICES; v++) 
  {
    voice[v].dp   = voice[v].dp2   = 1.0f;
    voice[v].saw  = voice[v].p     = voice[v].p2    = 0.0f;
    voice[v].env  = voice[v].envd  = voice[v].envl  = 0.0f;
    voice[v].fenv = voice[v].fenvd = voice[v].fenvl = 0.0f;
    voice[v].f0   = voice[v].f1    = voice[v].f2    = 0.0f;
    voice[v].note = 0;
  }
  lfo = modwhl = filtwhl = press = fzip = 0.0f; 
  rezwhl = pbend = ipbend = 1.0f;
  volume = 0.0005f;
  K = mode = lastnote = sustain = activevoices = 0;
  noise = 22222;

  update();
	suspend();
}


void mdaJX10::update()  //parameter change
{
  double ifs = 1.0 / Fs;
  float * param = programs[curProgram].param;

  mode = (int32_t)(7.9f * param[3]);
  noisemix = param[21] * param[21];
  voltrim = (3.2f - param[0] - 1.5f * noisemix) * (1.5f - 0.5f * param[7]);
  noisemix *= 0.06f;
  oscmix = param[0];

  semi = (float)floor(48.0f * param[1]) - 24.0f;
  cent = 15.876f * param[2] - 7.938f;
  cent = 0.1f * (float)floor(cent * cent * cent);
  detune = (float)pow(1.059463094359f, - semi - 0.01f * cent);
  tune = -23.376f - 2.0f * param[23] - 12.0f * (float)floor(param[22] * 4.9);
  tune = Fs * (float)pow(1.059463094359f, tune);

  vibrato = pwmdep = 0.2f * (param[20] - 0.5f) * (param[20] - 0.5f);
  if(param[20]<0.5f) vibrato = 0.0f;

  lfoHz = (float)exp(7.0f * param[19] - 4.0f);
  dlfo = lfoHz * (float)(ifs * TWOPI * KMAX); 

  filtf = 8.0f * param[6] - 1.5f;
  filtq  = (1.0f - param[7]) * (1.0f - param[7]); ////// + 0.02f;
  filtlfo = 2.5f * param[9] * param[9];
  filtenv = 12.0f * param[8] - 6.0f;
  filtvel = 0.1f * param[10] - 0.05f;
  if(param[10]<0.05f) { veloff = 1; filtvel = 0; } else veloff = 0;

  att = 1.0f - (float)exp(-ifs * exp(5.5 - 7.5 * param[15]));
  dec = 1.0f - (float)exp(-ifs * exp(5.5 - 7.5 * param[16]));
  sus = param[17];
  rel = 1.0f - (float)exp(-ifs * exp(5.5 - 7.5 * param[18]));
  if(param[18]<0.01f) rel = 0.1f; //extra fast release

  ifs *= KMAX; //lower update rate...

  fatt = 1.0f - (float)exp(-ifs * exp(5.5 - 7.5 * param[11]));
  fdec = 1.0f - (float)exp(-ifs * exp(5.5 - 7.5 * param[12]));
  fsus = param[13] * param[13];
  frel = 1.0f - (float)exp(-ifs * exp(5.5 - 7.5 * param[14]));

  if(param[4]<0.02f) glide = 1.0f; else
  glide = 1.0f - (float)exp(-ifs * exp(6.0 - 7.0 * param[4]));
  glidedisp = (6.604f * param[5] - 3.302f);
  glidedisp *= glidedisp * glidedisp;
}


void mdaJX10::setSampleRate(float rate)
{
	AudioEffectX::setSampleRate(rate);
  Fs = rate;
 
  dlfo = lfoHz * (float)(TWOPI * KMAX) / Fs; 
}


void mdaJX10::resume()
{	
  DECLARE_LVZ_DEPRECATED (wantEvents) ();
}


void mdaJX10::suspend() //Used by Logic (have note off code in 3 places now...)
{
  for(int32_t v=0; v<NVOICES; v++)
  {
    voice[v].envl = voice[v].env = 0.0f; 
    voice[v].envd = 0.99f;
    voice[v].note = 0;
    voice[v].f0 = voice[v].f1 = voice[v].f2 = 0.0f;
  }
}


mdaJX10::~mdaJX10()  //destroy any buffers...
{
  if(programs) delete[] programs;
}


void mdaJX10::setProgram(int32_t program)
{
	curProgram = program;
    update();
} //may want all notes off here - but this stops use of patches as snapshots!


void mdaJX10::setParameter(int32_t index, float value)
{
  programs[curProgram].param[index] = value;
  update();

  ///if(editor) editor->postUpdate();
}


void mdaJX10::fillpatch(int32_t p, const char *name,
                      float p0,  float p1,  float p2,  float p3,  float p4,  float p5, 
                      float p6,  float p7,  float p8,  float p9,  float p10, float p11,
                      float p12, float p13, float p14, float p15, float p16, float p17, 
                      float p18, float p19, float p20, float p21, float p22, float p23)
{
  strcpy(programs[p].name, name);
  programs[p].param[0]  = p0;   programs[p].param[1]  = p1;
  programs[p].param[2]  = p2;   programs[p].param[3]  = p3;
  programs[p].param[4]  = p4;   programs[p].param[5]  = p5;
  programs[p].param[6]  = p6;   programs[p].param[7]  = p7;
  programs[p].param[8]  = p8;   programs[p].param[9]  = p9;
  programs[p].param[10] = p10;  programs[p].param[11] = p11;
  programs[p].param[12] = p12;  programs[p].param[13] = p13;
  programs[p].param[14] = p14;  programs[p].param[15] = p15;
  programs[p].param[16] = p16;  programs[p].param[17] = p17;
  programs[p].param[18] = p18;  programs[p].param[19] = p19;
  programs[p].param[20] = p20;  programs[p].param[21] = p21;
  programs[p].param[22] = p22;  programs[p].param[23] = p23;  
}


float mdaJX10::getParameter(int32_t index)     { return programs[curProgram].param[index]; }
void  mdaJX10::setProgramName(char *name)   { strcpy(programs[curProgram].name, name); }
void  mdaJX10::getProgramName(char *name)   { strcpy(name, programs[curProgram].name); }
void  mdaJX10::setBlockSize(int32_t blockSize) {	AudioEffectX::setBlockSize(blockSize); }
bool  mdaJX10::getEffectName(char* name)    { strcpy(name, "MDA JX10 Synth"); return true; }
bool  mdaJX10::getVendorString(char* text)  {	strcpy(text, "MDA"); return true; }
bool  mdaJX10::getProductString(char* text) { strcpy(text, "MDA JX10 Synth"); return true; }


bool mdaJX10::getOutputProperties(int32_t index, LvzPinProperties* properties)
{
	if(index<NOUTS)
	{
		sprintf(properties->label, "JX10%d", index + 1);
		properties->flags = kLvzPinIsActive;
		if(index<2) properties->flags |= kLvzPinIsStereo; //make channel 1+2 stereo
		return true;
	}
	return false;
}


bool mdaJX10::getProgramNameIndexed(int32_t category, int32_t index, char* text)
{
	if ((unsigned int)index < NPROGS)
	{
		strcpy(text, programs[index].name);
		return true;
	}
	return false;
}


bool mdaJX10::copyProgram(int32_t destination)
{
  if(destination<NPROGS)
  {
    programs[destination] = programs[curProgram];
    return true;
  }
	return false;
}


int32_t mdaJX10::canDo(const char* text)
{
	if(!strcmp (text, "receiveLvzEvents")) return 1;
	if(!strcmp (text, "receiveLvzMidiEvent"))	return 1;
	return -1;
}


void mdaJX10::getParameterName(int32_t index, char *label)
{
	switch (index)
	{
		case  0: strcpy(label, "OSC Mix"); break;
		case  1: strcpy(label, "OSC Tune"); break;
		case  2: strcpy(label, "OSC Fine"); break;
		
    case  3: strcpy(label, "Glide"); break;
		case  4: strcpy(label, "Gld Rate"); break;
		case  5: strcpy(label, "Gld Bend"); break;
		
    case  6: strcpy(label, "VCF Freq"); break;
		case  7: strcpy(label, "VCF Reso"); break;
		case  8: strcpy(label, "VCF Env"); break;
		
    case  9: strcpy(label, "VCF LFO"); break;
 	  case 10: strcpy(label, "VCF Vel"); break;
 	  case 11: strcpy(label, "VCF Att"); break;

    case 12: strcpy(label, "VCF Dec"); break;
    case 13: strcpy(label, "VCF Sus"); break;
 	  case 14: strcpy(label, "VCF Rel"); break;
 	  
    case 15: strcpy(label, "ENV Att"); break;
		case 16: strcpy(label, "ENV Dec"); break;
		case 17: strcpy(label, "ENV Sus"); break;

		case 18: strcpy(label, "ENV Rel"); break;
		case 19: strcpy(label, "LFO Rate"); break;
		case 20: strcpy(label, "Vibrato"); break;

    case 21: strcpy(label, "Noise"); break;
    case 22: strcpy(label, "Octave"); break;
    default: strcpy(label, "Tuning");
	}
}


void mdaJX10::getParameterDisplay(int32_t index, char *text)
{
	char string[16];
	float * param = programs[curProgram].param;
  
  switch(index)
  {
    case  0: sprintf(string, "%4.0f:%2.0f", 100.0-50.0f*param[index], 50.0f*param[index]); break;
    case  1: sprintf(string, "%.0f", semi); break;
    case  2: sprintf(string, "%.1f", cent); break; 
    case  3: switch(mode)
             { case  0:
               case  1: strcpy(string, "POLY    "); break;
               case  2: strcpy(string, "P-LEGATO"); break;
               case  3: strcpy(string, "P-GLIDE "); break;
               case  4: 
               case  5: strcpy(string, "MONO    "); break;
               case  6: strcpy(string, "M-LEGATO"); break;
               default: strcpy(string, "M-GLIDE "); break; } break;
    case  5: sprintf(string, "%.2f", glidedisp); break;
    case  6: sprintf(string, "%.1f", 100.0f * param[index]); break; 
    case  8:
    case 23: sprintf(string, "%.1f", 200.0f * param[index] - 100.0f); break;
    case 10: if(param[index]<0.05f) strcpy(string, "   OFF  ");
               else sprintf(string, "%.0f", 200.0f * param[index] - 100.0f); break;
    case 19: sprintf(string, "%.3f", lfoHz); break;
    case 20: if(param[index]<0.5f) sprintf(string, "PWM %3.0f", 100.0f - 200.0f * param[index]);
               else sprintf(string, "%7.0f", 200.0f * param[index] - 100.0f); break;
    case 22: sprintf(string, "%d", (int32_t)(param[index] * 4.9f) - 2); break;
    default: sprintf(string, "%.0f", 100.0f * param[index]);
  }
	string[8] = 0;
	strcpy(text, (char *)string);
}


void mdaJX10::getParameterLabel(int32_t index, char *label)
{
  switch(index)
  {
    case  1: 
    case  5: strcpy(label, "   semi "); break;
    case  2: 
    case 23: strcpy(label, "   cent "); break;
    case  3: 
    case 22: strcpy(label, "        "); break;
    case 19: strcpy(label, "     Hz "); break;
    default: strcpy(label, "      % ");
  }
}

void mdaJX10::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float* out1 = outputs[0];
	float* out2 = outputs[1];
	int32_t frame=0, frames, v;
  float o, e, vib, pwm, pb=pbend, ipb=ipbend, gl=glide;
  float x, y, hpf=0.997f, min=1.0f, w=0.0f, ww=noisemix;
  float ff, fe=filtenv, fq=filtq * rezwhl, fx=1.97f-0.85f*fq, fz=fzip;
  int32_t k=K;
  unsigned int r;

  vib = (float)sin(lfo);
  ff = filtf + filtwhl + (filtlfo + press) * vib; //have to do again here as way that
  pwm = 1.0f + vib * (modwhl + pwmdep);           //below triggers on k was too cheap!
  vib = 1.0f + vib * (modwhl + vibrato);

  LV2_Atom_Event* ev = lv2_atom_sequence_begin(&eventInput->body);
  bool end = lv2_atom_sequence_is_end(&eventInput->body, eventInput->atom.size, ev);
  if(activevoices>0 || !end)
  {    
    while(frame<sampleFrames)
    {
      end = lv2_atom_sequence_is_end(&eventInput->body, eventInput->atom.size, ev);
      frames = end ? sampleFrames : ev->time.frames;
      frames -= frame;
      frame += frames;

      while(--frames>=0)
      {
        VOICE *V = voice;
        o = 0.0f;
        
        noise = (noise * 196314165) + 907633515;
        r = (noise & 0x7FFFFF) + 0x40000000; //generate noise + fast convert to float
        w = *(float *)&r;
        w = ww * (w - 3.0f);

        if(--k<0)
        {
          lfo += dlfo;
          if(lfo>PI) lfo -= TWOPI;
          vib = (float)sin(lfo);
          ff = filtf + filtwhl + (filtlfo + press) * vib;
          pwm = 1.0f + vib * (modwhl + pwmdep);
          vib = 1.0f + vib * (modwhl + vibrato);
          k = KMAX;
        }

        for(v=0; v<NVOICES; v++)  //for each voice
        { 
          e = V->env;
          if(e > SILENCE)
          { //Sinc-Loop Oscillator
            x = V->p + V->dp;
            if(x > min) 
            {
              if(x > V->pmax) 
              { 
                x = V->pmax + V->pmax - x;  
                V->dp = -V->dp; 
              }
              V->p = x;
              x = V->sin0 * V->sinx - V->sin1; //sine osc
              V->sin1 = V->sin0;
              V->sin0 = x;
              x = x / V->p;
            }
            else
            { 
              V->p = x = - x;  
              V->dp = V->period * vib * pb; //set period for next cycle
              V->pmax = (float)floor(0.5f + V->dp) - 0.5f;
              V->dc = -0.5f * V->lev / V->pmax;
              V->pmax *= PI;
              V->dp = V->pmax / V->dp;
              V->sin0 = V->lev * (float)sin(x);
              V->sin1 = V->lev * (float)sin(x - V->dp);
              V->sinx = 2.0f * (float)cos(V->dp);
              if(x*x > .1f) x = V->sin0 / x; else x = V->lev; //was 0.01f;
            }
            
            y = V->p2 + V->dp2; //osc2
            if(y > min) 
            { 
              if(y > V->pmax2) 
              { 
                y = V->pmax2 + V->pmax2 - y;  
                V->dp2 = -V->dp2; 
              }
              V->p2 = y;
              y = V->sin02 * V->sinx2 - V->sin12;
              V->sin12 = V->sin02;
              V->sin02 = y;
              y = y / V->p2;
            }
            else
            {
              V->p2 = y = - y;  
              V->dp2 = V->period * V->detune * pwm * pb;
              V->pmax2 = (float)floor(0.5f + V->dp2) - 0.5f;
              V->dc2 = -0.5f * V->lev2 / V->pmax2;
              V->pmax2 *= PI;
              V->dp2 = V->pmax2 / V->dp2;
              V->sin02 = V->lev2 * (float)sin(y);
              V->sin12 = V->lev2 * (float)sin(y - V->dp2);
              V->sinx2 = 2.0f * (float)cos(V->dp2);
              if(y*y > .1f) y = V->sin02 / y; else y = V->lev2;
            }
            V->saw = V->saw * hpf + V->dc + x - V->dc2 - y;  //integrated sinc = saw
            x = V->saw + w;
            V->env += V->envd * (V->envl - V->env);

            if(k==KMAX) //filter freq update at LFO rate
            {
              if((V->env+V->envl)>3.0f) { V->envd=dec; V->envl=sus; } //envelopes
              V->fenv += V->fenvd * (V->fenvl - V->fenv);
              if((V->fenv+V->fenvl)>3.0f) { V->fenvd=fdec; V->fenvl=fsus; }

              fz += 0.005f * (ff - fz); //filter zipper noise filter
              y = V->fc * (float)exp(fz + fe * V->fenv) * ipb; //filter cutoff
              if(y<0.005f) y=0.005f;
              V->ff = y;
 
              V->period += gl * (V->target - V->period); //glide
              if(V->target < V->period) V->period += gl * (V->target - V->period);
            }

            if(V->ff > fx) V->ff = fx; //stability limit
            
            V->f0 += V->ff * V->f1; //state-variable filter
            V->f1 -= V->ff * (V->f0 + fq * V->f1 - x - V->f2);
            V->f1 -= 0.2f * V->f1 * V->f1 * V->f1; //soft limit

            V->f2 = x;
            
            o += V->env * V->f0;
          }
          V++;
        }

        *out1++ = o;
        *out2++ = o;
      }

      if(!end)
      {
        processEvent(ev);
        ev = lv2_atom_sequence_next(ev);
      }
    }
  
    activevoices = NVOICES;
    for(v=0; v<NVOICES; v++)
    {
      if(voice[v].env<SILENCE)  //choke voices
      {
        voice[v].env = voice[v].envl = 0.0f;
        voice[v].f0 = voice[v].f1 = voice[v].f2 = 0.0f;
        activevoices--;
      }
    }
  }
  else //empty block
  {
		while(--sampleFrames >= 0)
		{
			*out1++ = 0.0f;
			*out2++ = 0.0f;
		}
  }
  fzip = fz;
  K = k;
}


void mdaJX10::noteOn(int32_t note, int32_t velocity)
{
  float p, l=100.0f; //louder than any envelope!
  int32_t  v=0, tmp, held=0;
  
  if(velocity>0) //note on
  {
    if(veloff) velocity = 80;
    
    if(mode & 4) //monophonic
    {
      if(voice[0].note > 0) //legato pitch change
      {
        for(tmp=(NVOICES-1); tmp>0; tmp--)  //queue any held notes
        {
          voice[tmp].note = voice[tmp - 1].note;
        }
        p = tune * (float)exp(-0.05776226505 * ((double)note + ANALOG * (double)v));
        while(p<3.0f || (p * detune)<3.0f) p += p;
        voice[v].target = p;
        if((mode & 2)==0) voice[v].period = p;
        voice[v].fc = (float)exp(filtvel * (float)(velocity - 64)) / p;
        voice[v].env += SILENCE + SILENCE; ///was missed out below if returned?
        voice[v].note = note;
        return;
      }
    }
    else //polyphonic 
    {
      for(tmp=0; tmp<NVOICES; tmp++)  //replace quietest voice not in attack
      {
        if(voice[tmp].note > 0) held++;
        if(voice[tmp].env<l && voice[tmp].envl<2.0f) { l=voice[tmp].env;  v=tmp; }
      }
    }  
    p = tune * (float)exp(-0.05776226505 * ((double)note + ANALOG * (double)v));
    while(p<3.0f || (p * detune)<3.0f) p += p;
    voice[v].target = p;
    voice[v].detune = detune;
  
    tmp = 0;
    if(mode & 2)
    {
      if((mode & 1) || held) tmp = note - lastnote; //glide
    }
    voice[v].period = p * (float)pow(1.059463094359, (double)tmp - glidedisp);
    if(voice[v].period<3.0f) voice[v].period = 3.0f; //limit min period

    voice[v].note = lastnote = note;

    voice[v].fc = (float)exp(filtvel * (float)(velocity - 64)) / p; //filter tracking

    voice[v].lev = voltrim * volume * (0.004f * (float)((velocity + 64) * (velocity + 64)) - 8.0f);
    voice[v].lev2 = voice[v].lev * oscmix;

    if(programs[curProgram].param[20]<0.5f) //force 180 deg phase difference for PWM
    {
      if(voice[v].dp>0.0f)
      {
        p = voice[v].pmax + voice[v].pmax - voice[v].p;
        voice[v].dp2 = -voice[v].dp;
      }
      else
      {
        p = voice[v].p;
        voice[v].dp2 = voice[v].dp;
      }
      voice[v].p2 = voice[v].pmax2 = p + PI * voice[v].period;

      voice[v].dc2 = 0.0f;
      voice[v].sin02 = voice[v].sin12 = voice[v].sinx2 = 0.0f;
    }

    if(mode & 4) //monophonic retriggering
    {
      voice[v].env += SILENCE + SILENCE;
    }
    else
    {
      //if(programs[curProgram].param[15] < 0.28f) 
      //{
      //  voice[v].f0 = voice[v].f1 = voice[v].f2 = 0.0f; //reset filter
      //  voice[v].env = SILENCE + SILENCE;
      //  voice[v].fenv = 0.0f;
      //}
      //else 
        voice[v].env += SILENCE + SILENCE; //anti-glitching trick
    }
    voice[v].envl  = 2.0f;
    voice[v].envd  = att;
    voice[v].fenvl = 2.0f;
    voice[v].fenvd = fatt;
  }
  else //note off
  {
    if((mode & 4) && (voice[0].note==note)) //monophonic (and current note)
    {
      for(v=(NVOICES-1); v>0; v--) 
      {
        if(voice[v].note>0) held = v; //any other notes queued?
      }
      if(held>0)
      {
        voice[v].note = voice[held].note;
        voice[held].note = 0;
        
        p = tune * (float)exp(-0.05776226505 * ((double)voice[v].note + ANALOG * (double)v));
        while(p<3.0f || (p * detune)<3.0f) p += p;
        voice[v].target = p;
        if((mode & 2)==0) voice[v].period = p;
        voice[v].fc = 1.0f / p;
      }
      else
      {
        voice[v].envl  = 0.0f;
        voice[v].envd  = rel;
        voice[v].fenvl = 0.0f;
        voice[v].fenvd = frel;
        voice[v].note  = 0;
      }
    }
    else //polyphonic
    {
      for(v=0; v<NVOICES; v++) if(voice[v].note==note) //any voices playing that note?
      {
        if(sustain==0)
        {
          voice[v].envl  = 0.0f;
          voice[v].envd  = rel;
          voice[v].fenvl = 0.0f;
          voice[v].fenvd = frel;
          voice[v].note  = 0;
        }
        else voice[v].note = SUSTAIN;
      }
    }
  }
}


int32_t mdaJX10::processEvent(const LV2_Atom_Event* ev)
{
  if (ev->body.type != midiEventType)
      return 0;

  const uint8_t* midiData = (const uint8_t*)LV2_ATOM_BODY(&ev->body);
		
    switch(midiData[0] & 0xf0) //status byte (all channels)
    {
      case 0x80: //note off
        noteOn(midiData[1] & 0x7F, 0);
        break;

      case 0x90: //note on
        noteOn(midiData[1] & 0x7F, midiData[2] & 0x7F);
        break;

      case 0xB0: //controller
        switch(midiData[1])
        {
          case 0x01:  //mod wheel
            modwhl = 0.000005f * (float)(midiData[2] * midiData[2]);
            break;
          case 0x02:  //filter +
          case 0x4A:
            filtwhl = 0.02f * (float)(midiData[2]);
            break;
          case 0x03:  //filter -
            filtwhl = -0.03f * (float)(midiData[2]);
            break;

          case 0x07:  //volume
            volume = 0.00000005f * (float)(midiData[2] * midiData[2]);
            break;

          case 0x10:  //resonance
          case 0x47:
            rezwhl = 0.0065f * (float)(154 - midiData[2]);
            break;

          case 0x40:  //sustain
            sustain = midiData[2] & 0x40;
            if(sustain==0)
            {
              noteOn(SUSTAIN, 0);
            }
            break;

          default:  //all notes off
            if(midiData[1]>0x7A) 
            {  
              for(int32_t v=0; v<NVOICES; v++) 
              { 
                voice[v].envl = voice[v].env = 0.0f; 
                voice[v].envd = 0.99f;
                voice[v].note = 0;
                //could probably reset some more stuff here for safety!
              }
              sustain = 0;
            }
            break;
        }
        break;

      case 0xC0: //program change
        if(midiData[1]<NPROGS) setProgram(midiData[1]);
        break;

      case 0xD0: //channel aftertouch
        press = 0.00001f * (float)(midiData[1] * midiData[1]);
        break;
      
      case 0xE0: //pitch bend
        ipbend = (float)exp(0.000014102 * (double)(midiData[1] + 128 * midiData[2] - 8192));
        pbend = 1.0f / ipbend;
        break;
      
      default: break;
    }

	return 1;
}

