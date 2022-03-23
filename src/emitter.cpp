#include "emitter.hpp"
#include <cstdlib>

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

int Emitter::get_item(int array_id)
{
	auto array = symtab_ptr->get(array_id);
	if (array.m_entry != entry::ARR)
	{
		throw CompilerException(interpolate("Syntax error. {0} is not subscriptable.", array.m_entry), lineno);
	}

	auto dim_ids = this->params;

	if(dim_ids.empty())
	{
		return array_id;
	}

	std::vector<Symbol> dims;

	std::transform(dim_ids.cbegin(), dim_ids.cend(), std::inserter(dims, dims.begin()), [this](auto symbol_id)
	{
		return symtab_ptr->get(symbol_id);
	});

	return this->reduce(array, dims);
} 

int Emitter::reduce(Symbol& array, std::vector<Symbol>& dims)
{
	auto array_type_spec = array.args[0];

	if (array_type_spec.args.size() < dims.size())
	{
		throw CompilerException(interpolate("Syntax error. Out of array dimensions access: {0} < {1}", array_type_spec.args.size(), dims.size()), lineno);
	}

	auto is_result_arr = array_type_spec.args.size() > dims.size();

	auto temp_id = symtab_ptr->insert_temp(array_type_spec.m_dtype, true);
	auto temp = symtab_ptr->get(temp_id);

	if (is_result_arr)
	{
		std::vector<Symbol> new_dims;
		new_dims.insert(new_dims.cbegin(), array_type_spec.args.crbegin(), array_type_spec.args.crend() - dims.size());
		auto temp_type = symtab_ptr->get(symtab_ptr->insert_array_type(new_dims, array_type_spec.m_dtype) - static_cast<int>(dtype::OBJECT));
		temp.m_entry = entry::ARR;
		
		temp.args = {temp_type};
		temp.start_ind = temp_type.start_ind;
		temp.stop_ind = temp_type.stop_ind;
		symtab_ptr->update(temp);
	}

	auto offset = symtab_ptr->get(symtab_ptr->insert_temp(dtype::INT));

	std::vector<Symbol> dim_specs(array_type_spec.args.begin(), array_type_spec.args.begin() + dims.size());
	auto multiplier_sym = symtab_ptr->get(symtab_ptr->insert_temp(dtype::INT));
	auto temp_2 = symtab_ptr->get(symtab_ptr->insert_temp(dtype::INT));
	auto temp_3 = symtab_ptr->get(symtab_ptr->insert_temp(dtype::INT));

	this->assign(multiplier_sym, symtab_ptr->get(symtab_ptr->insert_constant("1", dtype::INT)));
	this->assign(temp_2, symtab_ptr->get(symtab_ptr->insert_constant("0", dtype::INT)));
	this->assign(offset, symtab_ptr->get(symtab_ptr->insert_constant("0", dtype::INT)));

	if(is_result_arr)
	{
		for(auto dim : temp.args[0].args)
		{
			auto coeff = symtab_ptr->get(symtab_ptr->insert_constant(std::to_string(std::abs(dim.stop_ind - dim.start_ind + 1)), dtype::INT));
			this->binop(opcode::MUL, multiplier_sym, coeff, &multiplier_sym);
		}
	}

	for (auto spec_it = dim_specs.rbegin(), dim_it = dims.rbegin(); spec_it < dim_specs.crend(); ++spec_it, ++dim_it)
	{
		//Compile-time known accessor
		if(dim_it->m_entry == entry::NUM)
		{
			this->check_bounds(*dim_it, *spec_it);
		}

		auto coeff = symtab_ptr->get(symtab_ptr->insert_constant(std::to_string(spec_it->start_ind), dtype::INT));

		this->binop(opcode::SUB, *dim_it, coeff, &temp_3);
		this->binop(opcode::MUL, multiplier_sym, temp_3, &temp_2);
		this->binop(opcode::ADD, temp_2, offset, &offset);

		if(spec_it + 1 < dim_specs.crend())
		{
			auto coeff = symtab_ptr->get(symtab_ptr->insert_constant(std::to_string(std::abs(spec_it->stop_ind - spec_it->start_ind + 1)), dtype::INT));
			this->binop(opcode::MUL, multiplier_sym, coeff, &multiplier_sym);
		}
	}

	varsize sz;

	switch (array.args[0].m_dtype) 
	{
		case dtype::INT: sz = varsize::INT; break;
		case dtype::REAL: sz = varsize::REAL; break;
		default: sz = varsize::NONE; break;
	};

	auto size_constant = symtab_ptr->get(symtab_ptr->insert_constant(std::to_string(static_cast<int>(sz)), dtype::INT));

	this->binop(opcode::MUL, size_constant, offset, &offset);
	this->shift_pointer(array, offset, &temp);

	return temp_id;
}

