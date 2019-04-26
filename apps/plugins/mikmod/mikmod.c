/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "mikmod_build.h"

PLUGIN_HEADER

struct plugin_api *rb;

MODULE *module;

#define MAXPCMBUFFERS 16 // Do NOT set this to a value lower than 2 or playback will break
#define PCMBUFFERSIZE 65536

char *pcmbufs[MAXPCMBUFFERS];
int bufferlen[MAXPCMBUFFERS];
int buffercount = 0;

int curbuff = 0;
int lastbuff = 0;

size_t buffrendered = 0;
size_t buffplayed = 0;
size_t buff_last = ~0;

int vol, minvol, maxvol;
int lastpower = 0, lastvol = 0;
int lastbutton = 0;

char sbuf[32];

int mallocbufsize;
char *mbuf;


void mikmod_prepare_malloc(char *buff, int bufsize);
long mikmod_get_malloc_usage(void);
void* mikmod_malloc(size_t size);



int FindLastOccurrenceChar(char *string, char chr)
{
	int i = 0;
	//int LastOcc = -1;
	int LastOcc = 0;

	while (string[i] != 0)
	{
		if (string[i] == chr)
		{
			LastOcc = i;
		}

		i++;
	}

	return LastOcc;
}


void get_more(unsigned char **start, size_t *size)
	{
		if (curbuff >= (buffercount - 1))
		{
			curbuff = 0;
		}
		else
		{
			curbuff++;
		}
		*start = pcmbufs[curbuff];
		*size  = bufferlen[curbuff];
		buffplayed++;
	}


void checkbutton(void)
{
	lastbutton = rb->button_get(false);
	if (lastbutton & BUTTON_MENU)
	{
		return;
	}

	switch (lastbutton)
	{
		case BUTTON_SCROLL_FWD:
		case (BUTTON_SCROLL_FWD | BUTTON_REPEAT):
			vol = rb->global_settings->volume;

			if (vol < maxvol)
			{
				vol++;
				rb->sound_set(SOUND_VOLUME, vol);
				rb->global_settings->volume = vol;
			}
			break;

		case BUTTON_SCROLL_BACK:
		case (BUTTON_SCROLL_BACK | BUTTON_REPEAT):
			vol = rb->global_settings->volume;

			if (vol > minvol)
			{
				vol--;
				rb->sound_set(SOUND_VOLUME, vol);
				rb->global_settings->volume = vol;
			}
			break;

		case (BUTTON_PLAY | BUTTON_REL):

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
	rb->cpu_boost(false); // Just in case we got called from inside the render loop
#endif
			rb->pcm_play_pause(false);
			do /* code from mpegplayer, *very* simple unpause loop */
			{
				lastbutton = rb->button_get(true);
				if (lastbutton == BUTTON_MENU)
				{
					return;
				}
			} while (lastbutton != (BUTTON_PLAY | BUTTON_REL));
			rb->pcm_play_pause(true);
			break;

		default:
			if(rb->default_event_handler(lastbutton) == SYS_USB_CONNECTED)
			{
				return;
			}
	} /* switch(button) */

	if ((rb->global_settings->volume != lastvol) || (rb->battery_level() != lastpower))
	{
		lastpower = rb->battery_level();
		lastvol = rb->global_settings->volume;
		
		rb->snprintf(sbuf, sizeof(sbuf) - 1, "%d dB   ", rb->global_settings->volume);
		rb->lcd_puts(5, 3, sbuf);
		rb->snprintf(sbuf, sizeof(sbuf) - 1, "%d %%   ", rb->battery_level());
		rb->lcd_puts(5, 4, sbuf);

		rb->lcd_update();
	}
	lastbutton = 0;
}


void mainloop(void)
{
	int i;

	minvol = rb->sound_min(SOUND_VOLUME);
	maxvol = rb->sound_max(SOUND_VOLUME);

	while (buffplayed <= buff_last && !lastbutton)
	{
		while (curbuff != lastbuff && !lastbutton) // Render loop, fills up all buffers except the one being played
		{

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
	rb->cpu_boost(true);
#endif

			checkbutton();

			if (Player_Active())
			{
				bufferlen[lastbuff] = VC_WriteBytes(pcmbufs[lastbuff], PCMBUFFERSIZE);
				buffrendered++;
			}
			else
			{
				// No more data to render (songend) - clear the next buffer so get_more() can return it a last time
				for (i = 0; i < PCMBUFFERSIZE; i++)
					(pcmbufs[lastbuff])[i] = 0;
				bufferlen[lastbuff] = PCMBUFFERSIZE;
				buff_last = buffrendered;
			}

			if (lastbuff >= (buffercount - 1))
				lastbuff = 0;
			else
				lastbuff++;

			checkbutton();
			rb->reset_poweroff_timer();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
	rb->cpu_boost(false);
#endif

		}

		checkbutton();

		rb->yield();

	} /* while */

} /* mainloop */




