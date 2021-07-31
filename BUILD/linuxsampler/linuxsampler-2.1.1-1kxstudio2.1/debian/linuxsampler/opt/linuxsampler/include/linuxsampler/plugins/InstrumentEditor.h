/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2007 - 2015 Christian Schoenebeck                       *
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

#ifndef LS_INSTRUMENT_EDITOR_H
#define LS_INSTRUMENT_EDITOR_H

#include "../common/global.h"
#include "../common/Thread.h"
#include "../drivers/midi/VirtualMidiDevice.h"

#include <set>

namespace LinuxSampler {

    // just symbol prototyping
    class InstrumentEditorListener;
    class EngineChannel;

    /** @brief Instrument Editor Interface (external plugin)
     *
     * LinuxSampler allows to spawn arbitrary instrument editor applications
     * within the sampler's own process. That way instrument editors are
     * able to modify already loaded instruments live or on-the-fly, that is
     * without having to load it again neither on sampler nor on editor side,
     * which is essential for editing large instruments.
     *
     * Instrument editors have to implement this abstract base class, embedded
     * into a DLL and placed into the respective sampler's library path. The
     * sampler will load these external DLLs as plugins on startup. Whenever
     * there's a request for editing an instrument, the sampler will try to
     * launch a matching registered editor, by calling the respective
     * plugin's @c Main() method.
     */
    class InstrumentEditor : public VirtualMidiDevice, protected Thread {
    public:

        /////////////////////////////////////////////////////////////////
        // abstract methods
        //     (these have to be implemented by the descendant)

        /**
         * Entry point for the instrument editor's thread. As the
         * instrument data structure is passed as typeless (void*)
         * pointer, this is not type safe! The implementing instrument
         * editor has to interpret the @a sTypeName and @a sTypeVersion
         * arguments to determine if it's able to cast the instrument
         * pointer to a known type and actually be able to work with it.
         *
         * @param pInstrument - pointer to the respective instrument object
         * @param sTypeName - format of the instrument data structure
         *                    (i.e. @c "libgig" )
         * @param sTypeVersion - version of the instrument data structure
         *                       (i.e. @c "3.0.1" ).
         * @param pUserData - (optional) arbitrary 3rd party data that might
         *                    been passed by Launch()
         */
        virtual int Main(void* pInstrument, String sTypeName, String sTypeVersion, void* pUserData = NULL) = 0;

        /**
         * The instrument editor has to return @c true in case it supports
         * the given instrument data structure type and version, it has to
         * return @c false otherwise. This method will be called by the
         * sampler to determine which editor is capable to work with a
         * certain instrument.
         *
         * @param sTypeName - i.e. @c "libgig"
         * @param sTypeVersion - i.e. @c "3.0.1"
         */
        virtual bool IsTypeSupported(String sTypeName, String sTypeVersion) = 0;

        /**
         * The instrument editor's name (i.e. @c "gigedit" ).
         */
        virtual String Name() = 0;

        /**
         * The instrument editor's version (i.e. @c "0.0.1" ).
         */
        virtual String Version() = 0;

        /**
         * Arbitrary textual description of the instrument editor
         * (i.e. "Gigasampler and DLS format editor, GTK based").
         */
        virtual String Description() = 0;



        /////////////////////////////////////////////////////////////////
        // normal methods
        //     (usually not to be overriden by descendant)

        /** @brief Dispatch pending sample removal event.
         *
         * @e SHOULD be called by the instrument editor @e before deleting
         * samples, so the sampler can react by stopping usage of these
         * samples to avoid a crash.
         *
         * After calling this method, the instrument editor @e MUST call
         * @c NotifySamplesRemoved() after it actually deleted the samples, so
         * the sampler can react by i.e. resuming playback of sampler engines.
         *
         * @param Samples - list of samples that will be deleted by the
         *                  instrument editor
         */
        void NotifySamplesToBeRemoved(std::set<void*> Samples);

