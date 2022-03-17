#include "emitter.hpp"
#include <optional>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

const std::map<opcode, std::string> Emitter::mnemonics = {
	{opcode::ADD,       "add"},
	{opcode::NOT,       "not"},
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
	{opcode::ENTER,	  "enter"},
	{opcode::INCSP,   "incsp"},
	{opcode::RET,    "return"},
	{opcode::LEAVE,   "leave"},
	{opcode::EXIT,     "exit"}
};

std::vector<int> Emitter::get_params()
{
	return this->params;
}

void Emitter::clear_params()
{
	this->params.clear();
}

void Emitter::store_param_on_stack(int symbol_id)
{
	this->params_stack.top().push_back(symbol_id);
}


int Emitter::variable_or_call(int symbol_id)
{

}

void Emitter::begin_parametric_expr()
{
	this->params_stack.push(this->params);
	this->params.clear();
}

void Emitter::end_parametric_expr()
{
	this->params = this->params_stack.top();
	this->params_stack.pop();
}

void Emitter::store_param(int id)
{
	this->params.push_back(id);
}

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

int Emitter::negate(Symbol& symbol)
{
	if(symbol.entry != entry::VAR and symbol.entry != entry::NUM)
	{
		throw CompilerException("Syntax error. Variable or numeric constant expected as a operand.", lineno);
	}

	return this->binop(this->symtab_ptr->get(this->symtab_ptr->lookup("-")), this->symtab_ptr->get(this->symtab_ptr->insert_label("0")), symbol);
} 	

int Emitter::boolean_negate(Symbol& symbol)
{
	if(symbol.entry != entry::VAR and symbol.entry != entry::NUM)
	{
		throw CompilerException("Syntax error. Variable or numeric constant expected as a operand.", lineno);
	}

	auto type = dtype::INT;
	if (symbol.dtype != dtype::INT)
	{
		symbol = this->symtab_ptr->get(this->cast(symbol, type));
	}

	auto mnemonic = this->mnemonics.at(opcode::NOT);
	auto op = mnemonic + this->get_type_str(type);

	auto temp = this->symtab_ptr->get(this->symtab_ptr->insert_temp(type));
	
	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}, {2}", mnemonic, symbol.name, temp.name), symbol.addr_to_str(true), temp.addr_to_str(true));

	return temp.symtab_id;
}

int Emitter::unary_op(int op_id, int operand_id)
{
	auto op_symbol= this->symtab_ptr->get(op_id); 
	auto symbol = this->symtab_ptr->get(operand_id);

	if(op_symbol.entry != entry::OP)
	{
		throw CompilerException("Unknown error.", lineno);
	}
	
	if(symbol.entry != entry::VAR and symbol.entry != entry::NUM)
	{
		throw CompilerException("Syntax error. Variable or numeric constant expected as a operand.", lineno);
	}

	auto opcd = static_cast<opcode>(op_symbol.address);

	switch (opcd) 
	{
		case opcode::ADD:
			return operand_id;
		case opcode::SUB:
			return this->negate(symbol);
		case opcode::NOT:
			return this->boolean_negate(symbol);
		default: throw CompilerException(interpolate("Invalid operation: {0} is not unary operator", op_symbol.name), lineno);
	}
}

void Emitter::start_program(int label_id)
{
	this->label(label_id);
}

void Emitter::call_program(int symbol_id)
{
	auto& symbol = this->symtab_ptr->get(symbol_id);

	if(symbol.scope != scope::UNBOUND)
	{
		throw CompilerException(interpolate("Syntax error. Redefinition of \"{0}\" program.", symbol.name), lineno);
	}

	symbol.scope = scope::GLOBAL;
	symbol.entry = entry::LABEL;

	this->jump(symbol);
}

void Emitter::commit_subprogram()
{
	this->output << this->mem.str();
	this->mem.str("");
}

void Emitter::leave_subprogram()
{
	auto leave_mnemonic = this->mnemonics.at(opcode::LEAVE);
	auto return_mnemonic = this->mnemonics.at(opcode::RET);
	this->emit_to_stream("\t\t", leave_mnemonic, interpolate(";\t{0}", leave_mnemonic));
	this->emit_to_stream("\t\t", return_mnemonic, interpolate(";\t{0}", return_mnemonic));
}