void Emitter::move_pointer(Symbol& pointer, Symbol& dest)
{

	if (dest.m_entry == entry::VAR and dest.m_dtype != dtype::INT)
	{
		throw CompilerException(interpolate("Variable is expected to be integer, got {0}", dest.m_dtype), lineno);
	}

	auto mnemonic = this->mnemonics.at(opcode::MOV);
	auto op = mnemonic + this->get_type_str(dtype::INT);

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t&{1}, &{2}", mnemonic, pointer.name, dest.name), 
						 pointer.addr_to_str(false), dest.addr_to_str(false));
}

int Emitter::shift_pointer(Symbol& pointer, Symbol& offset, Symbol* result)
{
	if (offset.m_entry != entry::NUM and offset.m_entry != entry::VAR)
	{
		throw CompilerException(interpolate("Unknown error. Expected constant or variable, got: {0}", offset.m_entry), lineno);
	}

	if (offset.m_dtype != dtype::INT)
	{
		throw CompilerException(interpolate("Unknown error. Expected integer offset, got: {0}", offset.m_dtype), lineno);
	}

	auto temp = result == nullptr ? symtab_ptr->get(symtab_ptr->insert_temp(pointer.m_dtype, true)) : *result;
	auto mnemonic = this->mnemonics.at(opcode::ADD);
	auto op = mnemonic + this->get_type_str(dtype::INT);

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t&{1}, {2}, &{3}", mnemonic, pointer.name, offset.name, temp.name), 
						 pointer.addr_to_str(false), offset.addr_to_str(true), temp.addr_to_str(false));

	return temp.symtab_id;
}

void Emitter::check_bounds(Symbol& constant_dim_accessor, Symbol& axis)
{
	if(constant_dim_accessor.m_entry != entry::NUM or axis.m_entry != entry::RNG)
	{
		throw CompilerException(interpolate("Unknown error. Expected {0} entry and {1} entry, got {2}, {3}", entry::NUM, entry::RNG, constant_dim_accessor.m_entry, axis.m_entry), lineno);
	}

	if(constant_dim_accessor.m_dtype != dtype::INT)
	{
		throw CompilerException(interpolate("Syntax error. Expected integer constant got: {0}", constant_dim_accessor.m_dtype),lineno);
	}

	int index = std::atoi(constant_dim_accessor.name.c_str());
	if (index < axis.start_ind)
	{
		throw CompilerException(interpolate("{0} is smaller than the lower bound of axis dimensions {1}..{2}", index, axis.start_ind, axis.stop_ind), lineno);
	}

	if(index > axis.stop_ind)
	{
		throw CompilerException(interpolate("{0} is greater than the upper bound of axis dimensions {1}..{2}", index, axis.start_ind, axis.stop_ind), lineno);
	}
}

void Emitter::store_param_on_stack(int symbol_id)
{
	this->params_stack.top().push_back(symbol_id);
}

int Emitter::variable_or_call(int symbol_id, bool is_lvalue)
{
	auto symbol = symtab_ptr->get(symbol_id);
	
	if(symbol.m_entry == entry::VAR)
	{
		return symbol_id;
	}

	if(symbol.m_entry == entry::ARR)
	{
		return this->get_item(symbol_id);
	}

	if(symbol.m_entry == entry::FUNC and is_lvalue)
	{
		return  this->variable_or_call(symtab_ptr->lookup(interpolate("${0}_result", symbol.name)), is_lvalue);
	}
	else if(symbol.m_entry == entry::FUNC)
	{	
		auto result = this->make_call(symbol_id, true);
		return result.value_or(SymTable::NONE);
	}

	throw CompilerException("Syntax error. Illegal expression.", lineno);
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
       	default: break;
    }

	return out;
}