        /** @brief Dispatch completed sample removal event.
         *
         * Inform the sampler that the by @c NotifySamplesToBeRemoved()
         * previously announced samples have been deleted.
         */
        void NotifySamplesRemoved();

        /** @brief Dispatch pending data structure modification event.
         *
         * @e SHOULD be called by the instrument editor @e before modifying
         * data structures (except samples, which have their own dispatch
         * methods) which could otherwise lead to undesired synchronisation
         * issues and a crash. The respective data structure is passed as a
         * typeless pointer @a pStruct , so the instrument editor additionally
         * has to pass the name of the data structure (i.e. @c "gig::Region" ),
         * so the sampler can cast the pointer to an appropriate type. The
         * sampler will react by stopping usage of the respective data
         * structure.
         *
         * After calling this method, the instrument editor @e MUST call
         * @c NotifyDataStructureChanged() , so the sampler can react by
         * resuming usage of the respective data structure for playback.
         *
         * @param pStruct     - data structure going to be modified
         * @param sStructType - name of the data structure (i.e. its C++
         *                      struct or class name)
         */
        void NotifyDataStructureToBeChanged(void* pStruct, String sStructType);

        /** @brief Dispatch completed data structure modification event.
         *
         * Inform the sampler that the by @c NotifyDataStructureToBeChanged()
         * previously announced data structure has been completely modified.
         *
         * @param pStruct     - data structure that has been modified
         * @param sStructType - name of the data structure (i.e. its C++
         *                      struct or class name)
         */
        void NotifyDataStructureChanged(void* pStruct, String sStructType);

        /** @brief Dispatch sample reference changed event.
         *
         * @e SHOULD be called by the instrument editor @e after a certain data
         * structure changed its reference / pointer to a sample, so the
         * sampler can react by:
         *
         * - Caching the newly referenced sample if necessary.
         * - Un-caching the old referenced sample if necessary.
         *
         * @e Note: the instrument editor additionally @e MUST embed this call
         * into the respective @c NotifyDataStructureToBeChanged() and
         * @c NotifyDataStructureChanged() calls for announcing the data
         * structure whose sample reference is actually to be changed, so the
         * sampler can react by suspending usage. For example:
         * @code
         *  NotifyDataStructureToBeChanged(pDimReg, "gig::DimensionRegion");
         *  gig::Sample* pOldSample = pDimReg->pSample;
         *  pDimReg->pSample = pNewSample;
         *  NotifySampleReferenceChanged(pOldSample, pNewSample);
         *  NotifyDataStructureChanged(pDimReg, "gig::DimensionRegion");
         * @endcode
         * So calling this method alone is not safe!
         *
         * @param pOldSample - previous sample reference
         * @param pNewSample - current sample reference
         */
        void NotifySampleReferenceChanged(void* pOldSample, void* pNewSample);

        /**
         * Launch the instrument editor for the given instrument. The
         * editor will be spawned in its own thread and this method will
         * return as soon as the editor's thread actually started.
         *
         * @param pEngineChannel - the engine channel on which @a pInstrument is
         *                         currently used on and for which the instrument
         *                         editor shall be spawned for editing
         * @param pInstrument - pointer to the respective instrument object
         * @param sTypeName - format of the instrument data structure
         *                    (i.e. @c "libgig" )
         * @param sTypeVersion - version of the instrument data structure
         *                       (i.e. @c "3.0.1" ).
         * @param pUserData - (optional) arbitrary 3rd party data that might
         *                    e.g. been passed by
         *                    InstrumentManager::LaunchInstrumentEditor()
         */
        void Launch(EngineChannel* pEngineChannel, void* pInstrument, String sTypeName, String sTypeVersion, void* pUserData = NULL);

        /**
         * Returns the EngineChannel for which this instrument editor was
         * spawned for, for editing the respective instrument loaded on that
         * EngineChannel.
         */
        EngineChannel* GetEngineChannel();

