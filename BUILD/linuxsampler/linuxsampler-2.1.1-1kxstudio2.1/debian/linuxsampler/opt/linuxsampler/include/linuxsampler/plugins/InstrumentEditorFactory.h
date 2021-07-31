/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2007 - 2014 Christian Schoenebeck                       *
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

#ifndef LS_INSTRUMENT_EDITOR_FACTORY_H
#define LS_INSTRUMENT_EDITOR_FACTORY_H

#include <map>
#include <vector>
#include <list>

#include "../common/Exception.h"
#include "InstrumentEditor.h"

#if defined(WIN32)
# define REGISTER_INSTRUMENT_EDITOR(PluginClass) \
    extern "C" __declspec(dllexport) void* \
    createInstrumentEditorInnerFactory() { \
        return new LinuxSampler::InstrumentEditorFactory::InnerFactoryTemplate<PluginClass>(); \
    }
#else
# define REGISTER_INSTRUMENT_EDITOR(PluginClass) \
    LinuxSampler::InstrumentEditorFactory::InnerFactoryRegistrator<PluginClass> \
    __auto_register_instrument_editor__##PluginClass;
#endif

namespace LinuxSampler {

    class InstrumentEditorFactory {
    public:
        class InnerFactory {
        public:
            virtual ~InnerFactory() {}
            virtual InstrumentEditor* Create() = 0;
            virtual void Destroy(InstrumentEditor* pEditor) = 0;
        };

        template<class PluginClass_T>
        class InnerFactoryTemplate : public InnerFactory {
        public:
            virtual InstrumentEditor* Create() {
                return new PluginClass_T();
            }

            virtual void Destroy(InstrumentEditor* pEditor)  {
                delete pEditor;
            }
        };

        template<class PluginClass_T>
        class InnerFactoryRegistrator {
        public:
            InnerFactoryRegistrator() {
                InnerFactoryTemplate<PluginClass_T>* pInnerFactory =
                    new InnerFactoryTemplate<PluginClass_T>();
                InstrumentEditor* pEditor = pInnerFactory->Create();
                if (InnerFactories.count(pEditor->Name())) {
                    pInnerFactory->Destroy(pEditor);
                    delete pInnerFactory;
                } else {
                    InnerFactories[pEditor->Name()] = pInnerFactory;
                    pInnerFactory->Destroy(pEditor);
                }
            }

           ~InnerFactoryRegistrator() {
                InnerFactoryTemplate<PluginClass_T> innerFactory;
                InstrumentEditor* pEditor = innerFactory.Create();
                if (InnerFactories.count(pEditor->Name())) {
                    InnerFactory* pZombie =
                        InnerFactories[pEditor->Name()];
                    InnerFactories.erase(pEditor->Name());
                    if (pZombie) delete pZombie;
                }
                innerFactory.Destroy(pEditor);
            }
        };

        static InstrumentEditor*   Create(String InstrumentEditorName) throw (Exception);
        static void                Destroy(InstrumentEditor* pInstrumentEditor) throw (Exception);
        static std::vector<String> AvailableEditors();
        static String              AvailableEditorsAsString();
        static std::vector<String> MatchingEditors(String sTypeName, String sTypeVersion);
        static void                LoadPlugins();
        static void                ClosePlugins();

    protected:
        static std::map<String, InnerFactory*> InnerFactories;
        static bool                            bPluginsLoaded;
        static std::list<void*>                LoadedDLLs;

    private:
        static bool LoadPlugins(String plugindir);
    };

} // namespace LinuxSampler

#endif // LS_INSTRUMENT_EDITOR_FACTORY_H
