/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2006 - 2009 Christian Schoenebeck                       *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this library; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#ifndef __LS_MIDIINSTRUMENTMAPPER_H__
#define __LS_MIDIINSTRUMENTMAPPER_H__

#include <map>

#include "../../EventListeners.h"
#include "../../common/global.h"
#include "../../common/optional.h"
#include "../../engines/InstrumentManager.h"
#include "midi.h"

namespace LinuxSampler {

    // just symbol prototyping
    class MidiInputPort;

    /** @brief Mapping MIDI bank/program numbers with real instruments.
     *
     * By default (that is on startup) the sampler will simply ignore all
     * MIDI program change messages. The MidiInstrumentMapper allows to map
     * arbitrary (MIDI bank MSB, MIDI bank LSB, MIDI program) triples with
     * an actual (Sampler Engine, Instrument File, Index) triple, so the
     * sampler knows which instrument to load on the respective MIDI program
     * change messages.
     *
     * The sampler allows to manage arbitrary amount of MIDI instrument
     * maps. For example you might create (at least) two MIDI instrument
     * maps: one for "normal" instruments and one for drumkits.
     */
    class MidiInstrumentMapper {
        public:
            /**
             * Defines the life-time strategy for an instrument.
             */
            enum mode_t {
                ON_DEMAND      = 0,  ///< Instrument will be loaded when needed, freed once not needed anymore.
                ON_DEMAND_HOLD = 1,  ///< Instrument will be loaded when needed and kept even if not needed anymore.
                PERSISTENT     = 2,  ///< Instrument will immediately be loaded and kept all the time.
                #if !defined(WIN32)
                VOID           = 127, ///< @deprecated use DONTCARE instead!
                #endif
                DONTCARE       = 127 ///< Don't care, let it up to the InstrumentManager to decide for an appropriate LoadMode.
            };

            /**
             * Defines the instrument and settings a MIDI bank MSB, LSB,
             * program triple ought to be mapped to.
             */
            struct entry_t {
                String EngineName;      ///< The sampler engine to be used.
                String InstrumentFile;  ///< File name of the instrument to be loaded.
                uint   InstrumentIndex; ///< Index of the instrument within its file.
                mode_t LoadMode;        ///< Life-time strategy of instrument.
                float  Volume;          ///< Global volume factor for this instrument.
                String Name;            ///< Display name that should be associated with this mapping entry.
            };

            /**
             * Registers the specified listener to be notified when the number
             * of MIDI instruments on a particular MIDI instrument map is changed.
             */
            static void AddMidiInstrumentCountListener(MidiInstrumentCountListener* l);

            /**
             * Removes the specified listener.
             */
            static void RemoveMidiInstrumentCountListener(MidiInstrumentCountListener* l);

            /**
             * Registers the specified listener to be notified when
             * a MIDI instrument in a MIDI instrument map is changed.
             */
            static void AddMidiInstrumentInfoListener(MidiInstrumentInfoListener* l);

            /**
             * Removes the specified listener.
             */
            static void RemoveMidiInstrumentInfoListener(MidiInstrumentInfoListener* l);

            /**
             * Registers the specified listener to be notified
             * when the number of MIDI instrument maps is changed.
             */
            static void AddMidiInstrumentMapCountListener(MidiInstrumentMapCountListener* l);

            /**
             * Removes the specified listener.
             */
            static void RemoveMidiInstrumentMapCountListener(MidiInstrumentMapCountListener* l);

            /**
             * Registers the specified listener to be notified when
             * the settings of a MIDI instrument map are changed.
             */
            static void AddMidiInstrumentMapInfoListener(MidiInstrumentMapInfoListener* l);

            /**
             * Removes the specified listener.
             */
            static void RemoveMidiInstrumentMapInfoListener(MidiInstrumentMapInfoListener* l);

            /**
             * Adds a new entry to the given MIDI instrument map in case
             * an entry with \a Index does not exist yet, otherwise it will
             * replace the existing entry. Note that some given settings
             * might simply be ignored or might change the settings of other
             * entries in the map (i.e. because another instrument in the
             * map is part of the same file and the respective sampler
             * engine does not allow to use different LoadModes for
             * instruments of the same file). Note that in case of a
             * PERSISTENT LoadMode argument the given instrument will
             * immediately be loaded, that means by default this method will
             * block until the whole instrument was loaded completely. You
             * can override this behavior by setting \a bInBackground to
             * true, so the instrument will be loaded in a separate thread
             * (in that case you won't catch loading errors though, i.e. if
             * the file does not exist or might be corrupt for example).
             *
             * @param Map   - map index
             * @param Index - unique index of the new entry to add
             * @param Entry - the actual instrument and settings
             * @param bInBackground - avoid this method to block for long time
             * @throws Exception - if the given map or engine type does not
             *                     exist or instrument loading failed
             */
            static void AddOrReplaceEntry(int Map, midi_prog_index_t Index, entry_t Entry, bool bInBackground = false) throw (Exception);

