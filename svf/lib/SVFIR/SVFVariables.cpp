//===- SVFVariables.cpp -- SVF symbols and variables----------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013->  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//

/*
 * SVFVariables.cpp
 *
 *  Created on: Oct 11, 2013
 *      Author: Yulei Sui
 */

#include "SVFIR/SVFVariables.h"
#include "Util/Options.h"
#include "Util/SVFUtil.h"
#include "Graphs/CallGraph.h"

using namespace SVF;
using namespace SVFUtil;


/*!
 * SVFVar constructor
 */
SVFVar::SVFVar(const SVFValue* val, NodeID i, PNODEK k) :
    GenericPAGNodeTy(i,k), value(val), func(nullptr)
{
    assert( ValNode <= k && k <= DummyObjNode && "new SVFIR node kind?");
    switch (k)
    {
    case ValNode:
    case ArgNode:
    case ConstantDataValNode:
    case GlobalValNode:
    case BlackHoleNode:
    case ConstantFPValNode:
    case ConstantIntValNode:
    case ConstantNullptrValNode:
    case GepValNode:
    {
        assert(val != nullptr && "value is nullptr for ValVar or GepValNode");
        isPtr = val->getType()->isPointerTy();
        break;
    }
    case FunValNode:
    case VarargNode:
    case DummyValNode:
    {
        isPtr = true;
        break;
    }
    case ObjNode:
    case GepObjNode:
    case BaseObjNode:
    case ConstantDataObjNode:
    case GlobalObjNode:
    case ConstantFPObjNode:
    case ConstantIntObjNode:
    case ConstantNullptrObjNode:
    case DummyObjNode:
    {
        isPtr = true;
        if(val)
            isPtr = val->getType()->isPointerTy();
        break;
    }
    case RetNode:
    case FunObjNode:
    case HeapObjNode:
    case StackObjNode:
    {
        // to be completed in derived class
        break;
    }
    default:
        assert(false && "var not handled");
        break;
    }
}

bool SVFVar::isIsolatedNode() const
{
    if (getInEdges().empty() && getOutEdges().empty())
        return true;
    else if (isConstDataOrAggDataButNotNullPtr())
        return true;
    else
        return false;
}


const std::string SVFVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "SVFVar ID: " << getId();
    return rawstr.str();
}

void SVFVar::dump() const
{
    outs() << this->toString() << "\n";
}

const std::string ValVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "ValVar ID: " << getId();
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << value->toString();
    }
    return rawstr.str();
}

const std::string ObjVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "ObjVar ID: " << getId();
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << value->toString();
    }
    return rawstr.str();
}

ArgValVar::ArgValVar(NodeID i, u32_t argNo, const ICFGNode* icn,
                     const SVF::CallGraphNode* callGraphNode, bool isUncalled,
                     SVF::SVFVar::PNODEK ty)
    : ValVar(callGraphNode->getFunction()->getArg(argNo), i, ty, icn),
      cgNode(callGraphNode), argNo(argNo), uncalled(isUncalled)
{
    isPtr =
        callGraphNode->getFunction()->getArg(argNo)->getType()->isPointerTy();
}

const SVFFunction* ArgValVar::getFunction() const
{
    return getParent();
}

const SVFFunction* ArgValVar::getParent() const
{
    return cgNode->getFunction();
}

const std::string ArgValVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "ArgValVar ID: " << getId();
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << valueOnlyToString();
    }
    return rawstr.str();
}

const std::string GepValVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "GepValVar ID: " << getId() << " with offset_" + std::to_string(getConstantFieldIdx());
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << value->toString();
    }
    return rawstr.str();
}

RetPN::RetPN(NodeID i, const CallGraphNode* node) : ValVar(i, RetNode), callGraphNode(node)
{
    isPtr = node->getFunction()->getReturnType()->isPointerTy();
}

const SVFFunction* RetPN::getFunction() const
{
    return callGraphNode->getFunction();
}

const std::string RetPN::getValueName() const
{
    return callGraphNode->getName() + "_ret";
}

const std::string GepObjVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "GepObjVar ID: " << getId() << " with offset_" + std::to_string(apOffset);
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << value->toString();
    }
    return rawstr.str();
}

const std::string BaseObjVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "BaseObjVar ID: " << getId() << " (base object)";
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << value->toString();
    }
    return rawstr.str();
}

HeapObjVar::HeapObjVar(NodeID i, const MemObj* mem, const SVFType* svfType,
                       const SVFFunction* f, PNODEK ty)
    : BaseObjVar(mem->getValue(), i, mem, ty)
{
    isPtr = svfType->isPointerTy();
    func = f;
}

