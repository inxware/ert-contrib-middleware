/*
 * ALSA external PCM plugin SDK (draft version)
 *
 * Copyright (c) 2005 Takashi Iwai <tiwai@suse.de>
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __ALSA_PCM_EXTPLUG_H
#define __ALSA_PCM_EXTPLUG_H

/** hw constraints for extplug */
enum {
	SND_PCM_EXTPLUG_HW_FORMAT,	/**< format */
	SND_PCM_EXTPLUG_HW_CHANNELS,	/**< channels */
	SND_PCM_EXTPLUG_HW_PARAMS	/**< max number of hw constraints */
};
	
typedef struct snd_pcm_extplug snd_pcm_extplug_t;
typedef struct snd_pcm_extplug_callback snd_pcm_extplug_callback_t;

/**
 * Protocol version
 */
#define SND_PCM_EXTPLUG_VERSION_MAJOR	1
#define SND_PCM_EXTPLUG_VERSION_MINOR	0
#define SND_PCM_EXTPLUG_VERSION_TINY	0
#define SND_PCM_EXTPLUG_VERSION		((SND_PCM_EXTPLUG_VERSION_MAJOR<<16) |\
					 (SND_PCM_EXTPLUG_VERSION_MINOR<<8) |\
					 (SND_PCM_EXTPLUG_VERSION_TINY))

/** handle of extplug */
struct snd_pcm_extplug {
	/**
	 * protocol version; SND_PCM_EXTPLUG_VERSION must be filled here
	 * before calling #snd_pcm_extplug_create()
	 */
	unsigned int version;
	/**
	 * name of this plugin; must be filled before calling #snd_pcm_extplug_create()
	 */
	const char *name;
	/**
	 * callbacks of this plugin; must be filled before calling #snd_pcm_extplug_create()
	 */
	const snd_pcm_extplug_callback_t *callback;
	/**
	 * private data, which can be used freely in the driver callbacks
	 */
	void *private_data;
	/**
	 * PCM handle filled by #snd_pcm_extplug_create()
	 */
	snd_pcm_t *pcm;
	/**
	 * stream direction; read-only status
	 */
	snd_pcm_stream_t stream;
	/**
	 * format hw parameter; filled after hw_params is caled
	 */
	snd_pcm_format_t format;
	/**
	 * subformat hw parameter; filled after hw_params is caled
	 */
	snd_pcm_subformat_t subformat;
	/**
	 * channels hw parameter; filled after hw_params is caled
	 */
	unsigned int channels;
	/**
	 * rate hw parameter; filled after hw_params is caled
	 */
	unsigned int rate;
	/**
	 * slave_format hw parameter; filled after hw_params is caled
	 */
	snd_pcm_format_t slave_format;
	/**
	 * slave_subformat hw parameter; filled after hw_params is caled
	 */
	snd_pcm_subformat_t slave_subformat;
	/**
	 * slave_channels hw parameter; filled after hw_params is caled
	 */
	unsigned int slave_channels;
};

/** callback table of extplug */
struct snd_pcm_extplug_callback {
	/**
	 * transfer between source and destination; this is a required callback
	 */
	snd_pcm_sframes_t (*transfer)(snd_pcm_extplug_t *ext,
				      const snd_pcm_channel_area_t *dst_areas,
				      snd_pcm_uframes_t dst_offset,
				      const snd_pcm_channel_area_t *src_areas,
				      snd_pcm_uframes_t src_offset,
				      snd_pcm_uframes_t size);
	/**
	 * close the PCM; optional
	 */
	int (*close)(snd_pcm_extplug_t *ext);
	/**
	 * hw_params; optional
	 */
	int (*hw_params)(snd_pcm_extplug_t *ext, snd_pcm_hw_params_t *params);
	/**
	 * hw_free; optional
	 */
	int (*hw_free)(snd_pcm_extplug_t *ext);
	/**
	 * dump; optional
	 */
	void (*dump)(snd_pcm_extplug_t *ext, snd_output_t *out);
};


int snd_pcm_extplug_create(snd_pcm_extplug_t *ext, const char *name,
			   snd_config_t *root, snd_config_t *slave_conf,
			   snd_pcm_stream_t stream, int mode);
int snd_pcm_extplug_delete(snd_pcm_extplug_t *ext);

/* clear hw_parameter setting */
void snd_pcm_extplug_params_reset(snd_pcm_extplug_t *ext);

/* hw_parameter setting */
int snd_pcm_extplug_set_param_list(snd_pcm_extplug_t *extplug, int type, unsigned int num_list, const unsigned int *list);
int snd_pcm_extplug_set_param_minmax(snd_pcm_extplug_t *extplug, int type, unsigned int min, unsigned int max);
int snd_pcm_extplug_set_slave_param_list(snd_pcm_extplug_t *extplug, int type, unsigned int num_list, const unsigned int *list);
int snd_pcm_extplug_set_slave_param_minmax(snd_pcm_extplug_t *extplug, int type, unsigned int min, unsigned int max);

static inline int snd_pcm_extplug_set_param(snd_pcm_extplug_t *extplug, int type, unsigned int val)
{
	return snd_pcm_extplug_set_param_list(extplug, type, 1, &val);
}

static inline int snd_pcm_extplug_set_slave_param(snd_pcm_extplug_t *extplug, int type, unsigned int val)
{
	return snd_pcm_extplug_set_slave_param_list(extplug, type, 1, &val);
}


#endif /* __ALSA_PCM_EXTPLUG_H */
