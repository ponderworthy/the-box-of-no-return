/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2011 - 2012 Grigor Iliev                                *
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

#ifndef __LS_SIGNALUNIT_H__
#define	__LS_SIGNALUNIT_H__

#include "../../common/ArrayList.h"
#include "../../common/Pool.h"


namespace LinuxSampler {

    template<typename T>
    class FixedArray {
        public:
            FixedArray(int capacity) {
                iSize = 0;
                iCapacity = capacity;
                pData = new T[iCapacity];
            }
            
            ~FixedArray() {
                delete[] pData;
                pData = NULL;
            }
            
            inline int size() const { return iSize; }
            inline int capacity() { return iCapacity; }
            
            void add(T element) {
                if (iSize >= iCapacity) throw Exception("Array out of bounds");
                pData[iSize++] = element;
            }
            
            
            T& increment() {
                if (iSize >= iCapacity) throw Exception("Array out of bounds");
                return pData[iSize++];
            }
            
            void clear() { iSize = 0; }
            
            void copy(const FixedArray<T>& array) {
                if(array.size() >= capacity()) throw Exception("Not enough space to copy array");
                for (int i = 0; i < array.size(); i++) pData[i] = array[i];
                iSize = array.size();
            }
            
            inline T& operator[](int idx) const {
                return pData[idx];
            }
            
        private:
            T*   pData;
            int  iSize;
            int  iCapacity;
    };
    
    class SignalUnitRack;

    /**
     * A signal unit consist of internal signal generator (like envelope generator,
     * low frequency oscillator, etc) with a number of generator parameters which
     * influence/modulate/dynamically change the generator's signal in some manner.
     * Each generator parameter (also called signal unit parameter) can receive
     * signal from another signal unit and use this signal to dynamically change the
     * behavior of the signal generator. In turn, the signal of this unit can be fed
     * to another unit(s) and influence its parameters.
     */
    class SignalUnit {
        public:

        /**
         * This class represents a parameter which will influence the signal
         * unit to which it belongs in certain way.
         * For example, let's say the signal unit is a low frequency oscillator
         * with frequency 1Hz. If we want to modulate the LFO to start with 1Hz
         * and increment its frequency to 5Hz in 1 second, we can add
         * a parameter which signal source is an envelope
         * generator with attack time of 1 second and coefficient 4. Thus, the
         * normalized level of the EG will move from 0 to 1 in one second.
         * On every time step (sample point) the normalized level
         * will be multiplied by 4 (the parameter coefficient) and added to the
         * LFO's frequency.
         * So, after 1 second, the LFO frequency will be 1x4 + 1 = 5Hz.
         * We can also add another parameter for modulating the LFO's pitch depth
         * and so on.
         */
        class Parameter {
            public:
                SignalUnit* pUnit; /* The source unit whose output signal 
                                    * will modulate the parameter.
                                    */
                
                float Coeff; // The modulation coefficient
                
                
                Parameter() : pUnit(NULL), Coeff(1) { }

                /**
                 * @param unit The source unit used to influence this parameter.
                 * @param coeff The coefficient by which the normalized signal
                 * received from the source unit should be multiplied when a
                 * default transformation is done.
                 */
                Parameter(SignalUnit* unit, float coeff = 1) {
                    pUnit = unit;
                    Coeff = coeff;
                }

                Parameter(const Parameter& Prm) { Copy(Prm); }
                void operator=(const Parameter& Prm) { Copy(Prm); }
            
                void Copy(const Parameter& Prm) {
                    if (this == &Prm) return;

                    pUnit = Prm.pUnit;
                    Coeff = Prm.Coeff;
                }
                
                
                /**
                 * Calculates the transformation for this parameter
                 * which should be applied to the parameter's value
                 * and multiplies by Coeff.
                 * This implementation of the method just multiplies by Coeff.
                 */
                virtual float Transform(float val) {
                    return val * Coeff;
                }
                
                /**
                 * Gets the current value of the parameter.
                 * This implementation returns the current signal level of the
                 * source unit with applied transformation if the source unit is
                 * active, otherwise returns 1.
                 * Note that this method assume that pUnit is not NULL.
                 */
                virtual float GetValue() {
                    return pUnit->Active() ? Transform(pUnit->GetLevel()) : 1.0f;
                }
        };


        public:
            ArrayList<SignalUnit::Parameter> Params; // The list of parameters which are modulating the signal unit
            