int Emitter::negate(Symbol& symbol)
{
	if(symbol.m_entry != entry::VAR and symbol.m_entry != entry::NUM)
	{
		throw CompilerException("Syntax error. Variable or numeric constant expected as a operand.", lineno);
	}

	return this->binop(opcode::SUB, symtab_ptr->get(symtab_ptr->insert_label("0")), symbol);
} 	

int Emitter::boolean_negate(Symbol& symbol)
{
	if(symbol.m_entry != entry::VAR and symbol.m_entry != entry::NUM)
	{
		throw CompilerException("Syntax error. Variable or numeric constant expected as a operand.", lineno);
	}

	auto type = dtype::INT;
	if (symbol.m_dtype != dtype::INT)
	{
		symbol = symtab_ptr->get(this->cast(symbol, type));
	}

	auto mnemonic = this->mnemonics.at(opcode::NOT);
	auto op = mnemonic + this->get_type_str(type);

	auto temp = symtab_ptr->get(symtab_ptr->insert_temp(type));
	
	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}, {2}", mnemonic, symbol.name, temp.name), symbol.addr_to_str(true), temp.addr_to_str(true));

	return temp.symtab_id;
}

int Emitter::unary_op(int op_id, int operand_id)
{
	auto symbol = symtab_ptr->get(operand_id);
	
	if(symbol.m_entry != entry::VAR and symbol.m_entry != entry::NUM)
	{
		throw CompilerException("Syntax error. Variable or numeric constant expected as a operand.", lineno);
	}

	auto opcd = static_cast<opcode>(op_id);

	switch (opcd) 
	{
		case opcode::ADD:
			return operand_id;
		case opcode::SUB:
			return this->negate(symbol);
		case opcode::NOT:
			return this->boolean_negate(symbol);
		default: throw CompilerException(interpolate("Invalid operation: {0} is not unary operator", opcd), lineno);
	}
}

void Emitter::start_program(int label_id)
{
	this->label(label_id);
}

void Emitter::call_program(int symbol_id)
{
	auto& symbol = symtab_ptr->get(symbol_id);

	if(symbol.m_scope != scope::UNBOUND)
	{
		throw CompilerException(interpolate("Syntax error. Redefinition of \"{0}\" program.", symbol.name), lineno);
	}

	symbol.m_scope = scope::GLOBAL;
	symbol.m_entry = entry::LABEL;

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

void Emitter::end_current_subprogram(int id)
{
	auto stack_size = std::abs(symtab_ptr->get_last_addr());

	std::cout << *(symtab_ptr);

	this->leave_subprogram();
	symtab_ptr->return_to_global_scope();
	this->label(id);
	this->enter(stack_size);
	this->commit_subprogram();

	symtab_ptr->restore_checkpoint();
}

void Emitter::end_program()
{
	if (symtab_ptr->get_scope() != scope::GLOBAL)
	{
		throw CompilerException("Cannot emit program exit if SymTable object is not in scope::GLOBAL", lineno);
	}
	
	this->emit_to_stream("\t\t", this->mnemonics.at(opcode::EXIT), ";\texit.");
	std::cout << *symtab_ptr << std::endl;
}

void Emitter::label(int label_id)
{
	auto symbol = symtab_ptr->get(label_id);
	return this->label(symbol);
}

void Emitter::jump_if(Symbol& expression, Symbol& test, Symbol& where, opcode opcd)
{
	if(expression.m_entry != entry::VAR and expression.m_entry != entry::NUM)
	{
		throw CompilerException(interpolate("Syntax error. Boolean value is ambigious for {0}", expression.m_entry), lineno);
	}

	if(test.m_entry != entry::VAR and test.m_entry != entry::NUM)
	{
		throw CompilerException(interpolate("Syntax error. Boolean value is ambigious for {0}", test.m_entry), lineno);
	}

	if(where.m_entry != entry::LABEL)
	{
		throw CompilerException(interpolate("Unknown error. Target entry is expected to be a LABEL, got {0}", where.m_entry), lineno);
	}

	auto mnemonic = this->mnemonics.at(opcd);
	auto op = mnemonic + this->get_type_str(expression.m_dtype);

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}, {2}, {3}", mnemonic, expression.name, test.name, where.name), 
		expression.addr_to_str(true), test.addr_to_str(true), where.addr_to_str(true));
}

int Emitter::end_if()
{
	auto result = symtab_ptr->insert_label("endif");
	this->jump(result);
	return result;
}