void Emitter::enter(int stack_size)
{
	auto enter_mnemonic	= this->mnemonics.at(opcode::ENTER);
	this->emit_to_stream("\t\t", enter_mnemonic + this->get_type_str(dtype::INT), interpolate(";\t{0}\t{1}", enter_mnemonic, stack_size), "#" + std::to_string(stack_size));
}

void Emitter::end_current_subprogram()
{
	auto stack_size = std::abs(this->symtab_ptr->get_last_addr());

	this->leave_subprogram();
	this->symtab_ptr->return_to_global_scope();
	this->enter(stack_size);
	this->commit_subprogram();

	std::cout << *(this->symtab_ptr);

	this->symtab_ptr->restore_checkpoint();
	this->symtab_ptr->leave_global_scope();
	this->symtab_ptr->create_checkpoint();
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
	auto symbol = this->symtab_ptr->get(label_id);
	return this->label(symbol);
}

void Emitter::label(Symbol& symbol)
{
	if (symbol.entry == entry::LABEL or symbol.entry == entry::PROC or symbol.entry == entry::FUNC)
	{
		return this->emit_to_stream(interpolate("{0}:", symbol.name), "", "");
	}

	throw CompilerException("Unknown error.", lineno);
}

void Emitter::push(Symbol& symbol)
{
	if (symbol.entry != entry::VAR and symbol.entry != entry::ARR)
	{
		throw CompilerException("Unknown error", lineno);
	}

	auto opcd = opcode::PSH;
	auto& mnemonic = this->mnemonics.at(opcd);
	auto op = mnemonic + this->get_type_str(dtype::INT);

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}", mnemonic, symbol.name), symbol.addr_to_str());
}

void Emitter::incsp(int num_of_bytes)
{
	auto opcd = opcode::INCSP;
	auto mnemonic = this->mnemonics.at(opcd);
	auto op = mnemonic + this->get_type_str(dtype::INT);

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}", mnemonic, num_of_bytes), "#" + std::to_string(num_of_bytes));
}

void Emitter::check_arrays(Symbol& arr1, Symbol& arr2)
{
	if (arr1.entry != entry::ARR or arr2.entry != entry::ARR)
	{
		throw CompilerException("Unknown exception, one of variables is not array", lineno);
	}
	auto& dims1 = arr1.args;
	auto& dims2 = arr2.args;

	if(dims1.size() != dims2.size())
	{
		throw CompilerException(interpolate("Array dimensions mismatch: {0} != {1}.", dims1.size(), dims2.size()), lineno);
	}

	for (auto i = 0; i < dims1.size(); ++i)
	{
		auto dim1 = dims1[i];
		auto dim2 = dims2[i];
		int val1 = std::abs(dim1.stop_ind - dim1.start_ind);
		int val2 = std::abs(dim2.stop_ind - dim2.start_ind);

		if(val2 != val1)
		{
			throw CompilerException(interpolate("Array dimensions mismatch on {0} axis: {1} != {2}", i, val1, val2), lineno);
		}
	}
}

