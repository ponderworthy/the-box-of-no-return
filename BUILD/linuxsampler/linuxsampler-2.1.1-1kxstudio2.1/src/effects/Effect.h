/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2008, 2010, 2013 Christian Schoenebeck                  *
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

#ifndef LS_EFFECT_H
#define LS_EFFECT_H

#include <vector>
#include "../drivers/audio/AudioChannel.h"
#include "../common/Exception.h"
#include "EffectControl.h"

namespace LinuxSampler {

// just symbol prototyping
class AudioOutputDevice;
class EffectInfo;

/**
 * Abstract base class for sampler internal effects.
 */
class Effect {
public:
    /////////////////////////////////////////////////////////////////
    // abstract methods
    //     (these have to be implemented by the descendant)

    /**
     * General information about this effect type, that is independent of this
     * instance of this effect type.
     *
     * @returns general effect information
     */
    virtual EffectInfo* GetEffectInfo() = 0;

    /**
     * Use the input audio signal given with @a ppInputChannels, render the
     * effect and mix the result into the effect's output channels.
     *
     * @param Samples - amount of sample points to process
     */
    virtual void RenderAudio(uint Samples) = 0;

    /**
     * Will be called by the sampler before using the effect the first time.
     * This method can be implemented by the effect to adjust itself to the
     * requirements given by the audio output device.
     *
     * This is the perfect place to create the required audio input and
     * output channels! ;-)
     *
     * CAUTION: InitEffect() might be called several times! For example it
     * will be called again if some audio context parameter of the audio
     * output driver in use, has been changed, like particulary sample rate
     * changes and max. samples per cycle (period size) changes. So take
     * care not to create memory leaks due to this circumstance.
     *
     * @param pDevice - audio output device which is going to play the signal
     * @throws Exception - if effect could not be initialized successfully
     */
    virtual void InitEffect(AudioOutputDevice* pDevice) throw (Exception);

    /**
     * Constructor, initializes variables.
     */
    Effect();

    /**
     * Destructor, deletes all audio input and output channels.
     */
    virtual ~Effect();



    /////////////////////////////////////////////////////////////////
    // normal methods
    //     (usually not to be overriden by descendant)

    /**
     * Returns audio input channel with index \a ChannelIndex or NULL if
     * index out of bounds.
     */
    AudioChannel* InputChannel(uint ChannelIndex) const;

    /**
     * Returns the amount of audio input channels the effect is currently
     * providing.
     */
    uint InputChannelCount() const;

    /**
     * Returns audio output channel with index \a ChannelIndex or NULL if
     * index out of bounds.
     */
    AudioChannel* OutputChannel(uint ChannelIndex) const;

    /**
     * Returns the amount of audio output channels the effect is currently
     * providing.
     */
    uint OutputChannelCount() const;

    /**
     * Returns effect parameter with index \a ControlIndex or NULL if index
     * out of bounds.
     */
    EffectControl* InputControl(uint ControlIndex) const;

    /**
     * Returns the amount of effect parameters the effect provides.
     */
    uint InputControlCount() const;

    /**
     * Shall be called to set the object that uses this effect, e.g. to
     * determine whether an effect is currently in use.
     */
    void SetParent(void* pParent);

    /**
     * Returns object which currently uses this effect.
     */
    void* Parent() const;

    /**
     * Sets the unique numerical ID of this effect within the sampler instance.
     * This method is usually only called by the EffectFactory class.
     */
    void SetId(int id);

    /**
     * Returns unique numerical ID of this effect within the sampler instance,
     * as previously set by SetId() .
     */
    int ID() const;

protected:
    std::vector<AudioChannel*> vInputChannels;
    std::vector<AudioChannel*> vOutputChannels;
    std::vector<EffectControl*> vInputControls;
    std::vector<EffectControl*> vOutputControls; ///< yet unused
    void* pParent;
    int iID;
};

} // namespace LinuxSampler

#endif // LS_EFFECT_H
