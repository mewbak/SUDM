#include "engine.h"
#include "disassembler.h"
#include "codegen.h" 

#include <iostream>
#include <sstream>
#include <boost/format.hpp>

#define GET(vertex) (boost::get(boost::vertex_name, g, vertex))

Disassembler* FF7::FF7WorldEngine::getDisassembler(InstVec &insts)
{
    return new FF7WorldDisassembler(this, insts, mScriptNumber);
}

CodeGenerator* FF7::FF7WorldEngine::getCodeGenerator(std::ostream &output)
{
    return new FF7WorldCodeGenerator(this, output);
}

void FF7::FF7WorldEngine::postCFG(InstVec &insts, Graph g)
{
    /*
    VertexRange vr = boost::vertices(g);
    for (VertexIterator v = vr.first; v != vr.second; ++v) 
    {
        GroupPtr gr = GET(*v);

        // If this group is the last instruction and its an unconditional jump
        if ((*gr->_start)->_address == insts.back()->_address && insts.back()->isUncondJump())
        {
            // Then assume its an infinite do { } while(true) loop that wraps part of the script
            gr->_type = kDoWhileCondGroupType;
        }
    }*/
}

bool FF7::FF7WorldEngine::detectMoreFuncs() const
{
	return false;
}

void FF7::FF7WorldEngine::getVariants(std::vector<std::string> &variants) const
{

}

void FF7::FF7WorldLoadBankInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{
    stack.push(new BankValue("Read(" + _params[0]->getString() + ")"));
}

void FF7::FF7WorldLoadInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{
    stack.push(new VarValue(_params[0]->getString()));
}

void FF7::FF7SubStackInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{
    std::string op;
    switch (_opcode)
    {
    case 0x41:
        op = "-";
        break;

    case 0x40:
        op = "+";
        break;

    case 0x0c0:
        op = "||";
        break;

    case 0x15: // neg
    case 0x17: // not
        stack.push(stack.pop()->negate());
        return;

    case 0x30:
        op = "*";
        break;

    case 0x60:
        op = "<";
        break;

    case 0x61:
        op = ">";
        break;

    case 0x62:
        op = "<=";
        break;

    case 0x0b0:
        op = "&&";
        break;

    case 0x51:
        op = "<<";
        break;

    default:
        op = "unknown_operation";
    }

    std::string strValue = stack.pop()->getString() + " " + op + " " + stack.pop()->getString();
    stack.push(new VarValue(strValue));
}

void FF7::FF7WorldStoreInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{
    std::string strValue = stack.pop()->getString();

    // If the bank address is from a load bank instruction, then we only want
    // the bank address, not whats *at* the bank address
    ValuePtr bankValue = stack.pop();
    std::string bankAddr = bankValue->getString();
    codeGen->addOutputLine("Write(" + bankAddr + ", " + strValue + ");");
}

void FF7::FF7WorldStackInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}

void FF7::FF7WorldCondJumpInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}

uint32 FF7::FF7WorldCondJumpInstruction::getDestAddress() const
{
    return 0x200 + _params[0]->getUnsigned();
}

std::ostream& FF7::FF7WorldCondJumpInstruction::print(std::ostream &output) const
{
    Instruction::print(output);
    return output;
}


bool FF7::FF7WorldUncondJumpInstruction::isFuncCall() const
{
	return _isCall;
}

bool FF7::FF7WorldUncondJumpInstruction::isUncondJump() const
{
	return !_isCall;
}

uint32 FF7::FF7WorldUncondJumpInstruction::getDestAddress() const
{
    // the world map while loops are incorrect without +1'in this, but
    // doing that will break some if elses.. hmm
    return 0x200 + _params[0]->getUnsigned();// +1; // TODO: +1 to skip param?
}

std::ostream& FF7::FF7WorldUncondJumpInstruction::print(std::ostream &output) const
{
    Instruction::print(output);
 //   output << " (Jump target address: 0x" << std::hex << getDestAddress() << std::dec << ")";
    return output;
}


void FF7::FF7WorldUncondJumpInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}

void FF7::FF7WorldKernelCallInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{
    std::string strFunc;
    switch (_opcode)
    {
    case 0x203:
        strFunc = "return;";
        break;

    case 0x317:
        strFunc = "TriggerBattle(" + stack.pop()->getString() + ");";
        break;

    case 0x324:
        // x, y, w, h
        strFunc = "SetWindowDimensions(" + 
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x32D:
        strFunc = "WaitForWindowReady();";
        break;

    case 0x325:
        strFunc = "SetWindowMessage(" + stack.pop()->getString() + ");";
        break;

    case 0x333:
        strFunc = "Unknown333(" +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x32e:
        strFunc = "WaitForMessageAcknowledge();";
        break;

    case 0x32c:
        // Mode, Permanency
        strFunc = "SetWindowParameters(" +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x318:
        strFunc = "EnterFieldScene(" +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x348:
        strFunc = "FadeIn(" +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x33b:
        strFunc = "FadeOut(" +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x31D:
        strFunc = "PlaySoundEffect(" + stack.pop()->getString() + ");";
        break;

    case 0x328:
        strFunc = "SetActiveEntityDirection(" + stack.pop()->getString() + ");";
        break;

    case 0x336: // honours walk mesh
    case 0x303:
        strFunc = "SetActiveEntityMovespeed(" + stack.pop()->getString() + ");";
        break;

    case 0x304:
        strFunc = "SetActiveEntityDirectionAndFacing(" + stack.pop()->getString() + ");";
        break;

    case 0x32b:
        strFunc = "SetBattleLock(" + stack.pop()->getString() + ");";
        break;

    case 0x305:
        strFunc = "SetWaitFrames(" + stack.pop()->getString() + ");";
        break;

    case 0x306:
        strFunc = "Wait();";
        break;

    case 0x307:
        strFunc = "SetControlLock(" + stack.pop()->getString() + ");";
        break;

    default:
        strFunc = "kernel_unknown_" + AddressValue(_opcode).getString() + "();";
        break;
    }
    codeGen->addOutputLine(strFunc);
}

void FF7::FF7WorldNoOutputInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}