        /**
         * Registers object that wants to be notified on events.
         */
        void AddListener(InstrumentEditorListener* pListener);

        /**
         * Unregisters object that doesn't want to be notified anymore.
         */
        void RemoveListener(InstrumentEditorListener* pListener);

        /**
         * Constructor
         */
        InstrumentEditor();

        /**
         * Destructor
         */
        virtual ~InstrumentEditor();

    protected:
        std::set<InstrumentEditorListener*> listeners;

        // derived abstract method from base class 'Thread'
        virtual int Main();
    private:
        void*    pInstrument;
        String   sTypeName;
        String   sTypeVersion;
        void*    pUserData;
        EngineChannel* pEngineChannel;
    };

    /** @brief Instrument Editor Notifications
     *
     * This abstract interface class has to be implemented by classes that
     * want to be notified on certain events of an instrument editor. This is
     * typically used on sampler side, but might also be used by an instrument
     * editor to get informed about modifications another instrument editor
     * makes.
     */
    class InstrumentEditorListener {
    public:
        /** @brief Called after the instrument editor stopped running.
         *
         * Automatically called after the instrument editor application stopped
         * running. This method has to be implemented by the descendant.
         *
         * @param pSender - instrument editor that died
         */
        virtual void OnInstrumentEditorQuit(InstrumentEditor* pSender) = 0;

        /** @brief Called before samples are to be deleted.
         *
         * See the dispatcher method
         * @c InstrumentEditor::NotifySamplesToBeRemoved() for details.
         * This method has to be implemented by the descendant.
         *
         * @param Samples - list of samples that will be deleted by the
         *                  instrument editor
         * @param pSender - instrument editor that is going to do this
         *                  modification
         */
        virtual void OnSamplesToBeRemoved(std::set<void*> Samples, InstrumentEditor* pSender) = 0;

        /** @brief  Called after samples have been deleted.
         *
         * See the dispatcher method
         * @c InstrumentEditor::NotifySamplesRemoved() for details.
         * This method has to be implemented by the descendant.
         *
         * @param pSender - instrument editor that did this modification
         */
        virtual void OnSamplesRemoved(InstrumentEditor* pSender) = 0;

        /** @brief Called before data structure is to be modified.
         *
         * See the dispatcher method
         * @c InstrumentEditor::NotifyDataStructureToBeChanged() for details.
         * This method has to be implemented by the descendant.
         *
         * @param pStruct     - data structure going to be modified
         * @param sStructType - name of the data structure (i.e. its C++
         *                      struct or class name)
         * @param pSender     - instrument editor that is going to do this
         *                      modification
         */
        virtual void OnDataStructureToBeChanged(void* pStruct, String sStructType, InstrumentEditor* pSender) = 0;

        /** @brief Called after data structure has been modified.
         *
         * See the dispatcher method
         * @c InstrumentEditor::NotifyDataStructureChanged() for details.
         * This method has to be implemented by the descendant.
         *
         * @param pStruct     - data structure that has been modified
         * @param sStructType - name of the data structure (i.e. its C++
         *                      struct or class name)
         * @param pSender     - instrument editor that did this modification
         */
        virtual void OnDataStructureChanged(void* pStruct, String sStructType, InstrumentEditor* pSender) = 0;

        /** @brief Called after some data structure changed its reference to a sample.
         *
         * @c InstrumentEditor::NotifySampleReferenceChanged() for details.
         * This method has to be implemented by the descendant.
         *
         * @param pOldSample - previous sample reference
         * @param pNewSample - current sample reference
         * @param pSender    - instrument editor that did this modification
         */
        virtual void OnSampleReferenceChanged(void* pOldSample, void* pNewSample, InstrumentEditor* pSender) = 0;
    };

} // namespace LinuxSampler

#endif // LS_INSTRUMENT_EDITOR_H