            SignalUnit(SignalUnitRack* rack): pRack(rack), bActive(false), Level(0.0f), bRecalculate(true), bCalculating(false), uiDelayTrigger(0) { }
            SignalUnit(const SignalUnit& Unit): pRack(Unit.pRack) { Copy(Unit); }
            void operator=(const SignalUnit& Unit) { Copy(Unit); }
            virtual ~SignalUnit() { }
            
            void Copy(const SignalUnit& Unit) {
                if (this == &Unit) return;

                bActive = Unit.bActive;
                Level   = Unit.Level;
                Params  = Unit.Params;
                uiDelayTrigger = Unit.uiDelayTrigger;
                bCalculating = false;
            }
                
            /*
             * Determines whether the unit is active.
             * If the unit is not active, its level should be ignored.
             * For endpoint unit this method determines whether
             * the rendering should be stopped.
             */
            virtual bool Active() { return bActive; }
            
            /**
             * Override this method to process the current control change events.
             * @param itEvent - iterator pointing to the event to be processed.
             */
            virtual void ProcessCCEvent(uint8_t Controller, uint8_t Value) { }
            
            virtual void EnterReleaseStage() { }
            
            virtual void CancelRelease() { }
        
            /**
             * Gets the normalized level of the unit for the current
             * time step (sample point). The level is calculated if it's not
             * calculated for the current step yet. Because the level depends on
             * the parameters, their levels are calculated too.
             */
            virtual float GetLevel() {
                if (Params.empty() || !bRecalculate) return Level;

                if (bCalculating) {
                    std::cerr << "SignalUnit: Loop detected. Aborted!";
                    return Level;
                }

                bCalculating = true;
                
                for(int i = 0; i < Params.size(); i++) {
                    Params[i].GetValue();
                }
                
                bRecalculate = bCalculating = false;
                return Level;
            }
        
            /**
             * Will be called to increment the time with one sample point.
             * The unit should recalculate or prepare for recalculation
             * its current level on every call of this function.
             * Note that it is not known whether all source signal unit's levels
             * are recalculated before the call of this method. So, the calculations
             * that depends on the unit's parameters should be postponed to
             * the call of GetLevel().
             */
            virtual void Increment() { bRecalculate = true; }

            /**
             * Initializes and triggers the unit.
             * Note that when a voice is the owner of a unit rack, all settings
             * should be reset when this method is called, because the sampler
             * is reusing the voice objects.
             */
            virtual void Trigger() = 0;
            
            /**
             * When the signal unit rack is triggered, it triggers all signal
             * units it holds. If for some reason the triggering of a unit
             * should be delayed, this method can be set to return non-zero value
             * specifying the delay in time steps.
             * Note that this is only a helper method and the implementation
             * should be done manually.
             */
            virtual uint DelayTrigger() { return uiDelayTrigger; }
            
            /**
             * A helper method which checks whether the delay
             * stage is finished.
             */
            bool DelayStage();
            
        protected:
            SignalUnitRack* const pRack;

            bool   bActive; /* Don't use it to check the active state of the unit!!!
                             * Use Active() instead! */
            float  Level;
            bool   bRecalculate; /* Determines whether the unit's level should be recalculated. */
            bool   bCalculating; /* Determines whether the unit is in process of calculating
                                  * its level. Used for preventing infinite loops.
                                  */

            uint   uiDelayTrigger; /* in time steps */
        
    };
    
    class EndpointSignalUnit: public SignalUnit {
        public:
            EndpointSignalUnit(SignalUnitRack* rack): SignalUnit(rack) { }

            /**
             * Gets the volume modulation value
             * for the current time step (sample point).
             */
            virtual float GetVolume() = 0;

            /**
             * Gets the filter cutoff frequency modulation value
             * for the current time step (sample point).
             */
            virtual float GetFilterCutoff() = 0;

            /**
             * Gets the pitch modulation value
             * for the current time step (sample point).
             */
            virtual float GetPitch() = 0;

            /**
             * Gets the resonance modulation value
             * for the current time step (sample point).
             */
            virtual float GetResonance() = 0;
            
            /** Should return value in the range [-100, 100] (L <-> R) */
            virtual float GetPan() = 0;
            
            virtual float CalculateFilterCutoff(float cutoff) {
                cutoff *= GetFilterCutoff();
                return cutoff > 13500 ? 13500 : cutoff;
            }
            
