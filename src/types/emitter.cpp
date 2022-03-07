#include "emitter.hpp"
#include <ostream>
#include <string>

const std::map<opcode, std::string> Emitter::mnemonics = {
	{opcode::ADD,       "add"},
	{opcode::SUB,       "sub"},
	{opcode::MUL,       "mul"},
	{opcode::DIV,       "div"},
	{opcode::MOD,       "mod"},
	{opcode::AND,       "and"},
	{opcode::OR,         "or"},
	{opcode::NE,        "jne"},
	{opcode::LE,        "jle"},
	{opcode::LT,         "jl"},
	{opcode::GE,        "jge"},
	{opcode::GT,         "jg"},
	{opcode::EQ,         "je"},
	{opcode::I2R, "inttoreal"},
	{opcode::R2I, "realtoint"},
	{opcode::MOV,       "mov"},
	{opcode::JMP,      "jump"},
	{opcode::WRT,     "write"},
	{opcode::RD,       "read"},
	{opcode::PSH,      "push"},
	{opcode::CALL,     "call"},
	{opcode::INCSP,   "incsp"},
	{opcode::RET,    "return"},
	{opcode::LEAVE,   "leave"},
	{opcode::EXIT,     "exit"}
};

std::string Emitter::get_type_str(const dtype& type)
{
	std::string out;
	switch (type) 
	{
		case dtype::REAL:
		{
			out =".r";
			break;
		}
		case dtype::INT:
		{
			out =".i";
			break;
		}
       	case dtype::NONE: break;
    }

	return out;
}

void Emitter::end_program()
{

}

void Emitter::label(int label_id)
{

}

void Emitter::make_call(int proc_or_fun, const std::vector<int> &args)
{

}

void Emitter::jump_if(int op, int comp, int comp1, int if_true)
{

}

int Emitter::binary_op(int op_id, int operand1, int operand2)
{	
	auto &first = this->symtab_ptr->get(operand1);
	auto &second = this->symtab_ptr->get(operand2);
	auto &op= this->symtab_ptr->get(op_id); 

	auto op_code = opcode(op.address);

	if (first.entry == entry::ARR or second.entry == entry::ARR)
	{
		throw CompilerException(interpolate("No matching overload of {0} for Array type", this->mnemonics.at(op_code)), lineno);
	}

	auto type = this->symtab_ptr->infer_type(first, second);
	
	
	
	auto temp_id = this->symtab_ptr->insert(type);


	return temp_id;
}

int Emitter::cast(int id, dtype & to)
{
	auto& sym = this->symtab_ptr->get(id);

	if (sym.dtype == to) return id;

	opcode op = opcode::NOP;
	switch (to) 
	{
        case dtype::INT:
		{	
			op = opcode::R2I;
			break;
		}
        case dtype::REAL:
		{
			op = opcode::I2R;
			break;
		}
        case dtype::NONE:
          break;
    }

	auto return_id = this->symtab_ptr->insert(to);

	

	return return_id;
}

std::ostream& Emitter::get_stream()
{
	return this->symtab_ptr->scope() == scope::GLOBAL ? this->output : this->mem;
}