std::optional<int> Emitter::make_call(int proc_or_fun, bool result_required)
{
	auto args = this->params;
	auto proc_or_fun_sym = this->symtab_ptr->get(proc_or_fun);

	const auto& entry_descriptor = [](entry& e){
		switch(e)
		{
            case entry::VAR: return "Scalar variable";
            case entry::ARR: return "Array variable";
            default: return "";
        }
	};

	if(proc_or_fun_sym.entry != entry::PROC and proc_or_fun_sym.entry != entry::FUNC)
	{
		throw CompilerException(interpolate("Syntax error. {0} is not callable", proc_or_fun_sym.name), lineno);
	}

	auto& signature = proc_or_fun_sym.args;

	if (signature.size() != args.size())
	{
		throw CompilerException(interpolate("Syntax error. Callable {0} expects {1} parameter, got {2}", proc_or_fun_sym.name, signature.size(), args.size()), lineno);
	}

	if(result_required and proc_or_fun_sym.entry == entry::PROC)
	{
		throw CompilerException(interpolate("Syntax error. {0} is not a function", proc_or_fun_sym.name), lineno);
	}

	for (auto i = 0; i < signature.size(); ++i)
	{
		auto sig_symbol = signature[i];
		auto arg_symbol = this->symtab_ptr->get(args[i]);
		
		if (arg_symbol.entry != entry::NUM and arg_symbol.entry != sig_symbol.entry)
		{
			throw CompilerException(interpolate("Syntax error. Unknown conversion from {0} to {1}", entry_descriptor(arg_symbol.entry), entry_descriptor(sig_symbol.entry)), lineno);
		}

		if (arg_symbol.entry == entry::ARR and sig_symbol.entry == entry::ARR)
		{
			check_arrays(arg_symbol, sig_symbol);
		}

		if(sig_symbol.is_reference)
		{
			if(arg_symbol.entry == entry::NUM)
			{
				throw CompilerException(("Syntax error. Constant value is not convertible to the reference type"), lineno);
			}

			else if(arg_symbol.dtype != sig_symbol.dtype)
			{
				throw CompilerException(("Syntax error. Reference variable is not convertible to the reference of the other type"), lineno);
			}

			this->push(arg_symbol);
		}
		else 
		{
			if(arg_symbol.dtype != sig_symbol.dtype and arg_symbol.entry == entry::ARR)
			{
				throw CompilerException(("Syntax error. Unknown conversion of array to specified internal data type"), lineno);
			}

			auto array_ref = arg_symbol.entry == entry::ARR;

			auto temp = this->symtab_ptr->get(this->symtab_ptr->insert_temp(arg_symbol.dtype, array_ref));

			this->assign(temp, arg_symbol);
			this->push(temp);
		}
	}

	int result = SymTable::NONE;

	if (proc_or_fun_sym.entry == entry::FUNC)
	{
		auto res_sym = this->symtab_ptr->get(this->symtab_ptr->insert_temp(proc_or_fun_sym.dtype));
		result = res_sym.symtab_id;
		this->push(res_sym);
	}

	auto opcd = opcode::CALL;
	auto mnemonic = this->mnemonics.at(opcd);
	auto op = mnemonic + this->get_type_str(dtype::INT);
	auto sz = static_cast<int>(varsize::REF) * args.size();

	if (proc_or_fun_sym.entry == entry::FUNC)
	{
		sz += static_cast<int>(varsize::REF);
	}

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}", mnemonic, proc_or_fun_sym.name), proc_or_fun_sym.addr_to_str(false, true));
	this->incsp(sz);

	if (result == SymTable::NONE)
	{
		return std::nullopt;
	}
	else 
	{
		return std::make_optional(result);
	}
}

int Emitter::left_eval_and_or(int lval_label, int rval, bool or_op)
{
	auto eval_lval_only = this->symtab_ptr->get(lval_label);

	if (eval_lval_only.entry != entry::LABEL)
	{
		throw CompilerException("Unknown error. Expected entry::LABEL", lineno);
	}

	auto symbol = this->symtab_ptr->get(rval);

	if (symbol.entry == entry::ARR)
	{
		throw CompilerException("The truth value of Array type is ambigious.", lineno);
	}

	if (symbol.entry != entry::VAR and symbol.entry != entry::NUM)
	{
		throw CompilerException("Unknown error.", lineno);
	}

	auto op_str = or_op ? "or" : "and";
	auto op_tok = or_op ? token::OR : token::AND;

	auto temp_lval = this->symtab_ptr->get(this->symtab_ptr->insert_temp(dtype::INT));
	auto temp_rval = this->symtab_ptr->get(this->symtab_ptr->insert_temp(dtype::INT));

	auto op_symbol = this->symtab_ptr->get(this->symtab_ptr->insert_by_token("!=", token::RELOP));
	auto eval_op_symbol = this->symtab_ptr->get(this->symtab_ptr->insert_by_token(op_str, op_tok));
	auto zero = this->symtab_ptr->get(this->symtab_ptr->insert_constant("0", dtype::INT));
	auto one = this->symtab_ptr->get(this->symtab_ptr->insert_constant("1", dtype::INT));

	auto relop_result = this->symtab_ptr->get(this->relop(op_symbol, zero, symbol));
	auto rest_of_code = this->symtab_ptr->get(this->symtab_ptr->insert_label(interpolate("{0}result", op_str)));

	auto r_enabler = or_op ? zero : one;
	auto r_disabler = or_op ? one : zero;

	this->assign(temp_lval, r_enabler);
	this->assign(temp_rval, relop_result);
	this->jump(rest_of_code);
	this->label(eval_lval_only);
	this->assign(temp_rval, zero);
	this->assign(temp_lval, r_disabler);
	this->label(rest_of_code);

	return this->andorop(eval_op_symbol, temp_lval, temp_rval);
}

