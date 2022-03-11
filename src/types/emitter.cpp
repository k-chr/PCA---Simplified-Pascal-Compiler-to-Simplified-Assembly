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

int unary_op(int, int)
{

}


void Emitter::end_program()
{
	if (this->symtab_ptr->scope() != scope::GLOBAL)
	{
		throw CompilerException("Cannot emit program exit if SymTable object is not in scope::GLOBAL", lineno);
	}
	
	this->emit_to_stream("\t\t", this->mnemonics.at(opcode::EXIT), ";\texit.");
}

void Emitter::label(int label_id)
{
	auto& symbol = this->symtab_ptr->get(label_id);
	if (symbol.entry == entry::LABEL)
	{
		return this->emit_to_stream(interpolate("{0}:", symbol.name), "", "");
	}

	throw CompilerException("Unknown error.", lineno);
}

void Emitter::make_call(int proc_or_fun, const std::vector<int> &args)
{

}

int Emitter::and_then(int, int)
{

}

int Emitter::or_else(int, int)
{

}

int Emitter::andorop(Symbol& and_or, Symbol& first, Symbol& second)
{

}

void Emitter::assign(int lval, int rval)
{
	auto& lval_sym = this->symtab_ptr->get(lval);

	if (lval_sym.entry != entry::VAR)
	{
		throw CompilerException("Syntax error. Variable expected as a left side of the assignment.", lineno);
	}

	auto& rval_sym = this->symtab_ptr->get(rval);

	if (rval_sym.entry != entry::VAR and rval_sym.entry != entry::NUM)
	{
		throw CompilerException("Syntax error. Variable or numeric constant expected as a right side of the assignment.", lineno);
	}

	if (lval_sym.dtype != rval_sym.dtype)
	{
		rval_sym = this->symtab_ptr->get(this->cast(rval_sym, lval_sym.dtype));
	}

	auto op = this->mnemonics.at(opcode::MOV) + this->get_type_str(lval_sym.dtype);

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}\t{2}", op, this->mnemonics.at(opcode::MOV), rval_sym.name, lval_sym.name), rval_sym.addr_to_str(true), lval_sym.addr_to_str(true));
}

void Emitter::jump(int where)
{
	auto& label = this->symtab_ptr->get(where);
	if (label.entry != entry::LABEL)
	{
		throw CompilerException("Unknown error.", lineno);
	}

	auto op = this->mnemonics.at(opcode::JMP) + ".i";

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}", op, label.name), label.addr_to_str());
}

int Emitter::relop(Symbol& op_symbol, Symbol& first, Symbol& second)
{
	if(not (op_symbol.entry == entry::OP and (
			first.entry == entry::NUM or first.entry == entry::VAR
		) and (
			second.entry == entry::NUM or second.entry == entry::VAR
		)
	)) throw CompilerException("Unknown error.", lineno);

	auto op_type = this->symtab_ptr->infer_type(first, second);
	auto op_code = opcode(op_symbol.address);
	auto type = dtype::INT;
	auto temp = this->symtab_ptr->get(this->symtab_ptr->insert(type));
	auto mnemonic = this->mnemonics.at(op_code);
	auto op = mnemonic + this->get_type_str(op_type);

	auto true_label = this->symtab_ptr->get(this->symtab_ptr->insert(mnemonic + "true"));
	auto false_label = this->symtab_ptr->get(this->symtab_ptr->insert(mnemonic + "false"));
	
	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}, {2}, {3}", mnemonic, first.name, second.name, true_label.addr_to_str()),
						 first.addr_to_str(true), second.addr_to_str(true), true_label.addr_to_str());

	
	this->assign(temp.symtab_id, this->symtab_ptr->insert("0", type));
	this->jump(false_label.symtab_id);
	this->label(true_label.symtab_id);
	this->assign(temp.symtab_id, this->symtab_ptr->insert("1", type));
	this->label(false_label.symtab_id);

	return temp.symtab_id;
}