int Emitter::if_statement(int expression_id)
{
	auto expression = symtab_ptr->get(expression_id);
	auto else_label = symtab_ptr->get(symtab_ptr->insert_label("else"));
	auto zero = symtab_ptr->get(symtab_ptr->insert_constant("0", expression.m_dtype));
	this->jump_if(expression, zero, else_label);

	return else_label.symtab_id;
}

int Emitter::begin_while()
{
	auto while_label = symtab_ptr->get(symtab_ptr->insert_label("while"));
	this->label(while_label);
	return while_label.symtab_id;
}

int Emitter::while_statement(int expression_id)
{
	auto expression = symtab_ptr->get(expression_id);
	
	auto else_label = symtab_ptr->get(symtab_ptr->insert_label("endwhile"));
	auto zero = symtab_ptr->get(symtab_ptr->insert_constant("0", expression.m_dtype));

	this->jump_if(expression, zero, else_label);

	return else_label.symtab_id;
}

std::tuple<int, int> Emitter::classic_for_statement(int variable_id, int init_value_id, int dec_or_inc, int control_value)
{
	auto variable = symtab_ptr->get(variable_id);
	auto init_value = symtab_ptr->get(init_value_id);
	auto test = symtab_ptr->get(control_value);

	auto for_label = symtab_ptr->get(symtab_ptr->insert_label("for"));
	auto else_label = symtab_ptr->get(symtab_ptr->insert_label("endfor"));

	auto opcd = static_cast<opcode>(dec_or_inc);

	if(opcd != opcode::ADD and opcd != opcode::SUB)
	{
		throw CompilerException(interpolate("Unknown error. Expected SUB or ADD operator, got {0}", opcd), lineno);
	}

	if (variable.m_dtype != dtype::INT)
	{
		throw CompilerException(interpolate("Syntax error. Variable should be of integer type, not: {0}", variable.m_dtype), lineno);
	}

	if (test.m_dtype != dtype::INT)
	{
		throw CompilerException(interpolate("Syntax error. Control value should be of integer type, not: {0}", test.m_dtype), lineno);
	}

	if (init_value.m_dtype != dtype::INT)
	{
		throw CompilerException(interpolate("Syntax error. Initial value should be of integer type, not: {0}", init_value.m_dtype), lineno);
	}

	auto rel_opcd = opcd == opcode::ADD ? opcode::GT : opcode::LT;

	this->assign(variable, init_value);
	this->label(for_label);
	this->jump_if(variable, test, else_label, rel_opcd);

	return {for_label.symtab_id, else_label.symtab_id};
}

void Emitter::classic_end_iteration(int variable_id, int dec_or_inc, int for_label_id)
{
	auto opcd = static_cast<opcode>(dec_or_inc);
	auto variable = symtab_ptr->get(variable_id);
	auto for_label = symtab_ptr->get(for_label_id);

	if(opcd != opcode::ADD and opcd != opcode::SUB)
	{
		throw CompilerException(interpolate("Unknown error. Expected SUB or ADD operator, got {0}", opcd), lineno);
	}

	if (variable.m_dtype != dtype::INT)
	{
		throw CompilerException(interpolate("Syntax error. Variable should be of integer type, not: {0}", variable.m_dtype), lineno);
	}
	
	auto one = symtab_ptr->get(symtab_ptr->insert_constant("1", dtype::INT));

	this->binop(opcd, variable, one, &variable);
	this->jump(for_label);
}

int Emitter::repeat()
{
	auto repeat_label = symtab_ptr->get(symtab_ptr->insert_label("repeat"));
	this->label(repeat_label);
	return repeat_label.symtab_id;
}

void Emitter::until(int repeat_label_id, int expression_id)
{
	auto repeat_label = symtab_ptr->get(repeat_label_id);
	auto expression = symtab_ptr->get(expression_id);
	auto one = symtab_ptr->get(symtab_ptr->insert_constant("1", expression.m_dtype));

	this->jump_if(expression, one, repeat_label);
}

void Emitter::label(Symbol& symbol)
{
	if (symbol.m_entry == entry::LABEL or symbol.m_entry == entry::PROC or symbol.m_entry == entry::FUNC)
	{
		return this->emit_to_stream(interpolate("{0}:", symbol.name), "", "");
	}

	throw CompilerException(interpolate("Unknown error [label]. Expected LABEL, PROC or FUNC got: {0}", symbol.m_entry), lineno);
}

