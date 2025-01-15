/*
 *  ALSAmmapAudioDriver.cpp
 *
 *  Copyright (c) 2001-2023 Nick Dowell
 *  Copyright (c) 2002, Karsten Wiese
 *
 *  This file is part of amsynth.
 *
 *  amsynth is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  amsynth is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with amsynth.  If not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstddef>

#include "ALSAmmapAudioDriver.h"

#ifdef WITH_ALSA

#include "core/Configuration.h"
#include "AudioDriver.h"

#include <alsa/asoundlib.h>
#include <iostream>


class ALSAmmapAudioDriver : public AudioDriver {
public:
    ALSAmmapAudioDriver();
    ~ALSAmmapAudioDriver() override;
    int	open() override;
    void close() override;
    int	write(float *buffer, int frames) override;

private:
    int 	xrun_recovery();

    unsigned int _rate;
    int		_channels;
    snd_pcm_t		*playback_handle;
    snd_pcm_hw_params_t	*hw_params;
    int			err;
    unsigned		periods;
};


int
ALSAmmapAudioDriver::xrun_recovery()
{
        if (err == -EPIPE) {    /* under-run */
				periods = 0;
                err = snd_pcm_prepare(playback_handle);
                if (err < 0){
                        std::cerr << "Can't recovery from underrun, prepare failed: " << snd_strerror(err) << "\n";
return -1;
				}
                return 0;
        } else if (err == -ESTRPIPE) {
                while ((err = snd_pcm_resume(playback_handle)) == -EAGAIN)
                        sleep(1);       /* wait until the suspend flag is released */
                if (err < 0) {
						periods = 0;
                        err = snd_pcm_prepare(playback_handle);
                        if (err < 0){
                                std::cerr << "Can't recovery from suspend, prepare failed: " << snd_strerror(err) << "\n";
return -1;
						}
                }
                return 0;
        }
	return 0;
}

int
ALSAmmapAudioDriver::write(float *buffer, int frames)
{
	Configuration & config = Configuration::get();

	int i;
	snd_pcm_sframes_t avail;
	snd_pcm_uframes_t offset, lframes = frames / 2;
	const snd_pcm_channel_area_t* areas;
	short int*		audiobuf;

	while( 1 )
	{
		avail = snd_pcm_avail_update( playback_handle);
		if (avail < 0)
		{
//			char b[ 80];
//			sprintf( b, "%i", err = avail);
//			std::cerr << "snd_pcm_avail_update error " << b << "=\"" 
//			<< snd_strerror( err) << "\"\n";
			return xrun_recovery();
		}
		lframes = frames / 2;
		if( (int)avail >= (int)lframes ) break;
		if( 0 > ( err = snd_pcm_wait( playback_handle, -1)))
		{
//			std::cerr << "snd_pcm_wait error\n";
			config.xruns++;
			return xrun_recovery();
		}
	}

	if( 0 > ( err = snd_pcm_mmap_begin( playback_handle, &areas, &offset, &lframes)))
	{
		std::cerr << "snd_pcm_mmap_begin error\n";
		xrun_recovery();
		// Return an error code so we can quickly test during initialisation.
		// Won't stop playback at runtime because AudioOutput checks for a return code of -1
		return 0xfeedface;
	}

	audiobuf = (short int*)areas[ 0].addr + 2*offset;

	for( i=0; i<(frames); i++ )
		*(audiobuf++) = (int)(32767*( 1.0 + buffer[i])) - 32767;

	if( 0 > ( err = snd_pcm_mmap_commit(  playback_handle, offset, lframes)))
	{
		std::cerr << "snd_pcm_mmap_commit error\n";
		return xrun_recovery();
	}

	if( periods < 2)
		if( 2 == ++periods )
//				std::cerr << "snd_pcm_start soon\n";
			if( 0 > ( err = snd_pcm_start( playback_handle ) ) )
			{
				std::cerr << "snd_pcm_start error\n";
				return -1;
			}
	/*else{
		if( ++periods == 0)
			periods = 2;
		if( ! ( periods % 0x100))
				std::cerr << "( periods % 0x100)\n";
	}*/
	return 0;
}

int
ALSAmmapAudioDriver::open()
{
	Configuration & config = Configuration::get();

	if (playback_handle != nullptr) return 0;
	
	_channels = config.channels;
	_rate = config.sample_rate;

	if(snd_pcm_open(&playback_handle, config.alsa_audio_device.c_str(), SND_PCM_STREAM_PLAYBACK, 0)<0){
		std::cerr << "ALSA: cannot open audio device " << config.alsa_audio_device << std::endl;
		return -1;
	}
    snd_pcm_hw_params_alloca( &hw_params );
    snd_pcm_hw_params_any( playback_handle, hw_params );
    snd_pcm_hw_params_set_access( playback_handle, hw_params, SND_PCM_ACCESS_MMAP_INTERLEAVED/*SND_PCM_ACCESS_RW_INTERLEAVED*/ );
    snd_pcm_hw_params_set_format( playback_handle, hw_params, SND_PCM_FORMAT_S16_LE );
    snd_pcm_hw_params_set_rate_near( playback_handle, hw_params, &_rate, nullptr );
    snd_pcm_hw_params_set_channels( playback_handle, hw_params, _channels );
	snd_pcm_hw_params_set_periods( playback_handle, hw_params, 16, 0 );
	snd_pcm_hw_params_set_period_size( playback_handle, hw_params, config.buffer_size, 0 );
    snd_pcm_hw_params( playback_handle, hw_params );
	
	config.sample_rate = _rate;
	config.current_audio_driver = "ALSA-MMAP";
#ifdef ENABLE_REALTIME
	config.current_audio_driver_wants_realtime = 1;
#endif
	
	periods = 0;
	return 0;
}

void ALSAmmapAudioDriver::close()
{
	if (playback_handle != nullptr) snd_pcm_close (playback_handle);
	playback_handle = nullptr;
}

ALSAmmapAudioDriver::ALSAmmapAudioDriver()
{
	playback_handle = nullptr;
}

ALSAmmapAudioDriver::~ALSAmmapAudioDriver()
{
  close();
}

#endif


AudioDriver * CreateALSAmmapAudioDriver()
{
#ifdef WITH_ALSA
    return new ALSAmmapAudioDriver();
#else
    return NULL;
#endif
}
