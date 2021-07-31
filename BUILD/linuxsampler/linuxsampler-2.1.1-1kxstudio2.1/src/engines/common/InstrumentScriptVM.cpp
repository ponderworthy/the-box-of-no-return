/*
 * Copyright (c) 2014 - 2017 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "../../common/global_private.h"
#include "InstrumentScriptVM.h"
#include "../AbstractEngineChannel.h"
#include "../../common/global_private.h"
#include "AbstractInstrumentManager.h"
#include "MidiKeyboardManager.h"
#include "Fade.h"

namespace LinuxSampler {

    ///////////////////////////////////////////////////////////////////////
    // class 'EventGroup'

    void EventGroup::insert(int eventID) {
        if (contains(eventID)) return;

        AbstractEngine* pEngine = m_script->pEngineChannel->pEngine;

        // before adding the new event ID, check if there are any dead events
        // and remove them in that case, before otherwise we might run in danger
        // to run out of free space on this group for event IDs if a lot of
        // events die before being removed explicitly from the group by script
        //
        // NOTE: or should we do this "dead ones" check only once in a while?
        int firstDead = -1;
        for (int i = 0; i < size(); ++i) {
            if (firstDead >= 0) {
                if (pEngine->EventByID(eventID)) {
                    remove(firstDead, i - firstDead);
                    firstDead = -1;
                }
            } else {
                if (!pEngine->EventByID(eventID)) firstDead = i;
            }
        }

        append(eventID);
    }

    void EventGroup::erase(int eventID) {
        int index = find(eventID);
        remove(index);
    }

    ///////////////////////////////////////////////////////////////////////
    // class 'InstrumentScript'

    InstrumentScript::InstrumentScript(AbstractEngineChannel* pEngineChannel) {
        parserContext = NULL;
        bHasValidScript = false;
        handlerInit = NULL;
        handlerNote = NULL;
        handlerRelease = NULL;
        handlerController = NULL;
        pEvents = NULL;
        for (int i = 0; i < 128; ++i)
            pKeyEvents[i] = NULL;
        this->pEngineChannel = pEngineChannel;
        for (int i = 0; i < INSTR_SCRIPT_EVENT_GROUPS; ++i)
            eventGroups[i].setScript(this);
    }

    InstrumentScript::~InstrumentScript() {
        resetAll();
        if (pEvents) {
            for (int i = 0; i < 128; ++i) delete pKeyEvents[i];
            delete pEvents;
        }
    }

    /** @brief Load real-time instrument script.
     *
     * Loads the real-time instrument script given by @a text on the engine
     * channel this InstrumentScript object belongs to (defined by
     * pEngineChannel member variable). The sampler engine's resource manager is
     * used to allocate and share equivalent scripts on multiple engine
     * channels.
     *
     * @param text - source code of script
     */
    void InstrumentScript::load(const String& text) {
        dmsg(1,("Loading real-time instrument script ... "));

        // hand back old script reference and VM execution contexts
        // (if not done already)
        unload();

        code = text;

        AbstractInstrumentManager* pManager =
            dynamic_cast<AbstractInstrumentManager*>(pEngineChannel->pEngine->GetInstrumentManager());

        // get new script reference
        parserContext = pManager->scripts.Borrow(text, pEngineChannel);
        if (!parserContext->errors().empty()) {
            std::vector<ParserIssue> errors = parserContext->errors();
            std::cerr << "[ScriptVM] Could not load instrument script, there were "
                    << errors.size() << " parser errors:\n";
            for (int i = 0; i < errors.size(); ++i)
                errors[i].dump();
            return; // stop here if there were any parser errors
        }

        handlerInit = parserContext->eventHandlerByName("init");
        handlerNote = parserContext->eventHandlerByName("note");
        handlerRelease = parserContext->eventHandlerByName("release");
        handlerController = parserContext->eventHandlerByName("controller");
        bHasValidScript =
            handlerInit || handlerNote || handlerRelease || handlerController;

        // amount of script handlers each script event has to execute
        int handlerExecCount = 0;
        if (handlerNote || handlerRelease || handlerController) // only one of these are executed after "init" handler
            handlerExecCount++;

        // create script event pool (if it doesn't exist already)
        if (!pEvents) {
            pEvents = new Pool<ScriptEvent>(CONFIG_MAX_EVENTS_PER_FRAGMENT);
            for (int i = 0; i < 128; ++i)
                pKeyEvents[i] = new RTList<ScriptEvent>(pEvents);
            // reset RTAVLNode's tree node member variables after nodes are allocated
            // (since we can't use a constructor right now, we do that initialization here)
            while (!pEvents->poolIsEmpty()) {
                RTList<ScriptEvent>::Iterator it = pEvents->allocAppend();
                it->reset();
            }
            pEvents->clear();
        }

        // create new VM execution contexts for new script
        while (!pEvents->poolIsEmpty()) {
            RTList<ScriptEvent>::Iterator it = pEvents->allocAppend();
            it->execCtx = pEngineChannel->pEngine->pScriptVM->createExecContext(
                parserContext
            );
            it->handlers = new VMEventHandler*[handlerExecCount+1];
        }
        pEvents->clear();

        dmsg(1,("Done\n"));
    }

    /** @brief Unload real-time instrument script.
     *
     * Unloads the currently used real-time instrument script and frees all
     * resources allocated for that script. The sampler engine's resource manager
     * is used to share equivalent scripts among multiple sampler channels, and
     * to deallocate the parsed script once not used on any engine channel
     * anymore.
     *
     * Calling this method will however not clear the @c code member variable.
     * Thus, the script can be parsed again afterwards.
     */
    void InstrumentScript::unload() {
        //dmsg(1,("InstrumentScript::unload(this=0x%llx)\n", this));

        if (parserContext)
            dmsg(1,("Unloading current instrument script.\n"));

        resetEvents();

        // free allocated VM execution contexts
        if (pEvents) {
            pEvents->clear();
            while (!pEvents->poolIsEmpty()) {
                RTList<ScriptEvent>::Iterator it = pEvents->allocAppend();
                if (it->execCtx) {
                    // free VM execution context object
                    delete it->execCtx;
                    it->execCtx = NULL;
                    // free C array of handler pointers
                    delete [] it->handlers;
                }
            }
            pEvents->clear();
        }
        // hand back VM representation of script
        if (parserContext) {
            AbstractInstrumentManager* pManager =
                dynamic_cast<AbstractInstrumentManager*>(pEngineChannel->pEngine->GetInstrumentManager());

            pManager->scripts.HandBack(parserContext, pEngineChannel);
            parserContext = NULL;
            handlerInit = NULL;
            handlerNote = NULL;
            handlerRelease = NULL;
            handlerController = NULL;
        }
        bHasValidScript = false;
    }

    /**
     * Same as unload(), but this one also empties the @c code member variable
     * to an empty string.
     */
    void InstrumentScript::resetAll() {
        unload();
        code.clear();
    }
    
    /**
     * Clears all currently active script events. This should be called
     * whenever the engine or engine channel was reset for some reason.
     */
    void InstrumentScript::resetEvents() {
        for (int i = 0; i < INSTR_SCRIPT_EVENT_GROUPS; ++i)
            eventGroups[i].clear();

        for (int i = 0; i < 128; ++i)
            if (pKeyEvents[i])
                pKeyEvents[i]->clear();

        suspendedEvents.clear();

        if (pEvents) pEvents->clear();
    }

    ///////////////////////////////////////////////////////////////////////
    // class 'InstrumentScriptVM'

    InstrumentScriptVM::InstrumentScriptVM() :
        m_event(NULL), m_fnPlayNote(this), m_fnSetController(this),
        m_fnIgnoreEvent(this), m_fnIgnoreController(this), m_fnNoteOff(this),
        m_fnSetEventMark(this), m_fnDeleteEventMark(this), m_fnByMarks(this),
        m_fnChangeVol(this), m_fnChangeVolTime(this),
        m_fnChangeTune(this), m_fnChangeTuneTime(this), m_fnChangePan(this),
        m_fnChangePanTime(this), m_fnChangePanCurve(this),
        m_fnChangeCutoff(this), m_fnChangeReso(this),  m_fnChangeAttack(this),
        m_fnChangeDecay(this), m_fnChangeSustain(this), m_fnChangeRelease(this),
        m_fnChangeCutoffAttack(this), m_fnChangeCutoffDecay(this),
        m_fnChangeCutoffSustain(this), m_fnChangeCutoffRelease(this),
        m_fnChangeAmpLFODepth(this), m_fnChangeAmpLFOFreq(this),
        m_fnChangeCutoffLFODepth(this), m_fnChangeCutoffLFOFreq(this),
        m_fnChangePitchLFODepth(this), m_fnChangePitchLFOFreq(this),
        m_fnChangeNote(this), m_fnChangeVelo(this), m_fnFork(this),
        m_fnEventStatus(this), m_fnWait2(this), m_fnStopWait(this),
        m_fnAbort(this), m_fnFadeIn(this), m_fnFadeOut(this),
        m_fnChangeVolCurve(this), m_fnChangeTuneCurve(this),
        m_fnGetEventPar(this), m_fnSetEventPar(this), m_fnChangePlayPos(this),
        m_fnCallbackStatus(this),
        m_varEngineUptime(this), m_varCallbackID(this), m_varAllEvents(this),
        m_varCallbackChildID(this)
    {
        m_CC.size = _MEMBER_SIZEOF(AbstractEngineChannel, ControllerTable);
        m_CC_NUM = DECLARE_VMINT(m_event, class ScriptEvent, cause.Param.CC.Controller);
        m_EVENT_ID = DECLARE_VMINT_READONLY(m_event, class ScriptEvent, id);
        m_EVENT_NOTE = DECLARE_VMINT_READONLY(m_event, class ScriptEvent, cause.Param.Note.Key);
        m_EVENT_VELOCITY = DECLARE_VMINT_READONLY(m_event, class ScriptEvent, cause.Param.Note.Velocity);
        m_KEY_DOWN.size = 128;
        m_KEY_DOWN.readonly = true;
        m_NI_CALLBACK_TYPE = DECLARE_VMINT_READONLY(m_event, class ScriptEvent, handlerType);
        m_NKSP_IGNORE_WAIT = DECLARE_VMINT(m_event, class ScriptEvent, ignoreAllWaitCalls);
        m_NKSP_CALLBACK_PARENT_ID = DECLARE_VMINT_READONLY(m_event, class ScriptEvent, parentHandlerID);
    }

    VMExecStatus_t InstrumentScriptVM::exec(VMParserContext* parserCtx, ScriptEvent* event) {
        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(event->cause.pEngineChannel);

        // prepare built-in script variables for script execution
        m_event = event;
        m_CC.data = (int8_t*) &pEngineChannel->ControllerTable[0];
        m_KEY_DOWN.data = &pEngineChannel->GetMidiKeyboardManager()->KeyDown[0];

        // if script is in start condition, then do mandatory MIDI event
        // preprocessing tasks, which essentially means updating i.e. controller
        // table with new CC value in case of a controller event, because the
        // script might access the new CC value
        if (!event->executionSlices) {
            switch (event->cause.Type) {
                case Event::type_control_change:
                    pEngineChannel->ControllerTable[event->cause.Param.CC.Controller] =
                        event->cause.Param.CC.Value;
                    break;
                case Event::type_channel_pressure:
                    pEngineChannel->ControllerTable[CTRL_TABLE_IDX_AFTERTOUCH] =
                        event->cause.Param.ChannelPressure.Value;
                    break;
                case Event::type_pitchbend:
                    pEngineChannel->ControllerTable[CTRL_TABLE_IDX_PITCHBEND] =
                        event->cause.Param.Pitch.Pitch;
                    break;
                default:
                    ; // noop
            }
        }

        // run the script handler(s)
        VMExecStatus_t res = VM_EXEC_NOT_RUNNING;
        for ( ; event->handlers[event->currentHandler]; event->currentHandler++) {
            res = ScriptVM::exec(
                parserCtx, event->execCtx, event->handlers[event->currentHandler]
            );
            event->executionSlices++;
            if (!(res & VM_EXEC_SUSPENDED)) { // if script terminated ...
                // check if this script handler instance has any forked children
                // to be auto aborted
                for (int iChild = 0; iChild < MAX_FORK_PER_SCRIPT_HANDLER &&
                     event->childHandlerID[iChild]; ++iChild)
                {
                    RTList<ScriptEvent>::Iterator itChild =
                        pEngineChannel->ScriptCallbackByID(event->childHandlerID[iChild]);
                    if (itChild && itChild->autoAbortByParent)
                        itChild->execCtx->signalAbort();
                }
            }
            if (res & VM_EXEC_SUSPENDED || res & VM_EXEC_ERROR) return res;
        }

        return res;
    }

    std::map<String,VMIntRelPtr*> InstrumentScriptVM::builtInIntVariables() {
        // first get built-in integer variables of derived VM class
        std::map<String,VMIntRelPtr*> m = ScriptVM::builtInIntVariables();

        // now add own built-in variables
        m["$CC_NUM"] = &m_CC_NUM;
        m["$EVENT_ID"] = &m_EVENT_ID;
        m["$EVENT_NOTE"] = &m_EVENT_NOTE;
        m["$EVENT_VELOCITY"] = &m_EVENT_VELOCITY;
//         m["$POLY_AT_NUM"] = &m_POLY_AT_NUM;
        m["$NI_CALLBACK_TYPE"] = &m_NI_CALLBACK_TYPE;
        m["$NKSP_IGNORE_WAIT"] = &m_NKSP_IGNORE_WAIT;
        m["$NKSP_CALLBACK_PARENT_ID"] = &m_NKSP_CALLBACK_PARENT_ID;

        return m;
    }

    std::map<String,VMInt8Array*> InstrumentScriptVM::builtInIntArrayVariables() {
        // first get built-in integer array variables of derived VM class
        std::map<String,VMInt8Array*> m = ScriptVM::builtInIntArrayVariables();

        // now add own built-in variables
        m["%CC"] = &m_CC;
        m["%KEY_DOWN"] = &m_KEY_DOWN;
        //m["%POLY_AT"] = &m_POLY_AT;

        return m;
    }

    std::map<String,int> InstrumentScriptVM::builtInConstIntVariables() {
        // first get built-in integer variables of derived VM class
        std::map<String,int> m = ScriptVM::builtInConstIntVariables();

        m["$EVENT_STATUS_INACTIVE"] = EVENT_STATUS_INACTIVE;
        m["$EVENT_STATUS_NOTE_QUEUE"] = EVENT_STATUS_NOTE_QUEUE;
        m["$VCC_MONO_AT"] = CTRL_TABLE_IDX_AFTERTOUCH;
        m["$VCC_PITCH_BEND"] = CTRL_TABLE_IDX_PITCHBEND;
        for (int i = 0; i < INSTR_SCRIPT_EVENT_GROUPS; ++i) {
            m["$MARK_" + ToString(i+1)] = i;
        }
        m["$EVENT_PAR_NOTE"] = EVENT_PAR_NOTE;
        m["$EVENT_PAR_VELOCITY"] = EVENT_PAR_VELOCITY;
        m["$EVENT_PAR_VOLUME"] = EVENT_PAR_VOLUME;
        m["$EVENT_PAR_TUNE"] = EVENT_PAR_TUNE;
        m["$EVENT_PAR_0"] = EVENT_PAR_0;
        m["$EVENT_PAR_1"] = EVENT_PAR_1;
        m["$EVENT_PAR_2"] = EVENT_PAR_2;
        m["$EVENT_PAR_3"] = EVENT_PAR_3;
        m["$NKSP_LINEAR"] = FADE_CURVE_LINEAR;
        m["$NKSP_EASE_IN_EASE_OUT"] = FADE_CURVE_EASE_IN_EASE_OUT;
        m["$CALLBACK_STATUS_TERMINATED"] = CALLBACK_STATUS_TERMINATED;
        m["$CALLBACK_STATUS_QUEUE"]      = CALLBACK_STATUS_QUEUE;
        m["$CALLBACK_STATUS_RUNNING"]    = CALLBACK_STATUS_RUNNING;

        return m;
    }

    std::map<String,VMDynVar*> InstrumentScriptVM::builtInDynamicVariables() {
        // first get built-in dynamic variables of derived VM class
        std::map<String,VMDynVar*> m = ScriptVM::builtInDynamicVariables();

        m["%ALL_EVENTS"] = &m_varAllEvents;
        m["$ENGINE_UPTIME"] = &m_varEngineUptime;
        m["$NI_CALLBACK_ID"] = &m_varCallbackID;
        m["%NKSP_CALLBACK_CHILD_ID"] = &m_varCallbackChildID;

        return m;
    }

    VMFunction* InstrumentScriptVM::functionByName(const String& name) {
        // built-in script functions of this class
        if      (name == "play_note") return &m_fnPlayNote;
        else if (name == "set_controller") return &m_fnSetController;
        else if (name == "ignore_event") return &m_fnIgnoreEvent;
        else if (name == "ignore_controller") return &m_fnIgnoreController;
        else if (name == "note_off") return &m_fnNoteOff;
        else if (name == "set_event_mark") return &m_fnSetEventMark;
        else if (name == "delete_event_mark") return &m_fnDeleteEventMark;
        else if (name == "by_marks") return &m_fnByMarks;
        else if (name == "change_vol") return &m_fnChangeVol;
        else if (name == "change_vol_time") return &m_fnChangeVolTime;
        else if (name == "change_tune") return &m_fnChangeTune;
        else if (name == "change_tune_time") return &m_fnChangeTuneTime;
        else if (name == "change_note") return &m_fnChangeNote;
        else if (name == "change_velo") return &m_fnChangeVelo;
        else if (name == "change_pan") return &m_fnChangePan;
        else if (name == "change_pan_time") return &m_fnChangePanTime;
        else if (name == "change_pan_curve") return &m_fnChangePanCurve;
        else if (name == "change_cutoff") return &m_fnChangeCutoff;
        else if (name == "change_reso") return &m_fnChangeReso;
        else if (name == "change_attack") return &m_fnChangeAttack;
        else if (name == "change_decay") return &m_fnChangeDecay;
        else if (name == "change_sustain") return &m_fnChangeSustain;
        else if (name == "change_release") return &m_fnChangeRelease;
        else if (name == "change_cutoff_attack") return &m_fnChangeCutoffAttack;
        else if (name == "change_cutoff_decay") return &m_fnChangeCutoffDecay;
        else if (name == "change_cutoff_sustain") return &m_fnChangeCutoffSustain;
        else if (name == "change_cutoff_release") return &m_fnChangeCutoffRelease;
        else if (name == "change_amp_lfo_depth") return &m_fnChangeAmpLFODepth;
        else if (name == "change_amp_lfo_freq") return &m_fnChangeAmpLFOFreq;
        else if (name == "change_cutoff_lfo_depth") return &m_fnChangeCutoffLFODepth;
        else if (name == "change_cutoff_lfo_freq") return &m_fnChangeCutoffLFOFreq;
        else if (name == "change_pitch_lfo_depth") return &m_fnChangePitchLFODepth;
        else if (name == "change_pitch_lfo_freq") return &m_fnChangePitchLFOFreq;
        else if (name == "fade_in") return &m_fnFadeIn;
        else if (name == "fade_out") return &m_fnFadeOut;
        else if (name == "change_vol_curve") return &m_fnChangeVolCurve;
        else if (name == "change_tune_curve") return &m_fnChangeTuneCurve;
        else if (name == "change_play_pos") return &m_fnChangePlayPos;
        else if (name == "get_event_par") return &m_fnGetEventPar;
        else if (name == "set_event_par") return &m_fnSetEventPar;
        else if (name == "event_status") return &m_fnEventStatus;
        else if (name == "wait") return &m_fnWait2; // override wait() core implementation
        else if (name == "stop_wait") return &m_fnStopWait;
        else if (name == "abort") return &m_fnAbort;
        else if (name == "fork") return &m_fnFork;
        else if (name == "callback_status") return &m_fnCallbackStatus;

        // built-in script functions of derived VM class
        return ScriptVM::functionByName(name);
    }

} // namespace LinuxSampler