void Emitter::push(Symbol& symbol)
{
	if (symbol.m_entry != entry::VAR and symbol.m_entry != entry::ARR)
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
	if (arr1.m_entry != entry::ARR or arr2.m_entry != entry::ARR)
	{
		throw CompilerException("Unknown exception, one of variables is not array", lineno);
	}

	auto& dims1 = arr1.args[0].args;
	auto& dims2 = arr2.args[0].args;

	if(dims1.size() != dims2.size())
	{
		throw CompilerException(interpolate("Array dimensions mismatch: {0} != {1}.", dims1.size(), dims2.size()), lineno);
	}

	for (auto i = 0ull; i < dims1.size(); ++i)
	{
		auto dim1 = dims1[i];
		auto dim2 = dims2[i];
		int val1 = std::abs(dim1.stop_ind - dim1.start_ind + 1);
		int val2 = std::abs(dim2.stop_ind - dim2.start_ind + 1);

		if(val2 != val1)
		{
			throw CompilerException(interpolate("Array dimensions mismatch on {0} axis: {1} != {2}", i, val1, val2), lineno);
		}
	}
}

std::optional<int> Emitter::make_call(int proc_or_fun, bool result_required)
{
	auto args = this->params;
	auto proc_or_fun_sym = symtab_ptr->get(proc_or_fun);

	const auto& entry_descriptor = [](entry& e){
		switch(e)
		{
            case entry::VAR: return std::string("Scalar variable");
            case entry::ARR: return std::string("Array variable");
            default: return stringify(e);
        }
	};

	if(proc_or_fun_sym.m_entry != entry::PROC and proc_or_fun_sym.m_entry != entry::FUNC)
	{
		throw CompilerException(interpolate("Syntax error. {0} is not callable", proc_or_fun_sym.name), lineno);
	}

	auto& signature = proc_or_fun_sym.args;

	if (signature.size() != args.size())
	{
		throw CompilerException(interpolate("Syntax error. Callable {0} expects {1} parameter, got {2}", proc_or_fun_sym.name, signature.size(), args.size()), lineno);
	}

	if(result_required and proc_or_fun_sym.m_entry == entry::PROC)
	{
		throw CompilerException(interpolate("Syntax error. {0} is not a function", proc_or_fun_sym.name), lineno);
	}

	for (int i = signature.size() -1; i >= 0; --i)
	{
		auto sig_symbol = signature[i];
		auto arg_symbol = symtab_ptr->get(args[i]);
		
		if (arg_symbol.m_entry != entry::NUM and arg_symbol.m_entry != sig_symbol.m_entry)
		{
			throw CompilerException(interpolate("Syntax error. Unknown conversion from {0} to {1}", entry_descriptor(arg_symbol.m_entry), entry_descriptor(sig_symbol.m_entry)), lineno);
		}

		if (arg_symbol.m_entry == entry::ARR and sig_symbol.m_entry == entry::ARR)
		{
			check_arrays(arg_symbol, sig_symbol);
		}

		if(sig_symbol.is_reference)
		{
			if(arg_symbol.m_entry == entry::NUM)
			{
				throw CompilerException(("Syntax error. Constant value is not convertible to the reference type"), lineno);
			}

			else if(arg_symbol.m_dtype != sig_symbol.m_dtype)
			{
				throw CompilerException(("Syntax error. Reference variable is not convertible to the reference of the other type"), lineno);
			}

			this->push(arg_symbol);
		}
		else 
		{
			if(arg_symbol.m_dtype != sig_symbol.m_dtype and arg_symbol.m_entry == entry::ARR)
			{
				throw CompilerException(("Syntax error. Unknown conversion of array to specified internal data type"), lineno);
			}

			auto array_ref = arg_symbol.m_entry == entry::ARR;

			auto temp = symtab_ptr->get(symtab_ptr->insert_temp(arg_symbol.m_dtype, array_ref));

			this->assign(temp, arg_symbol);
			this->push(temp);
		}
	}

	int result = SymTable::NONE;

	if (proc_or_fun_sym.m_entry == entry::FUNC)
	{
		auto res_sym = symtab_ptr->get(symtab_ptr->insert_temp(proc_or_fun_sym.m_dtype));
		result = res_sym.symtab_id;
		this->push(res_sym);
	}

	auto opcd = opcode::CALL;
	auto mnemonic = this->mnemonics.at(opcd);
	auto op = mnemonic + this->get_type_str(dtype::INT);
	auto sz = static_cast<int>(varsize::REF) * args.size();

	if (proc_or_fun_sym.m_entry == entry::FUNC)
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
	auto eval_lval_only = symtab_ptr->get(lval_label);

	if (eval_lval_only.m_entry != entry::LABEL)
	{
		throw CompilerException(interpolate("Unknown error [left_eval_and_or]. Expected entry::LABEL, got {0}", eval_lval_only.m_entry), lineno);
	}

	auto symbol = symtab_ptr->get(rval);

	if (symbol.m_entry == entry::ARR)
	{
		throw CompilerException("The truth value of Array type is ambigious.", lineno);
	}

	if (symbol.m_entry != entry::VAR and symbol.m_entry != entry::NUM)
	{
		throw CompilerException(interpolate("Unknown error [left_eval_and_or]. Expected VAR or NUM got: {0}", symbol.m_entry), lineno);
	}

	auto temp_lval = symtab_ptr->get(symtab_ptr->insert_temp(dtype::INT));
	auto temp_rval = symtab_ptr->get(symtab_ptr->insert_temp(dtype::INT));

	auto op_symbol = opcode::NE;
	auto eval_op_symbol = or_op? opcode::OR : opcode::AND;
	auto zero = symtab_ptr->get(symtab_ptr->insert_constant("0", dtype::INT));
	auto one = symtab_ptr->get(symtab_ptr->insert_constant("1", dtype::INT));

	auto relop_result = symtab_ptr->get(this->relop(op_symbol, zero, symbol));
	auto rest_of_code = symtab_ptr->get(symtab_ptr->insert_label(interpolate("{0}result", this->mnemonics.at(eval_op_symbol))));

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

int Emitter::andorop(opcode opcd, Symbol& first, Symbol& second, Symbol* result)
{
	auto mnemonic = this->mnemonics.at(opcd);

	if (opcd != opcode::OR and opcd != opcode::AND)
	{
		throw CompilerException(interpolate("Unknown error. Invalid operator, expected or/and, got: {0}", mnemonic), lineno);
	}

	auto type = dtype::INT;
	auto temp = result == nullptr ? symtab_ptr->get(symtab_ptr->insert_temp(type)) : *result;

	if (type != first.m_dtype)
	{
		first = symtab_ptr->get(this->cast(first, type));
	}

	if (type != second.m_dtype)
	{
		second = symtab_ptr->get(this->cast(second, type));
	}

	auto op = mnemonic + this->get_type_str(type);

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}, {2}, {3}", mnemonic, first.name, second.name, temp.name), 
						 first.addr_to_str(true), second.addr_to_str(true), temp.addr_to_str(true));

	return temp.symtab_id;
}