int Emitter::mulop(Symbol& op_symbol, Symbol& first, Symbol& second)
{
	if(not (op_symbol.entry == entry::OP and (
			first.entry == entry::NUM or first.entry == entry::VAR
		) and (
			second.entry == entry::NUM or second.entry == entry::VAR
		)
	)) throw CompilerException("Unknown error.", lineno);


	auto type = this->symtab_ptr->infer_type(first, second);
	auto op_code = opcode(op_symbol.address);
	
	if (type != first.dtype)
	{
		first = this->symtab_ptr->get(this->cast(first, type));
	}

	if (type != second.dtype)
	{
		second = this->symtab_ptr->get(this->cast(second, type));
	}
	
	auto temp = this->symtab_ptr->get(this->symtab_ptr->insert(type));
	auto op = this->mnemonics.at(op_code) + this->get_type_str(type);

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}, {2}, {3}", op, first.name, second.name, temp.name), 
						 first.addr_to_str(true), second.addr_to_str(true), temp.addr_to_str(true));

	return temp.symtab_id;
}

int Emitter::binary_op(int op_id, int operand1, int operand2)
{	
	auto &first = this->symtab_ptr->get(operand1);
	auto &second = this->symtab_ptr->get(operand2);
	auto &op_symbol= this->symtab_ptr->get(op_id); 

	auto op_code = opcode(op_symbol.address);

	if (first.entry == entry::ARR or second.entry == entry::ARR)
	{
		throw CompilerException(interpolate("No matching overload of {0} for Array type.", this->mnemonics.at(op_code)), lineno);
	}

	if ((first.entry != entry::VAR and first.entry != entry::NUM) or (second.entry != entry::VAR and second.entry != entry::NUM))
	{
		throw CompilerException("Unknown error.", lineno);
	}

	switch (op_code) {
		case opcode::ADD: 
		case opcode::SUB:
		case opcode::MUL:
		case opcode::DIV:
		case opcode::MOD:
			return mulop(op_symbol, first, second);
		case opcode::AND:
		case opcode::OR:
			return andorop(op_symbol, first, second);
		case opcode::NE:
		case opcode::LE:
		case opcode::LT:
		case opcode::GE:
		case opcode::GT:
		case opcode::EQ:
			return relop(op_symbol, first, second);
		default: throw CompilerException(interpolate("Invalid operation: {0} is not binary operator", op_symbol.name), lineno);
	}
}

int Emitter::cast(Symbol& symbol, dtype& to)
{
	if (symbol.entry == entry::ARR)
	{
		throw CompilerException("Cannot perform type cast for Array type", lineno);
	}

	if (symbol.entry != entry::VAR)
	{
		throw CompilerException("Unknown error.", lineno);
	}

	if (symbol.dtype == to) return symbol.symtab_id;

	opcode opcd = opcode::NOP;
	switch (to) 
	{
        case dtype::INT:
		{	
			opcd = opcode::R2I;
			break;
		}
        case dtype::REAL:
		{
			opcd = opcode::I2R;
			break;
		}
        case dtype::NONE:
          break;
    }

	if (opcd == opcode::NOP)
	{
		throw CompilerException("Unknown error.", lineno);
	}
	
	auto return_id = this->symtab_ptr->insert(to);
	auto& temp = this->symtab_ptr->get(return_id);
	auto& mnemonic = this->mnemonics.at(opcd);
	auto op = mnemonic + this->get_type_str(to);

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}, {2}", op, symbol.name, temp.name), symbol.addr_to_str(true), temp.addr_to_str(true));

	return return_id;
}

int Emitter::cast(int id, dtype & to)
{
	auto& sym = this->symtab_ptr->get(id);
	return this->cast(sym, to);
}

std::ostream& Emitter::get_stream()
{
	return this->symtab_ptr->scope() == scope::GLOBAL ? this->output : this->mem;
}