            virtual float CalculatePitch(float pitch) {
                return GetPitch() * pitch;
            }
            
            virtual float CalculateResonance(float res) {
                return GetResonance() * res;
            }
            
            /** Should return value in the range [0, 127] (L <-> R) */
            virtual uint8_t CalculatePan(int pan) {
                int p = pan + GetPan() * 0.63;
                if (p < 0) return 0;
                if (p > 127) return 127;
                return p;
            }
            
            /**
             * Decreases the delay by Sample time steps.
             * This method is used to delay the sample playback.
             * While the endpoint unit is in delay stage the rack is not incremented.
             */
            void DecreaseDelay(uint Samples) {
                uiDelayTrigger -= Samples;
            }
    };
    
    /**
     * Used to smooth out the parameter changes.
     */
    class Smoother {
        protected:
            uint  timeSteps; // The number of time steps to reach the goal
            uint  currentTimeStep;
            float goal;
            float prev;
            
        public:
            /**
             * 
             * @param time The time (in seconds) to reach the goal
             * @param sampleRate
             * @param val The initial value
             */
            void trigger(float time, float sampleRate, float val = 0) {
                currentTimeStep = timeSteps = time * sampleRate;
                prev = goal = val;
            }
            
            /**
             * Set the current value, which the smoother will not smooth out.
             * If you want the value to be smoothen out, use update() instead.
             */
            void setValue( float val) {
                currentTimeStep = timeSteps;
                prev = goal = val;
            }
            
            /**
             * Sets a new value. The render function will return
             * values gradually approaching this value.
             */
            void update(float val) {
                if (val == goal) return;
                
                prev = prev + (goal - prev) * (currentTimeStep / (float)timeSteps);
                goal = val;
                currentTimeStep = 0;
            }
            
            float render() {
                if (currentTimeStep >= timeSteps) return goal;
                return prev + (goal - prev) * (currentTimeStep++ / (float)timeSteps);
            }
            
            bool isSmoothingOut() { return currentTimeStep < timeSteps; }
            
            float getGoal() { return goal; }
    };
    
    /**
     * Continuous controller signal unit.
     * The level of this unit corresponds to the controllers changes
     * and their influences.
     */
    class CCSignalUnit: public SignalUnit {
        public:
            /** Listener which will be notified when the level of the unit is changed. */
            class Listener {
                public:
                    virtual void ValueChanged(CCSignalUnit* pUnit) = 0;
            };
            
            class CC {
                public:
                    uint8_t   Controller;  ///< MIDI controller number.
                    uint8_t   Value;       ///< Controller Value.
                    short int Curve;       ///< specifies the curve type
                    float     Influence;
                    float     Step;
                    
                    Smoother* pSmoother;
                    
                    CC (
                        uint8_t   Controller = 0,
                        float     Influence  = 0.0f,
                        short int Curve      = -1,
                        Smoother* pSmoother  = NULL,
                        float     Step       = 0
                    ) {
                        this->Controller = Controller;
                        this->Value = 0;
                        this->Curve = Curve;
                        this->Influence = Influence;
                        this->pSmoother = pSmoother;
                        this->Step  = Step;
                    }
                    
                    CC(const CC& cc) { Copy(cc); }
                    void operator=(const CC& cc) { Copy(cc); }
                    
                    void Copy(const CC& cc) {
                        Controller = cc.Controller;
                        Value      = cc.Value;
                        Influence  = cc.Influence;
                        Curve      = cc.Curve;
                        pSmoother  = cc.pSmoother;
                        Step       = cc.Step;
                    }
            };
            
        protected:
            RTList<CC>* pCtrls; // The MIDI controllers which modulates this signal unit.
            Listener* pListener;
            bool hasSmoothCtrls; // determines whether there are smooth controllers (used for optimization)
            bool isSmoothingOut; // determines whether there is a CC which is in process of smoothing out (used for optimization)

        public:
            
            CCSignalUnit(SignalUnitRack* rack, Listener* l = NULL): SignalUnit(rack), pCtrls(NULL) {
                pListener = l;
                hasSmoothCtrls = isSmoothingOut = false;
            }
            
            CCSignalUnit(const CCSignalUnit& Unit): SignalUnit(Unit.pRack), pCtrls(NULL) { Copy(Unit); }
            void operator=(const CCSignalUnit& Unit) { Copy(Unit); }
            