void Emitter::write(int symbol_id)
{
	auto symbol = symtab_ptr->get(symbol_id);
	auto& mnemonic = this->mnemonics.at(opcode::WRT);
	
	switch (symbol.m_entry)
	{		
        case entry::VAR:
        case entry::NUM:
		{
			std::string op = mnemonic + this->get_type_str(symbol.m_dtype);
			this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}", mnemonic, symbol.name), symbol.addr_to_str(true));
			break;
		}
		
        case entry::ARR:
			throw CompilerException("No matching overload of write procedure for Array type", lineno);
		case entry::RNG:
			throw CompilerException(interpolate("Syntax error, expected constant or variable identifier, got a range object: {0}..{1}", symbol.start_ind, symbol.stop_ind), lineno);
        default:
        	throw CompilerException("Unknown error", lineno);
    }
}

void Emitter::read(int symbol_id)
{
	auto symbol = symtab_ptr->get(symbol_id);
	auto& mnemonic = this->mnemonics.at(opcode::RD);
	switch (symbol.m_entry)
	{		
        case entry::VAR:
		{
			std::string op = mnemonic + this->get_type_str(symbol.m_dtype);
			this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}", mnemonic, symbol.name), symbol.addr_to_str(true));
			break;
		}
        case entry::NUM:
			throw CompilerException(interpolate("Syntax error, expected variable identifier, got an constant: {0}, of {1}", symbol.name, symbol.type_to_str()), lineno);
        case entry::ARR:
			throw CompilerException("No matching overload of read procedure for Array type", lineno);
		case entry::RNG:
			throw CompilerException(interpolate("Syntax error, expected variable identifier, got a range object: {0}..{1}", symbol.start_ind, symbol.stop_ind), lineno);
        default:
        	throw CompilerException("Unknown error", lineno);
    }
}