const std::string HeapObjVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "HeapObjVar ID: " << getId();
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << valueOnlyToString();
    }
    return rawstr.str();
}

StackObjVar::StackObjVar(NodeID i, const MemObj* mem, const SVFType* svfType, const SVFFunction* f, PNODEK ty)
    : BaseObjVar(mem->getValue(), i, mem, ty)
{
    isPtr = svfType->isPointerTy();
    func = f;
}

const std::string StackObjVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "StackObjVar ID: " << getId();
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << valueOnlyToString();
    }
    return rawstr.str();
}



FunValVar::FunValVar(NodeID i, const ICFGNode* icn, const CallGraphNode* cgn, PNODEK ty)
    : ValVar(cgn->getFunction(), i, ty, icn), callGraphNode(cgn)
{
    isPtr = cgn->getFunction()->getType()->isPointerTy();
}

const std::string FunValVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "FunValVar ID: " << getId();
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << callGraphNode->getName();
    }
    return rawstr.str();
}

const std::string ConstantDataValVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "ConstantDataValNode ID: " << getId();
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << valueOnlyToString();
    }
    return rawstr.str();
}

const std::string GlobalValVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "GlobalValVar ID: " << getId();
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << valueOnlyToString();
    }
    return rawstr.str();
}

const std::string ConstantFPValVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "ConstantFPValNode ID: " << getId();
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << valueOnlyToString();
    }
    return rawstr.str();
}

const std::string ConstantIntValVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "ConstantIntValNode ID: " << getId();
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << valueOnlyToString();
    }
    return rawstr.str();
}

const std::string ConstantNullPtrValVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "ConstantNullPtrValVar ID: " << getId();
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << valueOnlyToString();
    }
    return rawstr.str();
}

const std::string GlobalObjVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "GlobalObjNode ID: " << getId();
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << valueOnlyToString();
    }
    return rawstr.str();
}

const std::string ConstantDataObjVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "ConstantDataObjVar ID: " << getId();
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << valueOnlyToString();
    }
    return rawstr.str();
}

const std::string ConstantFPObjVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "ConstantFPObjVar ID: " << getId();
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << valueOnlyToString();
    }
    return rawstr.str();
}

const std::string ConstantIntObjVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "ConstantIntObjVar ID: " << getId();
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << valueOnlyToString();
    }
    return rawstr.str();
}

const std::string ConstantNullPtrObjVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "ConstantNullPtrObjVar ID: " << getId();
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << valueOnlyToString();
    }
    return rawstr.str();
}

FunObjVar::FunObjVar(NodeID i, const MemObj* mem, const CallGraphNode* cgNode,
                     PNODEK ty)
    : BaseObjVar(mem->getValue(), i, mem, ty), callGraphNode(cgNode)
{
    isPtr = callGraphNode->getFunction()->getType()->isPointerTy();
}

bool FunObjVar::isIsolatedNode() const
{
    return callGraphNode->getFunction()->isIntrinsic();
}

const std::string FunObjVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "FunObjVar ID: " << getId() << " (base object)";
    if (Options::ShowSVFIRValue())
    {
        rawstr << "\n";
        rawstr << callGraphNode->getName();
    }
    return rawstr.str();
}

const std::string RetPN::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "RetPN ID: " << getId() << " unique return node for function " << callGraphNode->getName();
    return rawstr.str();
}

const SVFFunction* VarArgPN::getFunction() const
{
    return callGraphNode->getFunction();
}

const std::string VarArgPN::getValueName() const
{
    return callGraphNode->getName() + "_vararg";
}

const std::string VarArgPN::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "VarArgPN ID: " << getId() << " Var arg node for function " << callGraphNode->getName();
    return rawstr.str();
}

const std::string DummyValVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "DummyValVar ID: " << getId();
    return rawstr.str();
}

const std::string DummyObjVar::toString() const
{
    std::string str;
    std::stringstream rawstr(str);
    rawstr << "DummyObjVar ID: " << getId();
    return rawstr.str();
}

/// Whether it is constant data, i.e., "0", "1.001", "str"
/// or llvm's metadata, i.e., metadata !4087
bool SVFVar::isConstDataOrAggDataButNotNullPtr() const
{
    if (hasValue())
        return value->isConstDataOrAggData() && (!SVFUtil::isa<SVFConstantNullPtr>(value)) && (!SVFUtil::isa<SVFBlackHoleValue>(value));
    else
        return false;
}
