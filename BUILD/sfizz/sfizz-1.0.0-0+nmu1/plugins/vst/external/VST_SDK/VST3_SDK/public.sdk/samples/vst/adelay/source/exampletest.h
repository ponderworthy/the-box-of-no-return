//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/adelay/source/exampletest.h
// Created by  : Steinberg, 10/2010
// Description : 
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2021, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#ifndef __exampletest__
#define __exampletest__

#include "base/source/fobject.h"
#include "pluginterfaces/test/itest.h"

namespace Steinberg {
namespace Vst {
class ITestPlugProvider;

//-----------------------------------------------------------------------------
class ADelayTest : public ITest, public FObject
{
public:
	ADelayTest (ITestPlugProvider* plugProvider);

	//--ITest
	bool PLUGIN_API setup () SMTG_OVERRIDE;
	bool PLUGIN_API run (ITestResult* testResult) SMTG_OVERRIDE;
	bool PLUGIN_API teardown () SMTG_OVERRIDE;
	const tchar* PLUGIN_API getDescription () SMTG_OVERRIDE;

	OBJ_METHODS(ADelayTest, FObject)
	DEF_INTERFACES_1(ITest, FObject)
	REFCOUNT_METHODS(FObject)
protected:
	ITestPlugProvider* plugProvider;
};

//-----------------------------------------------------------------------------
class ADelayTestFactory : public ITestFactory, public FObject
{
public:
	//--ITestFactory
	tresult PLUGIN_API createTests (FUnknown* context, ITestSuite* parentSuite) SMTG_OVERRIDE;

	static FUnknown* createInstance (void*) { return (ITestFactory*)new ADelayTestFactory (); }

	static FUID cid;

	OBJ_METHODS(ADelayTestFactory, FObject)
	DEF_INTERFACES_1(ITestFactory, FObject)
	REFCOUNT_METHODS(FObject)
};

}} // namespaces

#endif
