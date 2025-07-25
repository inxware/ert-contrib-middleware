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

#ifndef __ALSA_PCM_IOPLUG_H
#define __ALSA_PCM_IOPLUG_H

/** hw constraints for ioplug */
enum {
	SND_PCM_IOPLUG_HW_ACCESS = 0,	/**< access type */
	SND_PCM_IOPLUG_HW_FORMAT,	/**< format */
	SND_PCM_IOPLUG_HW_CHANNELS,	/**< channels */
	SND_PCM_IOPLUG_HW_RATE,		/**< rate */
	SND_PCM_IOPLUG_HW_PERIOD_BYTES,	/**< period bytes */
	SND_PCM_IOPLUG_HW_BUFFER_BYTES,	/**< buffer bytes */
	SND_PCM_IOPLUG_HW_PERIODS,	/**< number of periods */
	SND_PCM_IOPLUG_HW_PARAMS	/**< max number of hw constraints */
};
	
typedef struct snd_pcm_ioplug snd_pcm_ioplug_t;
typedef struct snd_pcm_ioplug_callback snd_pcm_ioplug_callback_t;

/**
 * bit flags for additional conditions
 */
#define SND_PCM_IOPLUG_FLAG_LISTED	(1<<0)		/* list up this PCM */

/**
 * Protocol version
 */
#define SND_PCM_IOPLUG_VERSION_MAJOR	1
#define SND_PCM_IOPLUG_VERSION_MINOR	0
#define SND_PCM_IOPLUG_VERSION_TINY	0
#define SND_PCM_IOPLUG_VERSION		((SND_PCM_IOPLUG_VERSION_MAJOR<<16) |\
					 (SND_PCM_IOPLUG_VERSION_MINOR<<8) |\
					 (SND_PCM_IOPLUG_VERSION_TINY))

/** handle of ioplug */
struct snd_pcm_ioplug {
	/**
	 * protocol version; SND_PCM_IOPLUG_VERSION must be filled here
	 * before calling #snd_pcm_ioplug_create()
	 */
	unsigned int version;
	/**
	 * name of this plugin; must be filled before calling #snd_pcm_ioplug_create()
	 */
	const char *name;
	unsigned int flags;	/**< SND_PCM_IOPLUG_FLAG_XXX */
	int poll_fd;		/**< poll file descriptor */
	unsigned int poll_events;	/**< poll events */
	unsigned int mmap_rw;		/**< pseudo mmap mode */
	/**
	 * callbacks of this plugin; must be filled before calling #snd_pcm_ioplug_create()
	 */
	const snd_pcm_ioplug_callback_t *callback;
	/**
	 * private data, which can be used freely in the driver callbacks
	 */
	void *private_data;
	/**
	 * PCM handle filled by #snd_pcm_extplug_create()
	 */
	snd_pcm_t *pcm;

	snd_pcm_stream_t stream;	/* stream direcion; read-only */	
	snd_pcm_state_t state;		/* current PCM state; read-only */
	volatile snd_pcm_uframes_t appl_ptr;	/* application pointer; read-only */
	volatile snd_pcm_uframes_t hw_ptr;	/* hw pointer; read-only */
	int nonblock;			/* non-block mode; read-only */

	snd_pcm_access_t access;	/* access type; filled after hw_params is called */
	snd_pcm_format_t format;	/* format; filled after hw_params is called */
	unsigned int channels;		/* channels; filled after hw_params is called */
	unsigned int rate;		/* rate; filled after hw_params is called */
	snd_pcm_uframes_t period_size;	/* period size; filled after hw_params is called */
	snd_pcm_uframes_t buffer_size;	/* buffer size; filled after hw_params is called */
};

/** callback table of ioplug */
struct snd_pcm_ioplug_callback {
	/**
	 * start the PCM; required
	 */
	int (*start)(snd_pcm_ioplug_t *io);
	/**
	 * stop the PCM; required
	 */
	int (*stop)(snd_pcm_ioplug_t *io);
	/**
	 * get the current DMA position; required
	 */
	snd_pcm_sframes_t (*pointer)(snd_pcm_ioplug_t *io);
	/**
	 * transfer the data; optional
	 */
	snd_pcm_sframes_t (*transfer)(snd_pcm_ioplug_t *io,
				      const snd_pcm_channel_area_t *areas,
				      snd_pcm_uframes_t offset,
				      snd_pcm_uframes_t size);
	/**
	 * close the PCM; optional
	 */
	int (*close)(snd_pcm_ioplug_t *io);
	/**
	 * hw_params; optional
	 */
	int (*hw_params)(snd_pcm_ioplug_t *io, snd_pcm_hw_params_t *params);
	/**
	 * hw_free; optional
	 */
	int (*hw_free)(snd_pcm_ioplug_t *io);
	/**
	 * sw_params; optional
	 */
	int (*sw_params)(snd_pcm_ioplug_t *io, snd_pcm_sw_params_t *params);
	/**
	 * prepare; optional
	 */
	int (*prepare)(snd_pcm_ioplug_t *io);
	/**
	 * drain; optional
	 */
	int (*drain)(snd_pcm_ioplug_t *io);
	/**
	 * toggle pause; optional
	 */
	int (*pause)(snd_pcm_ioplug_t *io, int enable);
	/**
	 * resume; optional
	 */
	int (*resume)(snd_pcm_ioplug_t *io);
	/**
	 * poll descriptors count; optional
	 */
	int (*poll_descriptors_count)(snd_pcm_ioplug_t *io);
	/**
	 * poll descriptors; optional
	 */
	int (*poll_descriptors)(snd_pcm_ioplug_t *io, struct pollfd *pfd, unsigned int space);
	/**
	 * mangle poll events; optional
	 */
	int (*poll_revents)(snd_pcm_ioplug_t *io, struct pollfd *pfd, unsigned int nfds, unsigned short *revents);
	/**
	 * dump; optional
	 */
	void (*dump)(snd_pcm_ioplug_t *io, snd_output_t *out);
};


int snd_pcm_ioplug_create(snd_pcm_ioplug_t *io, const char *name,
			  snd_pcm_stream_t stream, int mode);
int snd_pcm_ioplug_delete(snd_pcm_ioplug_t *io);

/* update poll_fd and mmap_rw */
int snd_pcm_ioplug_reinit_status(snd_pcm_ioplug_t *ioplug);

/* get a mmap area (for mmap_rw only) */
const snd_pcm_channel_area_t *snd_pcm_ioplug_mmap_areas(snd_pcm_ioplug_t *ioplug);

/* clear hw_parameter setting */
void snd_pcm_ioplug_params_reset(snd_pcm_ioplug_t *io);

/* hw_parameter setting */
int snd_pcm_ioplug_set_param_minmax(snd_pcm_ioplug_t *io, int type, unsigned int min, unsigned int max);
int snd_pcm_ioplug_set_param_list(snd_pcm_ioplug_t *io, int type, unsigned int num_list, const unsigned int *list);

#endif /* __ALSA_PCM_IOPLUG_H */