void Emitter::assign(Symbol& lval_sym, Symbol& rval_sym)
{
	if (rval_sym.m_entry == entry::ARR and lval_sym.m_entry == entry::VAR)
	{
		return this->move_pointer(rval_sym, lval_sym);
	}

	if (lval_sym.m_entry != entry::VAR and lval_sym.m_entry != entry::ARR)
	{
		throw CompilerException("Syntax error. Variable or array expected as a left side of the assignment.", lineno);
	}
	
	if (rval_sym.m_entry != entry::VAR and rval_sym.m_entry != entry::ARR and rval_sym.m_entry != entry::NUM)
	{
		throw CompilerException("Syntax error. Variable, array or numeric constant expected as a right side of the assignment.", lineno);
	}

	if(rval_sym.m_entry == entry::ARR and lval_sym.m_entry == entry::ARR)
	{
		this->check_arrays(rval_sym, lval_sym);
		return this->move_pointer(rval_sym, lval_sym);
	}

	if (lval_sym.m_dtype != rval_sym.m_dtype)
	{
		rval_sym = symtab_ptr->get(this->cast(rval_sym, lval_sym.m_dtype));
	}

	auto mnemonic = this->mnemonics.at(opcode::MOV);
	auto op = mnemonic + this->get_type_str(lval_sym.m_dtype);

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}, {2}", mnemonic, rval_sym.name, lval_sym.name), rval_sym.addr_to_str(true), lval_sym.addr_to_str(true));
}

void Emitter::assign(int lval, int rval)
{
	auto lval_sym = symtab_ptr->get(lval);
	auto rval_sym = symtab_ptr->get(rval);

	return this->assign(lval_sym, rval_sym);
}

void Emitter::jump(int where)
{
	auto label = symtab_ptr->get(where);
	return this->jump(label);
}

void Emitter::jump(Symbol& label)
{
	if (label.m_entry != entry::LABEL and label.m_entry != entry::FUNC and label.m_entry != entry::PROC)
	{
		throw CompilerException(interpolate("Unknown error [jump]. Expected LABEL, PROC or FUNC, got: {0}", label.m_entry), lineno);
	}

	auto& mnemonic = this->mnemonics.at(opcode::JMP);
	auto op = mnemonic + ".i";

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}", mnemonic, label.name), label.addr_to_str());
}

int Emitter::relop(opcode op_code, Symbol& first, Symbol& second, Symbol* result)
{
	if(not ((
			first.m_entry == entry::NUM or first.m_entry == entry::VAR
		) and (
			second.m_entry == entry::NUM or second.m_entry == entry::VAR
		)
	)) throw CompilerException(interpolate("Unknown error [relop]. Expected (NUM|VAR, NUM|VAR), got: ({0}, {1})", first.m_entry, second.m_entry), lineno);

	auto op_type = dtype::INT;
	auto type = dtype::INT;
	auto temp = result == nullptr ? symtab_ptr->get(symtab_ptr->insert_temp(type)) : *result;
	auto mnemonic = this->mnemonics.at(op_code);
	auto op = mnemonic + this->get_type_str(op_type);

	auto true_label = symtab_ptr->get(symtab_ptr->insert_label(mnemonic + "true"));
	auto false_label = symtab_ptr->get(symtab_ptr->insert_label(mnemonic + "false"));
	
	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}, {2}, {3}", mnemonic, first.name, second.name, true_label.name),
						 first.addr_to_str(true), second.addr_to_str(true), true_label.addr_to_str());

	
	this->assign(temp.symtab_id, symtab_ptr->insert_constant("0", type));
	this->jump(false_label.symtab_id);
	this->label(true_label.symtab_id);
	this->assign(temp.symtab_id, symtab_ptr->insert_constant("1", type));
	this->label(false_label.symtab_id);

	return temp.symtab_id;
}