            virtual ~CCSignalUnit() {
                if (pCtrls != NULL) delete pCtrls;
            }
            
            void Copy(const CCSignalUnit& Unit) {
                if (pCtrls != NULL) delete pCtrls;
                pCtrls = new RTList<CC>(*(Unit.pCtrls));
                if (pCtrls->poolIsEmpty() && pCtrls->count() < Unit.pCtrls->count()) {
                    std::cerr << "Maximum number of CC reached!" << std::endl;
                }
                
                pListener = Unit.pListener;
                hasSmoothCtrls = Unit.hasSmoothCtrls;
                isSmoothingOut = Unit.isSmoothingOut;
                SignalUnit::Copy(Unit);
            }
            
            virtual void InitCCList(Pool<CC>* pCCPool, Pool<Smoother>* pSmootherPool) {
                if (pCtrls != NULL) delete pCtrls;
                pCtrls = new RTList<CC>(pCCPool);
            }
            
            void AddCC(uint8_t Controller, float Influence, short int Curve = -1, Smoother* pSmoother = NULL, float Step = 0) {
                if(pCtrls->poolIsEmpty()) {
                    std::cerr << "Maximum number of CC reached!" << std::endl;
                    return;
                }
                *(pCtrls->allocAppend()) = CC(Controller, Influence, Curve, pSmoother, Step);
                if (pSmoother != NULL) hasSmoothCtrls = true;
            }
            
            virtual void RemoveAllCCs() { pCtrls->clear(); }
            
            int GetCCCount() { return pCtrls->count(); }
            
            bool HasCCs() { return GetCCCount() > 0; }
            
            virtual void Increment() {
                if (hasSmoothCtrls && isSmoothingOut) Calculate();
            }
            
            virtual void Trigger() {
                Calculate();
                bActive = Level != 0;
            }
            
            virtual void ProcessCCEvent(uint8_t Controller, uint8_t Value) {
                bool recalculate = false;
                
                RTList<CC>::Iterator ctrl = pCtrls->first();
                RTList<CC>::Iterator end  = pCtrls->end();
                for(; ctrl != end; ++ctrl) {
                    if (Controller != (*ctrl).Controller) continue;
                    if ((*ctrl).Value == Value) continue;
                    
                    (*ctrl).Value = Value;
                    
                    if ((*ctrl).Step > 0 && (*ctrl).pSmoother != NULL) {
                        float oldGoal = (*ctrl).pSmoother->getGoal();
                        float newGoal = Normalize(Value, (*ctrl).Curve) * (*ctrl).Influence;
                        newGoal = ((int) (newGoal / (*ctrl).Step)) * (*ctrl).Step;
                        if (oldGoal != newGoal) (*ctrl).pSmoother->update(newGoal);
                    }
                    
                    if ((*ctrl).pSmoother != NULL && (*ctrl).Step <= 0) (*ctrl).pSmoother->update(Value);
                    if (!bActive) bActive = true;
                    recalculate = true;
                }
                
                if (!(hasSmoothCtrls && isSmoothingOut) && recalculate) Calculate();
            }
            
            virtual void Calculate() {
                float l = 0;
                isSmoothingOut = false;
                RTList<CC>::Iterator ctrl = pCtrls->first();
                RTList<CC>::Iterator end  = pCtrls->end();
                for(; ctrl != end; ++ctrl) {
                    if ((*ctrl).pSmoother == NULL) {
                        float val = Normalize((*ctrl).Value, (*ctrl).Curve) * (*ctrl).Influence;
                        if ((*ctrl).Step > 0) val = ( (int)(val / (*ctrl).Step) ) * (*ctrl).Step;
                        l += val;
                    } else {
                        if ((*ctrl).pSmoother->isSmoothingOut()) isSmoothingOut = true;
                        
                        if ((*ctrl).Step > 0) {
                            l += (*ctrl).pSmoother->render();
                        } else {
                            l += Normalize((*ctrl).pSmoother->render(), (*ctrl).Curve) * (*ctrl).Influence;
                        }
                    }
                }
                if (Level != l) {
                    Level = l;
                    if (pListener != NULL) pListener->ValueChanged(this);
                }
            }
            
            virtual float Normalize(uint8_t val, short int curve = -1) {
                return val / 127.0f;
            }
    };
    
} // namespace LinuxSampler

#endif	/* __LS_SIGNALUNIT_H__ */
