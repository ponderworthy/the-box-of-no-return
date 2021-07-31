/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2007 Christian Schoenebeck                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#include "AudioOutputDeviceAlsa.h"
#include "AudioOutputDeviceFactory.h"

namespace LinuxSampler {

// *************** ParameterCard ***************
// *

    AudioOutputDeviceAlsa::ParameterCard::ParameterCard() : DeviceCreationParameterString() {
        InitWithDefault(); // use default card
    }

    AudioOutputDeviceAlsa::ParameterCard::ParameterCard(String s) throw (Exception) : DeviceCreationParameterString(s) {
    }

    String AudioOutputDeviceAlsa::ParameterCard::Description() {
        return "Sound card to be used";
    }

    bool AudioOutputDeviceAlsa::ParameterCard::Fix() {
        return true;
    }

    bool AudioOutputDeviceAlsa::ParameterCard::Mandatory() {
        return false;
    }

    std::map<String,DeviceCreationParameter*> AudioOutputDeviceAlsa::ParameterCard::DependsAsParameters() {
        return std::map<String,DeviceCreationParameter*>(); // no dependencies
    }

    optional<String> AudioOutputDeviceAlsa::ParameterCard::DefaultAsString(std::map<String,String> Parameters) {
        std::vector<String> cards = PossibilitiesAsString(Parameters);
        if (cards.empty()) throw Exception("AudioOutputDeviceAlsa: Can't find any card");
        return cards[0]; // first card by default
    }

    std::vector<String> AudioOutputDeviceAlsa::ParameterCard::PossibilitiesAsString(std::map<String,String> Parameters) {
        int err;
        std::vector<String> CardNames;

        // iterate through all cards
        int card_index = -1;
        while (snd_card_next(&card_index) >= 0 && card_index >= 0) {
            String hw_name = "hw:" + ToString(card_index);
            snd_ctl_t* hCardCtrl;
            if ((err = snd_ctl_open(&hCardCtrl, hw_name.c_str(), 0)) < 0) {
                std::cerr << "AudioOutputDeviceAlsa: Cannot open sound control for card " << card_index << " - " << snd_strerror(err) << std::endl;
                continue;
            }

            // iterate through all devices of that card
            int device_index = -1;
            while (!snd_ctl_pcm_next_device(hCardCtrl, &device_index) && device_index >= 0) {
                String name = ToString(card_index) + "," + ToString(device_index);
                //dmsg(1,("[possibility:%s]", name.c_str()));
                CardNames.push_back(name);
            }

            snd_ctl_close(hCardCtrl);
        }

        return CardNames;
    }

    void AudioOutputDeviceAlsa::ParameterCard::OnSetValue(String s) throw (Exception) {
        // not posssible, as parameter is fix
    }

    String AudioOutputDeviceAlsa::ParameterCard::Name() {
        return "CARD";
    }



// *************** ParameterSampleRate ***************
// *

    AudioOutputDeviceAlsa::ParameterSampleRate::ParameterSampleRate() : AudioOutputDevice::ParameterSampleRate::ParameterSampleRate() {
    }

    AudioOutputDeviceAlsa::ParameterSampleRate::ParameterSampleRate(String s) : AudioOutputDevice::ParameterSampleRate::ParameterSampleRate(s) {
    }

    std::map<String,DeviceCreationParameter*> AudioOutputDeviceAlsa::ParameterSampleRate::DependsAsParameters() {
        static ParameterCard card;
        std::map<String,DeviceCreationParameter*> dependencies;
        dependencies[card.Name()] = &card;
        return dependencies;
    }

    optional<int> AudioOutputDeviceAlsa::ParameterSampleRate::DefaultAsInt(std::map<String,String> Parameters) {
        if (!Parameters.count("CARD")) return optional<int>::nothing;

        // obtain information from given sound card
        ParameterCard card(Parameters["CARD"]);
        String pcm_name = "hw:" + card.ValueAsString();
        snd_pcm_t* pcm_handle = NULL;
        if (snd_pcm_open(&pcm_handle, pcm_name.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0) return optional<int>::nothing;
        snd_pcm_hw_params_t* hwparams;
        snd_pcm_hw_params_alloca(&hwparams);
        if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        uint rate = 44100;
        if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &rate, NULL) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        snd_pcm_close(pcm_handle);
        return rate;
    }