int Emitter::and_then(int lval_label, int rval)
{
	return this->left_eval_and_or(lval_label, rval, false);
}

int Emitter::or_else(int lval_label, int rval)
{
	return this->left_eval_and_or(lval_label, rval, true);
}

int Emitter::andorop(Symbol& and_or, Symbol& first, Symbol& second)
{
	if (and_or.entry != entry::OP)
	{
		throw CompilerException("Unknown error.", lineno);
	}

	auto opcd = static_cast<opcode>(and_or.address);
	auto mnemonic = this->mnemonics.at(opcd);

	if (opcd != opcode::OR and opcd != opcode::AND)
	{
		throw CompilerException(interpolate("Unknown error. Invalid operator, expected or/and, got: {0}", mnemonic), lineno);
	}

	auto type = dtype::INT;
	auto temp = this->symtab_ptr->get(this->symtab_ptr->insert_temp(type));

	if (type != first.dtype)
	{
		first = this->symtab_ptr->get(this->cast(first, type));
	}

	if (type != second.dtype)
	{
		second = this->symtab_ptr->get(this->cast(second, type));
	}

	auto op = mnemonic + this->get_type_str(type);

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}, {2}, {3}", op, first.name, second.name, temp.name), 
						 first.addr_to_str(true), second.addr_to_str(true), temp.addr_to_str(true));

	return temp.symtab_id;
}

void Emitter::write(int symbol_id)
{
	auto symbol = this->symtab_ptr->get(symbol_id);
	auto& mnemonic = this->mnemonics.at(opcode::WRT);
	
	switch (symbol.entry)
	{		
        case entry::VAR:
        case entry::NUM:
		{
			std::string op = mnemonic + this->get_type_str(symbol.dtype);
			this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}", op, symbol.name), symbol.addr_to_str(true));
			break;
		}
		
        case entry::ARR:
			throw CompilerException("No matching overload of write procedure for Array type", lineno);
		case entry::RNG:
			throw CompilerException(interpolate("Syntax error, expected constant or variable identifier, got a range object: {0}..{1}", symbol.start_ind, symbol.stop_ind), lineno);
        case entry::OP:
			throw CompilerException(interpolate("Syntax error, expected constant or variable identifier, got an operator: {0}", symbol.name), lineno);
        default:
        	throw CompilerException("Unknown error", lineno);
    }
}

void Emitter::read(int symbol_id)
{
	auto symbol = this->symtab_ptr->get(symbol_id);
	auto& mnemonic = this->mnemonics.at(opcode::RD);
	switch (symbol.entry)
	{		
        case entry::VAR:
		{
			std::string op = mnemonic + this->get_type_str(symbol.dtype);
			this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}", op, symbol.name), symbol.addr_to_str(true));
			break;
		}
        case entry::NUM:
			throw CompilerException(interpolate("Syntax error, expected variable identifier, got an constant: {0}, of {1}", symbol.name, symbol.type_to_str()), lineno);
        case entry::ARR:
			throw CompilerException("No matching overload of read procedure for Array type", lineno);
		case entry::RNG:
			throw CompilerException(interpolate("Syntax error, expected variable identifier, got a range object: {0}..{1}", symbol.start_ind, symbol.stop_ind), lineno);
        case entry::OP:
			throw CompilerException(interpolate("Syntax error, expected variable identifier, got an operator: {0}", symbol.name), lineno);
        default:
        	throw CompilerException("Unknown error", lineno);
    }
}

