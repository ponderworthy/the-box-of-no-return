/*
 * Copyright (c) 2014-2017 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "InstrumentScriptVMFunctions.h"
#include "InstrumentScriptVM.h"
#include "EngineChannel.h"

namespace LinuxSampler { namespace gig {

  /////////////////////////////////////////////////////////////////////////
  // Function:
  //     gig_set_dim_zone(event_id, dimension, zone)

    InstrumentScriptVMFunction_gig_set_dim_zone::InstrumentScriptVMFunction_gig_set_dim_zone(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    bool InstrumentScriptVMFunction_gig_set_dim_zone::acceptsArgType(int iArg, ExprType_t type) const {
        return type == INT_EXPR || type == INT_ARR_EXPR;
    }

    VMFnResult* InstrumentScriptVMFunction_gig_set_dim_zone::exec(VMFnArgs* args) {
        EngineChannel* pEngineChannel =
            static_cast<EngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        int dim  = args->arg(1)->asInt()->evalInt();
        int zone = args->arg(2)->asInt()->evalInt();

        if (args->arg(0)->exprType() == INT_EXPR) {
            const ScriptID id = args->arg(0)->asInt()->evalInt();
            if (!id) {
                wrnMsg("gig_set_dim_zone(): note ID for argument 1 may not be zero");
                return successResult();
            }
            if (!id.isNoteID()) {
                wrnMsg("gig_set_dim_zone(): argument 1 is not a note ID");
                return successResult();
            }

            NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
            if (!pNote) {
                dmsg(2,("gig_set_dim_zone(): no active note with ID %d\n", int(id)));
                return successResult();
            }

            int note = pNote->cause.Param.Note.Key;

            ::gig::Region* pRegion = pEngineChannel->pInstrument->GetRegion(note);
            if (!pRegion) {
                dmsg(2,("gig_set_dim_zone(): no region for key %d\n", note));
                return successResult();
            }

            int idx = -1, baseBits = 0;
            for (int i = 0; i < pRegion->Dimensions; ++i) {
                if (pRegion->pDimensionDefinitions[i].dimension == dim) {
                    idx = i;
                    break;
                }
                baseBits += pRegion->pDimensionDefinitions[i].bits;
            }
            if (idx < 0) {
                dmsg(2,("gig_set_dim_zone(): no such gig dimension %d\n", dim));
                return successResult(); // no such dimension found
            }

            int bits = pRegion->pDimensionDefinitions[idx].bits;
            int mask = 0;
            for (int i = 0; i < bits; ++i) mask |= (1 << (baseBits + i));

            pNote->Format.Gig.DimMask |= mask;
            pNote->Format.Gig.DimBits |= (zone << baseBits) & mask;

            dmsg(3,("gig_set_dim_zone(): success, mask=%d bits=%d\n", pNote->Format.Gig.DimMask, pNote->Format.Gig.DimBits));
        } else if (args->arg(0)->exprType() == INT_ARR_EXPR) {
            VMIntArrayExpr* ids = args->arg(0)->asIntArray();
            for (int i = 0; i < ids->arraySize(); ++i) {
                const ScriptID id = ids->evalIntElement(i);
                if (!id || !id.isNoteID()) continue;

                NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
                if (!pNote) continue;

                int note = pNote->cause.Param.Note.Key;

                ::gig::Region* pRegion = pEngineChannel->pInstrument->GetRegion(note);
                if (!pRegion) continue;

                int idx = -1, baseBits = 0;
                for (int i = 0; i < pRegion->Dimensions; ++i) {
                    if (pRegion->pDimensionDefinitions[i].dimension == dim) {
                        idx = i;
                        break;
                    }
                    baseBits += pRegion->pDimensionDefinitions[i].bits;
                }
                if (idx < 0) continue;

                int bits = pRegion->pDimensionDefinitions[idx].bits;
                int mask = 0;
                for (int i = 0; i < bits; ++i) mask |= (1 << (baseBits + i));

                pNote->Format.Gig.DimMask |= mask;
                pNote->Format.Gig.DimBits |= (zone << baseBits) & mask;

                dmsg(3,("gig_set_dim_zone(): success, mask=%d bits=%d\n", pNote->Format.Gig.DimMask, pNote->Format.Gig.DimBits));
            }
        }

        return successResult();
    }

  /////////////////////////////////////////////////////////////////////////
  // Function:
  //     same_region(key1, key2)

    InstrumentScriptVMFunction_same_region::InstrumentScriptVMFunction_same_region(InstrumentScriptVM* parent)
    : m_vm(parent)
    {
    }

    VMFnResult* InstrumentScriptVMFunction_same_region::exec(VMFnArgs* args) {
        EngineChannel* pEngineChannel =
            static_cast<EngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        int key1 = args->arg(0)->asInt()->evalInt();
        int key2 = args->arg(1)->asInt()->evalInt();

        if (key1 < 0 || key1 > 127) {
            wrnMsg("same_region(): key number for argument 1 out of range");
            return successResult(-1);
        }
        if (key2 < 0 || key2 > 127) {
            wrnMsg("same_region(): key number for argument 2 out of range");
            return successResult(-1);
        }

        ::gig::Region* pRgn1 = pEngineChannel->pInstrument->GetRegion(key1);
        ::gig::Region* pRgn2 = pEngineChannel->pInstrument->GetRegion(key2);

        if (!pRgn1 && !pRgn2)
            return successResult(5);
        if (pRgn1 == pRgn2)
            return successResult(1);
        if (pRgn1 && !pRgn2)
            return successResult(3);
        if (!pRgn1 && pRgn2)
            return successResult(4);
        if (pRgn1->KeyRange.overlaps(pRgn2->KeyRange))
            return successResult(2);
        return successResult(0);
    }

}} // namespace LinuxSampler::gig