enum plugin_status plugin_start(struct plugin_api *api, void *parameter)
{
	bool talk_menu;
	int i;

	rb = api;

	/* Have to shut up voice menus or it will mess up our waveform playback */
	talk_menu = rb->global_settings->talk_menu;
	rb->global_settings->talk_menu = false;

	rb->audio_stop();
	rb->pcm_play_stop();
	rb->pcm_set_frequency(44100);

	// Initialize internal semi-dynamic memory
	mbuf = rb->plugin_get_audio_buffer(&mallocbufsize);
	mikmod_prepare_malloc(mbuf, mallocbufsize);


	// Allocate PCM output buffers
	for (i = 0; i < MAXPCMBUFFERS; i++)
	{
		if ((pcmbufs[i] = mikmod_malloc(PCMBUFFERSIZE)) == NULL)
		{
			break;
		}
	}
	buffercount = i;
	//lastbuff = buffercount - 1;
	// Prefill buffers !


	// General output...
	rb->lcd_clear_display();
	rb->lcd_puts(0, 0, " MikMod for Rockbox 0.1");
	rb->lcd_puts(0, 1, "========================");

	rb->lcd_puts(0, 3, "Vol: -");
	rb->lcd_puts(0, 4, "Pwr: -");
	rb->lcd_puts(0, 5, "---------");
	rb->lcd_puts(0, 6, "File:");
	rb->lcd_puts(7, 6, &((char*)parameter)[FindLastOccurrenceChar(parameter, '/') + 1]);

	rb->lcd_update();

	//initialize the library
	MikMod_RegisterAllLoaders();
	md_mode = DMODE_SOFT_MUSIC | DMODE_16BITS | DMODE_STEREO | DMODE_INTERP | DMODE_SOFT_SNDFX;

	if (!MikMod_Init(""))
	{
		#ifdef HAVE_ADJUSTABLE_CPU_FREQ
			rb->cpu_boost(true);
		#endif
		//load module
		module = Player_Load(parameter, 32, 1);

		#ifdef HAVE_ADJUSTABLE_CPU_FREQ
			rb->cpu_boost(false);
		#endif

		if (module)
		{
			Player_Start(module);
#ifndef SIMULATOR
			rb->ata_sleep();
#endif

			rb->snprintf(sbuf, sizeof(sbuf) - 1, "Memory: %d / %d", (int)(mikmod_get_malloc_usage() - (buffercount * PCMBUFFERSIZE)), mallocbufsize);
			rb->lcd_puts(0, 16, sbuf);

			rb->lcd_puts(0, 7, "Type:");
			rb->lcd_puts(7, 7, module->modtype);

			rb->lcd_puts(0, 9, "Title:");
			rb->lcd_puts_scroll(7, 9, module->songname);

			rb->lcd_update();

			// Only prefill the first buffer so we get a quick start...
			bufferlen[0] = VC_WriteBytes(pcmbufs[0], PCMBUFFERSIZE);

			curbuff = buffercount - 1;
			lastbuff = 1; // Fixes skip during first buffers played

			rb->pcm_play_data(get_more, NULL, 0);

			mainloop();

		}
		else /* module */
		{
			rb->splash(HZ*2, "Could not load module, reason:");
			rb->splash(HZ*2, MikMod_strerror(MikMod_errno));
		}
	}
	else /* MikMod_Init */
	{
		rb->splash(HZ*2, "Could not initialize sound, reason:");
		rb->splash(HZ*2, MikMod_strerror(MikMod_errno));
	}


	#ifdef HAVE_ADJUSTABLE_CPU_FREQ
		rb->cpu_boost(false);
	#endif

	rb->pcm_play_stop();

	Player_Stop();
	Player_Free(module);
	MikMod_Exit();


	//restore default - user of apis is responsible for restoring
	//   default state - normally playback at 44100Hz
	rb->pcm_set_frequency(HW_SAMPR_DEFAULT);
	rb->global_settings->talk_menu = talk_menu;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
	rb->cpu_boost(false);
#endif

	return PLUGIN_OK;
}