int Emitter::binop(opcode op_code, Symbol& first, Symbol& second, Symbol* result)
{
	if(not ((
			first.m_entry == entry::NUM or first.m_entry == entry::VAR
		) and (
			second.m_entry == entry::NUM or second.m_entry == entry::VAR
		)
	)) throw CompilerException(interpolate("Unknown error [binop]. Expected (NUM|VAR, NUM|VAR), got: ({0}, {1})", first.m_entry, second.m_entry), lineno);

	auto type = symtab_ptr->infer_type(first, second);
	
	if (type != first.m_dtype)
	{
		first = symtab_ptr->get(this->cast(first, type));
	}

	if (type != second.m_dtype)
	{
		second = symtab_ptr->get(this->cast(second, type));
	}
	
	auto temp = result == nullptr ? symtab_ptr->get(symtab_ptr->insert_temp(type)) : *result;
	auto& mnemonic = this->mnemonics.at(op_code);
	auto op = mnemonic + this->get_type_str(type);

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}, {2}, {3}", mnemonic, first.name, second.name, temp.name), 
						 first.addr_to_str(true), second.addr_to_str(true), temp.addr_to_str(true));

	return temp.symtab_id;
}

int Emitter::binary_op(int op_id, int operand1, int operand2)
{	
	auto first = symtab_ptr->get(operand1);
	auto second = symtab_ptr->get(operand2);

	auto op_code = opcode(op_id);

	if (first.m_entry == entry::ARR or second.m_entry == entry::ARR)
	{
		throw CompilerException(interpolate("No matching overload of {0} for Array type.", this->mnemonics.at(op_code)), lineno);
	}

	switch (op_code) 
	{
		case opcode::ADD: 
		case opcode::SUB:
		case opcode::MUL:
		case opcode::DIV:
		case opcode::MOD:
			return binop(op_code, first, second);
		case opcode::AND:
		case opcode::OR:
			return andorop(op_code, first, second);
		case opcode::NE:
		case opcode::LE:
		case opcode::LT:
		case opcode::GE:
		case opcode::GT:
		case opcode::EQ:
			return relop(op_code, first, second);
		default: throw CompilerException(interpolate("Invalid operation: {0} is not binary operator", op_code), lineno);
	}
}

int Emitter::cast(Symbol& symbol, dtype& to)
{
	if (symbol.m_entry == entry::ARR)
	{
		throw CompilerException("Cannot perform type cast for Array type", lineno);
	}

	if (symbol.m_entry != entry::VAR and symbol.m_entry != entry::NUM)
	{
		throw CompilerException(interpolate("Unknown error [cast]. Expected VAR or NUM got: {0}", symbol.m_entry), lineno);
	}

	if (symbol.m_dtype == to) return symbol.symtab_id;

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
        default:
          break;
    }

	if (opcd == opcode::NOP)
	{
		throw CompilerException(interpolate("Unknown error [cast]. {0}", opcd), lineno);
	}
	
	auto return_id = symtab_ptr->insert_temp(to);
	auto temp = symtab_ptr->get(return_id);
	auto& mnemonic = this->mnemonics.at(opcd);
	auto op = mnemonic + this->get_type_str(symbol.m_dtype);

	this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}, {2}", op, symbol.name, temp.name), symbol.addr_to_str(true), temp.addr_to_str(true));

	return return_id;
}

int Emitter::cast(int id, dtype& to)
{
	auto sym = symtab_ptr->get(id);
	return this->cast(sym, to);
}

std::ostream& Emitter::get_stream()
{
	return symtab_ptr->get_scope() == scope::GLOBAL ? this->output : this->mem;
}

int Emitter::begin_left_eval_or_and(opcode opcd, int lval)
{
	if (opcd != opcode::EQ and opcd != opcode::NE)
	{
		throw CompilerException(interpolate("Unknown error [begin_left_eval_only]. Expected EQ or NE got: {0}", opcd), lineno);
	}

	auto& symbol = symtab_ptr->get(lval);

	if (symbol.m_entry == entry::ARR)
	{
		throw CompilerException("The truth value of Array type is ambigious.", lineno);
	}

	if (symbol.m_entry != entry::VAR and symbol.m_entry != entry::NUM)
	{
		throw CompilerException(interpolate("Unknown error [begin_left_eval_only]. Expected VAR or NUM got: {0}", symbol.m_entry), lineno);
	}

	auto eval_left_only = symtab_ptr->get(symtab_ptr->insert_label("leftonly"));

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