            /**
             * Gets an entry from the MIDI instrument map.
             *
             * @param Map   - map index
             * @param Index - index of entry to retrieve
             */
            static entry_t GetEntry(int Map, uint MidiBank, uint MidiProg);

            /**
             * Remove an existing entry from the MIDI instrument map.
             *
             * @param Map   - map index
             * @param Index - index of entry to delete
             */
            static void RemoveEntry(int Map, midi_prog_index_t Index);

            /**
             * Clear the whole given MIDI instrument map, that is delete all
             * its entries.
             *
             * @param Map - map index
             */
            static void RemoveAllEntries(int Map);

            /**
             * Returns the currently existing MIDI instrument map entries
             * of the given map with their current settings.
             *
             * @param Map - map index
             * @throws Exception - in case \a Map does not exist
             */
            static std::map<midi_prog_index_t,entry_t> Entries(int Map) throw (Exception);

            /**
             * Returns the IDs of all currently existing MIDI instrument
             * maps.
             */
            static std::vector<int> Maps();

            /**
             * Create a new MIDI instrument map. Optionally you can assign
             * a custom name for the map. Map names don't have to be unique.
             *
             * @param MapName - (optional) name for the map
             * @returns ID of the new map
             * @throws Exception - if there's no free map ID left
             */
            static int AddMap(String MapName = "") throw (Exception) ;

            /**
             * Returns the custom name of the given map.
             *
             * @param Map - map index
             * @throws Exception - if given map does not exist
             */
            static String MapName(int Map) throw (Exception);

            /**
             * Rename the given, already existing map. Map names don't have
             * to be unique.
             *
             * @param Map - map index
             * @param NewName - the map's new name to be assigned
             * @throws Exception - if the given map does not exist
             */
            static void RenameMap(int Map, String NewName) throw (Exception);

            /**
             * Delete the given map.
             *
             * @param Map - ID of the map to delete
             */
            static void RemoveMap(int Map);

            /**
             * Completely delete all existing maps.
             */
            static void RemoveAllMaps();

            /**
             * Returns amount of currently available MIDI instrument maps.
             */
            static int GetMapCount();

            /**
             * Returns the amount of currently available MIDI instruments in all maps.
             * @param Map - ID of the map
             */
            static int GetInstrumentCount();

            /**
             * Returns the amount of currently available MIDI instruments in the specified map.
             * @param Map - ID of the map
             */
            static int GetInstrumentCount(int Map);

            /**
             * Gets the ID of the default map.
             * For now, the default map is the first available map. When
             * the default map is removed, the default map becomes the next available map.
             * @return The ID of the default map or -1 if the there are no maps added.
             */
            static int GetDefaultMap();

	    /**
	     * Sets the default map.
	     * @param MapId The ID of the new default map.
	     */
	    static void SetDefaultMap(int MapId);

        protected:
            /**
             * Notifies listeners that the number of MIDI instruments
             * on the specified MIDI instrument map has been changed.
             * @param MapId The numerical ID of the MIDI instrument map.
             * @param NewCount The new number of MIDI instruments.
             */
            static void fireMidiInstrumentCountChanged(int MapId, int NewCount);

            /**
             * Notifies listeners that a MIDI instrument
             * in a MIDI instrument map is changed.
             * @param MapId The numerical ID of the MIDI instrument map.
             * @param Bank The index of the MIDI bank, containing the instrument.
             * @param Program The MIDI program number of the instrument.
             */
            static void fireMidiInstrumentInfoChanged(int MapId, int Bank, int Program);

             /**
             * Notifies listeners that the number of MIDI instrument maps has been changed.
             * @param NewCount The new number of MIDI instrument maps.
             */
            static void fireMidiInstrumentMapCountChanged(int NewCount);

            /**
             * Notifies listeners that the settings of a MIDI instrument map are changed.
             */
            static void fireMidiInstrumentMapInfoChanged(int MapId);

            static optional<entry_t> GetEntry(int Map, midi_prog_index_t Index); // shall only be used by EngineChannel ATM (see source comment)
            friend class EngineChannel; // allow EngineChannel to access GetEntry()

        private:
            /**
             * Sets the load mode of the specified entry.
             * The following field should be set before calling this method:
             * pEntry->EngineName, pEntry->InstrumentFile, pEntry->InstrumentIndex
             */
            static void SetLoadMode(entry_t* pEntry);

            static ListenerList<MidiInstrumentCountListener*> llMidiInstrumentCountListeners;
            static ListenerList<MidiInstrumentInfoListener*> llMidiInstrumentInfoListeners;
            static ListenerList<MidiInstrumentMapCountListener*> llMidiInstrumentMapCountListeners;
            static ListenerList<MidiInstrumentMapInfoListener*> llMidiInstrumentMapInfoListeners;
            
            static int DefaultMap;
    };

} // namespace LinuxSampler

#endif // __LS_MIDIINSTRUMENTMAPPER_H__