    optional<int> AudioOutputDeviceAlsa::ParameterSampleRate::RangeMinAsInt(std::map<String,String> Parameters) {
        if (!Parameters.count("CARD")) return optional<int>::nothing;

        // obtain information from given sound card
        ParameterCard card(Parameters["CARD"]);
        String pcm_name = "hw:" + card.ValueAsString();
        snd_pcm_t* pcm_handle = NULL;
        if (snd_pcm_open(&pcm_handle, pcm_name.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0) return optional<int>::nothing;
        snd_pcm_hw_params_t* hwparams;
        snd_pcm_hw_params_alloca(&hwparams);
        if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        uint rate;
        if (snd_pcm_hw_params_get_rate_min(hwparams, &rate, NULL) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        snd_pcm_close(pcm_handle);
        return rate;
    }

    optional<int> AudioOutputDeviceAlsa::ParameterSampleRate::RangeMaxAsInt(std::map<String,String> Parameters) {
        if (!Parameters.count("CARD")) return optional<int>::nothing;

        // obtain information from given sound card
        String pcm_name       = "hw:" + Parameters["CARD"];
        snd_pcm_t* pcm_handle = NULL;
        if (snd_pcm_open(&pcm_handle, pcm_name.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0) return optional<int>::nothing;
        snd_pcm_hw_params_t* hwparams;
        snd_pcm_hw_params_alloca(&hwparams);
        if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        uint rate;
        if (snd_pcm_hw_params_get_rate_max(hwparams, &rate, NULL) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        snd_pcm_close(pcm_handle);
        return rate;
    }



// *************** ParameterChannels ***************
// *

    AudioOutputDeviceAlsa::ParameterChannels::ParameterChannels() : AudioOutputDevice::ParameterChannels::ParameterChannels() {
        //InitWithDefault();
        // could not determine default value? ...
        //if (ValueAsInt() == 0) SetValue(2); // ... then (try) a common value
    }

    AudioOutputDeviceAlsa::ParameterChannels::ParameterChannels(String s) : AudioOutputDevice::ParameterChannels::ParameterChannels(s) {
    }

    std::map<String,DeviceCreationParameter*> AudioOutputDeviceAlsa::ParameterChannels::DependsAsParameters() {
        static ParameterCard card;
        std::map<String,DeviceCreationParameter*> dependencies;
        dependencies[card.Name()] = &card;
        return dependencies;
    }

    optional<int> AudioOutputDeviceAlsa::ParameterChannels::DefaultAsInt(std::map<String,String> Parameters) {
        if (!Parameters.count("CARD")) return optional<int>::nothing;

        // obtain information from given sound card
        ParameterCard card(Parameters["CARD"]);
        String pcm_name = "hw:" + card.ValueAsString();
        snd_pcm_t* pcm_handle = NULL;
        if (snd_pcm_open(&pcm_handle, pcm_name.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0) return optional<int>::nothing;
        snd_pcm_hw_params_t* hwparams;
        snd_pcm_hw_params_alloca(&hwparams);
        if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        uint channels = 2;
        if (snd_pcm_hw_params_set_channels_near(pcm_handle, hwparams, &channels) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        snd_pcm_close(pcm_handle);
        return channels;
    }

    optional<int> AudioOutputDeviceAlsa::ParameterChannels::RangeMinAsInt(std::map<String,String> Parameters) {
        uint channels = 1;
        if (!Parameters.count("CARD")) return channels;

        // obtain information from given sound card
        ParameterCard card(Parameters["CARD"]);
        String pcm_name = "hw:" + card.ValueAsString();
        snd_pcm_t* pcm_handle = NULL;
        if (snd_pcm_open(&pcm_handle, pcm_name.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0) return channels;
        snd_pcm_hw_params_t* hwparams;
        snd_pcm_hw_params_alloca(&hwparams);
        if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
            snd_pcm_close(pcm_handle);
            return channels;
        }

        if (snd_pcm_hw_params_get_channels_min(hwparams, &channels) < 0) {
            snd_pcm_close(pcm_handle);
            return channels;
        }
        snd_pcm_close(pcm_handle);
        return channels;
    }

    optional<int> AudioOutputDeviceAlsa::ParameterChannels::RangeMaxAsInt(std::map<String,String> Parameters) {
        uint channels = 100;
        if (!Parameters.count("CARD")) return optional<int>::nothing;

        // obtain information from given sound card
        String pcm_name       = "hw:" + Parameters["CARD"];
        snd_pcm_t* pcm_handle = NULL;
        if (snd_pcm_open(&pcm_handle, pcm_name.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0)
            return optional<int>::nothing;
        snd_pcm_hw_params_t* hwparams;
        snd_pcm_hw_params_alloca(&hwparams);
        if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }

        if (snd_pcm_hw_params_get_channels_max(hwparams, &channels) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        snd_pcm_close(pcm_handle);
        return channels;
    }



// *************** ParameterFragments ***************
// *

    AudioOutputDeviceAlsa::ParameterFragments::ParameterFragments() : DeviceCreationParameterInt() {
        InitWithDefault();
    }

    AudioOutputDeviceAlsa::ParameterFragments::ParameterFragments(String s) throw (Exception) : DeviceCreationParameterInt(s) {
    }

    String AudioOutputDeviceAlsa::ParameterFragments::Description() {
        return "Number of buffer fragments";
    }

    bool AudioOutputDeviceAlsa::ParameterFragments::Fix() {
        return true;
    }

    bool AudioOutputDeviceAlsa::ParameterFragments::Mandatory() {
        return false;
    }

    std::map<String,DeviceCreationParameter*> AudioOutputDeviceAlsa::ParameterFragments::DependsAsParameters() {
        static ParameterCard card;
        std::map<String,DeviceCreationParameter*> dependencies;
        dependencies[card.Name()] = &card;
        return dependencies;
    }

    optional<int> AudioOutputDeviceAlsa::ParameterFragments::DefaultAsInt(std::map<String,String> Parameters) {
        if (!Parameters.count("CARD")) return optional<int>::nothing;

        // obtain information from given sound card
        ParameterCard card(Parameters["CARD"]);
        String pcm_name = "hw:" + card.ValueAsString();
        snd_pcm_t* pcm_handle = NULL;
        if (snd_pcm_open(&pcm_handle, pcm_name.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0) return optional<int>::nothing;
        snd_pcm_hw_params_t* hwparams;
        snd_pcm_hw_params_alloca(&hwparams);
        if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        uint segs = 2;
        if (snd_pcm_hw_params_set_periods_near(pcm_handle, hwparams, &segs, NULL) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        snd_pcm_close(pcm_handle);
        return segs;
    }

    optional<int> AudioOutputDeviceAlsa::ParameterFragments::RangeMinAsInt(std::map<String,String> Parameters) {
        if (!Parameters.count("CARD")) return optional<int>::nothing;

        // obtain information from given sound card
        ParameterCard card(Parameters["CARD"]);
        String pcm_name = "hw:" + card.ValueAsString();
        snd_pcm_t* pcm_handle = NULL;
        if (snd_pcm_open(&pcm_handle, pcm_name.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0) return optional<int>::nothing;
        snd_pcm_hw_params_t* hwparams;
        snd_pcm_hw_params_alloca(&hwparams);
        if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        int dir = 0;
        uint periods_min;
        if (snd_pcm_hw_params_get_periods_min(hwparams, &periods_min, &dir) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        snd_pcm_close(pcm_handle);
        return (int) periods_min;
    }

    optional<int> AudioOutputDeviceAlsa::ParameterFragments::RangeMaxAsInt(std::map<String,String> Parameters) {
        if (!Parameters.count("CARD")) return optional<int>::nothing;

        // obtain information from given sound card
        ParameterCard card(Parameters["CARD"]);
        String pcm_name = "hw:" + card.ValueAsString();
        snd_pcm_t* pcm_handle = NULL;
        if (snd_pcm_open(&pcm_handle, pcm_name.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0) return optional<int>::nothing;
        snd_pcm_hw_params_t* hwparams;
        snd_pcm_hw_params_alloca(&hwparams);
        if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        int dir = 0;
        uint periods_max;
        if (snd_pcm_hw_params_get_periods_max(hwparams, &periods_max, &dir) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        snd_pcm_close(pcm_handle);
        return (int) periods_max;
    }

    std::vector<int> AudioOutputDeviceAlsa::ParameterFragments::PossibilitiesAsInt(std::map<String,String> Parameters) {
        return std::vector<int>();
    }

    void AudioOutputDeviceAlsa::ParameterFragments::OnSetValue(int i) throw (Exception) {
        // not posssible, as parameter is fix
    }

    String AudioOutputDeviceAlsa::ParameterFragments::Name() {
        return "FRAGMENTS";
    }



// *************** ParameterFragmentSize ***************
// *

    AudioOutputDeviceAlsa::ParameterFragmentSize::ParameterFragmentSize() : DeviceCreationParameterInt() {
        InitWithDefault();
    }

    AudioOutputDeviceAlsa::ParameterFragmentSize::ParameterFragmentSize(String s) throw (Exception) : DeviceCreationParameterInt(s) {
    }

    String AudioOutputDeviceAlsa::ParameterFragmentSize::Description() {
        return "Size of each buffer fragment";
    }

    bool AudioOutputDeviceAlsa::ParameterFragmentSize::Fix() {
        return true;
    }

    bool AudioOutputDeviceAlsa::ParameterFragmentSize::Mandatory() {
        return false;
    }

    std::map<String,DeviceCreationParameter*> AudioOutputDeviceAlsa::ParameterFragmentSize::DependsAsParameters() {
        static ParameterCard card;
        std::map<String,DeviceCreationParameter*> dependencies;
        dependencies[card.Name()] = &card;
        return dependencies;
    }

    optional<int> AudioOutputDeviceAlsa::ParameterFragmentSize::DefaultAsInt(std::map<String,String> Parameters) {
        if (!Parameters.count("CARD")) return optional<int>::nothing;

        // obtain information from given sound card
        ParameterCard card(Parameters["CARD"]);
        String pcm_name = "hw:" + card.ValueAsString();
        snd_pcm_t* pcm_handle = NULL;
        if (snd_pcm_open(&pcm_handle, pcm_name.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0) return optional<int>::nothing;
        snd_pcm_hw_params_t* hwparams;
        snd_pcm_hw_params_alloca(&hwparams);
        if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        snd_pcm_uframes_t size = 128;
        if (snd_pcm_hw_params_set_period_size_near(pcm_handle, hwparams, &size, NULL) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        snd_pcm_close(pcm_handle);
        return size;
    }

    optional<int> AudioOutputDeviceAlsa::ParameterFragmentSize::RangeMinAsInt(std::map<String,String> Parameters) {
        if (!Parameters.count("CARD")) return optional<int>::nothing;

        // obtain information from given sound card
        ParameterCard card(Parameters["CARD"]);
        String pcm_name = "hw:" + card.ValueAsString();
        snd_pcm_t* pcm_handle = NULL;
        if (snd_pcm_open(&pcm_handle, pcm_name.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0) return optional<int>::nothing;
        snd_pcm_hw_params_t* hwparams;
        snd_pcm_hw_params_alloca(&hwparams);
        if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        int dir = 0;
        unsigned long period_size_min;
        if (snd_pcm_hw_params_get_period_size_min(hwparams, &period_size_min, &dir) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        snd_pcm_close(pcm_handle);
        return (int) period_size_min;
    }

    optional<int> AudioOutputDeviceAlsa::ParameterFragmentSize::RangeMaxAsInt(std::map<String,String> Parameters) {
        if (!Parameters.count("CARD")) return optional<int>::nothing;

        // obtain information from given sound card
        ParameterCard card(Parameters["CARD"]);
        String pcm_name = "hw:" + card.ValueAsString();
        snd_pcm_t* pcm_handle = NULL;
        if (snd_pcm_open(&pcm_handle, pcm_name.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0) return optional<int>::nothing;
        snd_pcm_hw_params_t* hwparams;
        snd_pcm_hw_params_alloca(&hwparams);
        if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        int dir = 0;
        unsigned long period_size_max;
        if (snd_pcm_hw_params_get_period_size_max(hwparams, &period_size_max, &dir) < 0) {
            snd_pcm_close(pcm_handle);
            return optional<int>::nothing;
        }
        snd_pcm_close(pcm_handle);
        return (int) period_size_max; //FIXME: might overflow int limit
    }

    std::vector<int> AudioOutputDeviceAlsa::ParameterFragmentSize::PossibilitiesAsInt(std::map<String,String> Parameters) {
        return std::vector<int>();
    }

    void AudioOutputDeviceAlsa::ParameterFragmentSize::OnSetValue(int i) throw (Exception) {
        // not posssible, as parameter is fix
    }

    String AudioOutputDeviceAlsa::ParameterFragmentSize::Name() {
        return "FRAGMENTSIZE";
    }



// *************** AudioOutputDeviceAlsa ***************
// *

    /**
     * Create and initialize Alsa audio output device with given parameters.
     *
     * @param Parameters - optional parameters
     * @throws AudioOutputException  if output device cannot be opened
     */
    AudioOutputDeviceAlsa::AudioOutputDeviceAlsa(std::map<String,DeviceCreationParameter*> Parameters) : AudioOutputDevice(Parameters), Thread(true, true, 1, 0) {
        pcm_handle           = NULL;
        stream               = SND_PCM_STREAM_PLAYBACK;
        this->uiAlsaChannels = ((DeviceCreationParameterInt*)Parameters["CHANNELS"])->ValueAsInt();
        this->uiSamplerate   = ((DeviceCreationParameterInt*)Parameters["SAMPLERATE"])->ValueAsInt();
        this->FragmentSize   = ((DeviceCreationParameterInt*)Parameters["FRAGMENTSIZE"])->ValueAsInt();
        uint Fragments       = ((DeviceCreationParameterInt*)Parameters["FRAGMENTS"])->ValueAsInt();
        String Card          = ((DeviceCreationParameterString*)Parameters["CARD"])->ValueAsString();

        dmsg(2,("Checking if hw parameters supported...\n"));
        if (HardwareParametersSupported(Card, uiAlsaChannels, uiSamplerate, Fragments, FragmentSize)) {
            pcm_name = "hw:" + Card;
        }
        else {
            fprintf(stderr, "Warning: your soundcard doesn't support chosen hardware parameters; ");
            fprintf(stderr, "trying to compensate support lack with plughw...");
            fflush(stdout);
            pcm_name = "plughw:" + Card;
        }
        dmsg(2,("HW check completed.\n"));

        int err;

        snd_pcm_hw_params_alloca(&hwparams);  // Allocate the snd_pcm_hw_params_t structure on the stack.

        /* Open PCM. The last parameter of this function is the mode. */
        /* If this is set to 0, the standard mode is used. Possible   */
        /* other values are SND_PCM_NONBLOCK and SND_PCM_ASYNC.       */
        /* If SND_PCM_NONBLOCK is used, read / write access to the    */
        /* PCM device will return immediately. If SND_PCM_ASYNC is    */
        /* specified, SIGIO will be emitted whenever a period has     */
        /* been completely processed by the soundcard.                */
        if ((err = snd_pcm_open(&pcm_handle, pcm_name.c_str(), stream, 0)) < 0) {
            throw AudioOutputException(String("Error opening PCM device ") + pcm_name + ": " + snd_strerror(err));
        }

        if ((err = snd_pcm_hw_params_any(pcm_handle, hwparams)) < 0) {
            throw AudioOutputException(String("Error, cannot initialize hardware parameter structure: ") + snd_strerror(err));
        }

        /* Set access type. This can be either    */
        /* SND_PCM_ACCESS_RW_INTERLEAVED or       */
        /* SND_PCM_ACCESS_RW_NONINTERLEAVED.      */
        if ((err = snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
            throw AudioOutputException(String("Error snd_pcm_hw_params_set_access: ") + snd_strerror(err));
        }

        /* Set sample format */
        #if WORDS_BIGENDIAN
        if ((err = snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16_BE)) < 0)
        #else // little endian
        if ((err = snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16_LE)) < 0)
        #endif
        {
            throw AudioOutputException(String("Error setting sample format: ") + snd_strerror(err));
        }

        int dir = 0;

        /* Set sample rate. If the exact rate is not supported */
        /* by the hardware, use nearest possible rate.         */
        #if ALSA_MAJOR > 0
        if((err = snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &uiSamplerate, &dir)) < 0)
        #else
        if((err = snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, uiSamplerate, &dir)) < 0)
        #endif
        {
            throw AudioOutputException(String("Error setting sample rate: ") + snd_strerror(err));
        }

        if ((err = snd_pcm_hw_params_set_channels(pcm_handle, hwparams, uiAlsaChannels)) < 0) {
            throw AudioOutputException(String("Error setting number of channels: ") + snd_strerror(err));
        }

        /* Set number of periods. Periods used to be called fragments. */
        if ((err = snd_pcm_hw_params_set_periods(pcm_handle, hwparams, Fragments, dir)) < 0) {
            throw AudioOutputException(String("Error setting number of ") + ToString(Fragments) + " periods: " + snd_strerror(err));
        }

        /* Set buffer size (in frames). The resulting latency is given by */
        /* latency = periodsize * periods / (rate * bytes_per_frame)     */
        if ((err = snd_pcm_hw_params_set_buffer_size(pcm_handle, hwparams, (FragmentSize * Fragments))) < 0) {
            throw AudioOutputException(String("Error setting buffersize: ") + snd_strerror(err));
        }

        /* Apply HW parameter settings to */
        /* PCM device and prepare device  */
        if ((err = snd_pcm_hw_params(pcm_handle, hwparams)) < 0) {
            throw AudioOutputException(String("Error setting HW params: ") + snd_strerror(err));
        }

        if (snd_pcm_sw_params_malloc(&swparams) != 0) {
            throw AudioOutputException(String("Error in snd_pcm_sw_params_malloc: ") + snd_strerror(err));
        }

        if (snd_pcm_sw_params_current(pcm_handle, swparams) != 0) {
            throw AudioOutputException(String("Error in snd_pcm_sw_params_current: ") + snd_strerror(err));
        }

        if (snd_pcm_sw_params_set_stop_threshold(pcm_handle, swparams, 0xffffffff) != 0) {
            throw AudioOutputException(String("Error in snd_pcm_sw_params_set_stop_threshold: ") + snd_strerror(err));
        }

        if (snd_pcm_sw_params(pcm_handle, swparams) != 0) {
            throw AudioOutputException(String("Error in snd_pcm_sw_params: ") + snd_strerror(err));
        }

        if ((err = snd_pcm_prepare(pcm_handle)) < 0) {
            throw AudioOutputException(String("Error snd_pcm_prepare: ") + snd_strerror(err));
        }

        // allocate Alsa output buffer
        pAlsaOutputBuffer = new int16_t[uiAlsaChannels * FragmentSize];

        // create audio channels for this audio device to which the sampler engines can write to
        for (int i = 0; i < uiAlsaChannels; i++) this->Channels.push_back(new AudioChannel(i, FragmentSize));

	if (((DeviceCreationParameterBool*)Parameters["ACTIVE"])->ValueAsBool()) {
		Play();
	}
    }

    AudioOutputDeviceAlsa::~AudioOutputDeviceAlsa() {
        //dmsg(0,("Stopping Alsa Thread..."));
        //StopThread();  //FIXME: commented out due to a bug in thread.cpp (StopThread() doesn't return at all)
        //dmsg(0,("OK\n"));

        snd_pcm_close(pcm_handle);

        if (pAlsaOutputBuffer) {
            //FIXME: currently commented out due to segfault
            //delete[] pOutputBuffer;
        }
    }

    /**
     *  Checks if sound card supports the chosen parameters.
     *
     *  @returns  true if hardware supports it
     *  @throws AudioOutputException - if device cannot be accessed
     */
    bool AudioOutputDeviceAlsa::HardwareParametersSupported(String card, uint channels, int samplerate, uint numfragments, uint fragmentsize) throw (AudioOutputException) {
        pcm_name = "hw:" + card;
        int err;
        if ((err = snd_pcm_open(&pcm_handle, pcm_name.c_str(), stream, SND_PCM_NONBLOCK)) < 0) {
            throw AudioOutputException(String("Error opening PCM device ") + pcm_name + ": " + snd_strerror(err));
        }
        snd_pcm_hw_params_alloca(&hwparams);
        if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
            snd_pcm_close(pcm_handle);
            return false;
        }
        if (snd_pcm_hw_params_test_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
            snd_pcm_close(pcm_handle);
            return false;
        }
        #if WORDS_BIGENDIAN
        if (snd_pcm_hw_params_test_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16_BE) < 0)
        #else // little endian
        if (snd_pcm_hw_params_test_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16_LE) < 0)
        #endif
        {
            snd_pcm_close(pcm_handle);
            return false;
        }
        int dir = 0;
        if (snd_pcm_hw_params_test_rate(pcm_handle, hwparams, samplerate, dir) < 0) {
            snd_pcm_close(pcm_handle);
            return false;
        }
        if (snd_pcm_hw_params_test_channels(pcm_handle, hwparams, channels) < 0) {
            snd_pcm_close(pcm_handle);
            return false;
        }
        if (snd_pcm_hw_params_test_periods(pcm_handle, hwparams, numfragments, dir) < 0) {
            snd_pcm_close(pcm_handle);
            return false;
        }
        if (snd_pcm_hw_params_test_buffer_size(pcm_handle, hwparams, (fragmentsize * numfragments)) < 0) {
            snd_pcm_close(pcm_handle);
            return false;
        }

        snd_pcm_close(pcm_handle);
        return true;
    }

    void AudioOutputDeviceAlsa::Play() {
        StartThread();
    }

    bool AudioOutputDeviceAlsa::IsPlaying() {
        return IsRunning(); // if Thread is running
    }

    void AudioOutputDeviceAlsa::Stop() {
        StopThread();
    }

    AudioChannel* AudioOutputDeviceAlsa::CreateChannel(uint ChannelNr) {
        // just create a mix channel
        return new AudioChannel(ChannelNr, Channel(ChannelNr % uiAlsaChannels));
    }

    uint AudioOutputDeviceAlsa::MaxSamplesPerCycle() {
        return FragmentSize;
    }

    uint AudioOutputDeviceAlsa::SampleRate() {
        return uiSamplerate;
    }

    String AudioOutputDeviceAlsa::Name() {
        return "ALSA";
    }

    String AudioOutputDeviceAlsa::Driver() {
        return Name();
    }

    String AudioOutputDeviceAlsa::Description() {
        return "Advanced Linux Sound Architecture";
    }

    String AudioOutputDeviceAlsa::Version() {
       String s = "$Revision: 2494 $";
       return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
    }

    /**
     * Entry point for the thread.
     */
    int AudioOutputDeviceAlsa::Main() {
        while (true) {

            // let all connected engines render 'FragmentSize' sample points
            RenderAudio(FragmentSize);

            // convert from DSP value range (-1.0..+1.0) to 16 bit integer value
            // range (-32768..+32767), check clipping  and copy to Alsa output buffer
            // (note: we use interleaved output method to Alsa)
            for (int c = 0; c < uiAlsaChannels; c++) {
                float* in  = Channels[c]->Buffer();
                for (int i = 0, o = c; i < FragmentSize; i++ , o += uiAlsaChannels) {
                    float sample_point = in[i] * 32768.0f;
                    if (sample_point < -32768.0) sample_point = -32768.0;
                    if (sample_point >  32767.0) sample_point =  32767.0;
                    pAlsaOutputBuffer[o] = (int16_t) sample_point;
                }
            }

            // output sound
            int res = Output();
            if (res < 0) {
                fprintf(stderr, "Alsa: Audio output error, exiting.\n");
                exit(EXIT_FAILURE);
            }
        }
        // just to suppress compiler warning
        return EXIT_FAILURE;
    }

    /**
     *  Will be called after every audio fragment cycle, to output the audio data
     *  of the current fragment to the soundcard.
     *
     *  @returns  0 on success, a value < 0 on error
     */
    int AudioOutputDeviceAlsa::Output() {
        int err = snd_pcm_writei(pcm_handle, pAlsaOutputBuffer, FragmentSize);
        if (err < 0) {
            fprintf(stderr, "Error snd_pcm_writei failed: %s\n", snd_strerror(err));
            return -1;
        }
        return 0;
    }

} // namespace LinuxSampler