void Emitter::assign(Symbol& lval_sym, Symbol& rval_sym)
{
	if (lval_sym.entry != entry::VAR)
	{
		throw CompilerException("Syntax error. Variable expected as a left side of the assignment.", lineno);
	}

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

void Emitter::assign(int lval, int rval)
{
	auto lval_sym = this->symtab_ptr->get(lval);
	auto rval_sym = this->symtab_ptr->get(rval);

	return this->assign(lval_sym, rval_sym);
}

void Emitter::jump(int where)
{
	auto label = this->symtab_ptr->get(where);
	return this->jump(label);
}

void Emitter::jump(Symbol& label)
{
	if (label.entry != entry::LABEL and label.entry != entry::FUNC and label.entry != entry::PROC)
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
	auto temp = this->symtab_ptr->get(this->symtab_ptr->insert_temp(type));
	auto mnemonic = this->mnemonics.at(op_code);
	auto op = mnemonic + this->get_type_str(op_type);

	auto true_label = this->symtab_ptr->get(this->symtab_ptr->insert_label(mnemonic + "true"));
	auto false_label = this->symtab_ptr->get(this->symtab_ptr->insert_label(mnemonic + "false"));
	
	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}, {2}, {3}", mnemonic, first.name, second.name, true_label.name),
						 first.addr_to_str(true), second.addr_to_str(true), true_label.addr_to_str());

	
	this->assign(temp.symtab_id, this->symtab_ptr->insert_constant("0", type));
	this->jump(false_label.symtab_id);
	this->label(true_label.symtab_id);
	this->assign(temp.symtab_id, this->symtab_ptr->insert_constant("1", type));
	this->label(false_label.symtab_id);

	return temp.symtab_id;
}

int Emitter::binop(Symbol& op_symbol, Symbol& first, Symbol& second)
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
	
	auto temp = this->symtab_ptr->get(this->symtab_ptr->insert_temp(type));
	auto op = this->mnemonics.at(op_code) + this->get_type_str(type);

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}, {2}, {3}", op, first.name, second.name, temp.name), 
						 first.addr_to_str(true), second.addr_to_str(true), temp.addr_to_str(true));

	return temp.symtab_id;
}

int Emitter::binary_op(int op_id, int operand1, int operand2)
{	
	auto first = this->symtab_ptr->get(operand1);
	auto second = this->symtab_ptr->get(operand2);
	auto op_symbol= this->symtab_ptr->get(op_id); 

	if(op_symbol.entry != entry::OP)
	{
		throw CompilerException("Unknown error.", lineno);
	}

	auto op_code = opcode(op_symbol.address);

	if (first.entry == entry::ARR or second.entry == entry::ARR)
	{
		throw CompilerException(interpolate("No matching overload of {0} for Array type.", this->mnemonics.at(op_code)), lineno);
	}

	if ((first.entry != entry::VAR and first.entry != entry::NUM) or (second.entry != entry::VAR and second.entry != entry::NUM))
	{
		throw CompilerException("Unknown error.", lineno);
	}

	switch (op_code) 
	{
		case opcode::ADD: 
		case opcode::SUB:
		case opcode::MUL:
		case opcode::DIV:
		case opcode::MOD:
			return binop(op_symbol, first, second);
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
	
	auto return_id = this->symtab_ptr->insert_temp(to);
	auto temp = this->symtab_ptr->get(return_id);
	auto& mnemonic = this->mnemonics.at(opcd);
	auto op = mnemonic + this->get_type_str(to);

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}, {2}", op, symbol.name, temp.name), symbol.addr_to_str(true), temp.addr_to_str(true));

	return return_id;
}

int Emitter::cast(int id, dtype& to)
{
	auto sym = this->symtab_ptr->get(id);
	return this->cast(sym, to);
}

std::ostream& Emitter::get_stream()
{
	return this->symtab_ptr->scope() == scope::GLOBAL ? this->output : this->mem;
}

int Emitter::begin_left_eval_or_and(opcode opcd, int lval)
{
	if (opcd != opcode::EQ and opcd != opcode::NE)
	{
		throw CompilerException("Unknown error.", lineno);
	}

	auto& symbol = this->symtab_ptr->get(lval);

	if (symbol.entry == entry::ARR)
	{
		throw CompilerException("The truth value of Array type is ambigious.", lineno);
	}

	if (symbol.entry != entry::VAR and symbol.entry != entry::NUM)
	{
		throw CompilerException("Unknown error.", lineno);
	}

	auto eval_left_only = this->symtab_ptr->get(this->symtab_ptr->insert_label("leftonly"));

	auto& mnemonic = this->mnemonics.at(opcd);
	auto op = mnemonic + this->get_type_str(dtype::INT);

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}, 0, {2}", mnemonic, symbol.name, eval_left_only.name), symbol.addr_to_str(true), std::string("#0"), eval_left_only.addr_to_str());

	return eval_left_only.symtab_id;
}

int Emitter::begin_or_else(int lval)
{
	return this->begin_left_eval_or_and(opcode::NE, lval);
}

int Emitter::begin_and_then(int lval)
{
	return this->begin_left_eval_or_and(opcode::EQ, lval);
}