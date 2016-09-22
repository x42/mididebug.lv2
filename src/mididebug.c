/* midi event generator
 *
 * Copyright (C) 2016 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#define MDEBUG_URI "http://gareus.org/oss/lv2/mididebug"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>

typedef struct {
	/* ports */
	LV2_Atom_Sequence* midiout;
	float* p_nbytes;
	float* p_b[3];
	float* p_trigger;

	/* Cached Ports */
	bool c_trigger;

	/* atom-forge and URI mapping */
	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Frame frame;
	LV2_URID midi_MidiEvent;

	/* LV2 Output */
	LV2_Log_Log* log;
	LV2_Log_Logger logger;

} MDebug;

/**
 * add a midi message to the output port
 */
static void
forge_midimessage (MDebug* self,
                   uint32_t tme,
                   const uint8_t* const buffer,
                   uint32_t size)
{
	LV2_Atom midiatom;
	midiatom.type = self->midi_MidiEvent;
	midiatom.size = size;

	if (0 == lv2_atom_forge_frame_time (&self->forge, tme)) return;
	if (0 == lv2_atom_forge_raw (&self->forge, &midiatom, sizeof (LV2_Atom))) return;
	if (0 == lv2_atom_forge_raw (&self->forge, buffer, size)) return;
	lv2_atom_forge_pad (&self->forge, sizeof (LV2_Atom) + size);
}

/* *****************************************************************************
 * LV2 Plugin
 */

static LV2_Handle
instantiate (const LV2_Descriptor*     descriptor,
             double                    rate,
             const char*               bundle_path,
             const LV2_Feature* const* features)
{
	MDebug* self = (MDebug*)calloc (1, sizeof (MDebug));
	LV2_URID_Map* map = NULL;

	int i;
	for (i=0; features[i]; ++i) {
		if (!strcmp (features[i]->URI, LV2_URID__map)) {
			map = (LV2_URID_Map*)features[i]->data;
		} else if (!strcmp (features[i]->URI, LV2_LOG__log)) {
			self->log = (LV2_Log_Log*)features[i]->data;
		}
	}

	lv2_log_logger_init (&self->logger, map, self->log);

	if (!map) {
		lv2_log_error (&self->logger, "MDebug.lv2 error: Host does not support urid:map\n");
		free (self);
		return NULL;
	}

	lv2_atom_forge_init (&self->forge, map);
	self->midi_MidiEvent = map->map (map->handle, LV2_MIDI__MidiEvent);

	return (LV2_Handle)self;
}

static void
connect_port (LV2_Handle instance,
              uint32_t   port,
              void*      data)
{
	MDebug* self = (MDebug*)instance;

	switch (port) {
		case 0:
			self->midiout = (LV2_Atom_Sequence*)data;
			break;
		case 1:
			self->p_nbytes = (float*)data;
			break;
		case 2:
			self->p_b[0] = (float*)data;
			break;
		case 3:
			self->p_b[1] = (float*)data;
			break;
		case 4:
			self->p_b[2] = (float*)data;
			break;
		case 5:
			self->p_trigger = (float*)data;
			break;
		default:
			break;
	}
}


static void
run (LV2_Handle instance, uint32_t n_samples)
{
	MDebug* self = (MDebug*)instance;
	if (!self->midiout) {
		return;
	}

	/* initialize output port */
	const uint32_t capacity = self->midiout->atom.size;
	lv2_atom_forge_set_buffer (&self->forge, (uint8_t*)self->midiout, capacity);
	lv2_atom_forge_sequence_head (&self->forge, &self->frame, 0);

	if (*self->p_trigger > 0 && !self->c_trigger) {
		uint32_t nb = *self->p_nbytes;
		uint8_t b[3];
		b[0] = (int)rint (*self->p_b[0]) & 0xff;
		b[1] = (int)rint (*self->p_b[1]) & 0x7f;
		b[2] = (int)rint (*self->p_b[2]) & 0x7f;
		if (nb > 3) nb = 3;
		forge_midimessage (self, 0, b, nb);
	}
	self->c_trigger = *self->p_trigger > 0;
}

static void
cleanup (LV2_Handle instance)
{
	free (instance);
}

static const void*
extension_data (const char* uri)
{
	return NULL;
}

static const LV2_Descriptor descriptor = {
	MDEBUG_URI,
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	extension_data
};

#undef LV2_SYMBOL_EXPORT
#ifdef _WIN32
#    define LV2_SYMBOL_EXPORT __declspec(dllexport)
#else
#    define LV2_SYMBOL_EXPORT  __attribute__ ((visibility ("default")))
#endif
LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor (uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
