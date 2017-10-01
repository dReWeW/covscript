#include "headers/codegen.hpp"
namespace cs {
	statement_base *method_expression::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		return new statement_expression(dynamic_cast<token_expr *>(raw.front().front())->get_tree(), raw.front().back());
	}

	statement_base *method_import::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		const std::string& package_name = dynamic_cast<token_id *>(dynamic_cast<token_expr *>(raw.front().at(1))->get_tree().root().data())->get_id();
		std::string package_path = std::string(import_path) + "/" + package_name;
		if(std::ifstream(package_path + ".csp")) {
			runtime_t rt = covscript(package_path + ".csp");
			if (rt->package_name.empty())
				throw syntax_error("Target file is not a package.");
			runtime->storage.add_var(rt->package_name, var::make_protect<extension_t>(std::make_shared<extension_holder>(rt->storage.get_global())));
		}
		else if(std::ifstream(package_path + ".cse"))
			runtime->storage.add_var(package_name, var::make_protect<extension_t>(std::make_shared<extension_holder>(package_path + ".cse")));
		else
			throw fatal_error("No such file or directory.");
		return nullptr;
	}

	statement_base *method_package::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		if (!runtime->package_name.empty())
			throw syntax_error("Redefinition of package");
		runtime->package_name = dynamic_cast<token_id *>(dynamic_cast<token_expr *>(raw.front().at(1))->get_tree().root().data())->get_id();
		runtime->storage.add_var_global(runtime->package_name, var::make_protect<extension_t>(std::make_shared<extension_holder>(runtime->storage.get_global())));
		return nullptr;
	}

	statement_base *method_var::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		cov::tree<token_base *> &tree = dynamic_cast<token_expr *>(raw.front().at(1))->get_tree();
		define_var_profile dvp;
		parse_define_var(tree, dvp);
		return new statement_var(dvp, raw.front().back());
	}

	statement_base *method_constant::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		cov::tree<token_base *> &tree = dynamic_cast<token_expr *>(raw.front().at(2))->get_tree();
		define_var_profile dvp;
		parse_define_var(tree, dvp);
		if (dvp.expr.root().data()->get_type() != token_types::value)
			throw syntax_error("Constant variable must have an constant value.");
		runtime->storage.add_var(dvp.id, static_cast<token_value *>(dvp.expr.root().data())->get_value());
		return nullptr;
	}

	statement_base *method_end::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		return new statement_end;
	}

	statement_base *method_block::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		std::deque<statement_base *> body;
		kill_action({raw.begin() + 1, raw.end()}, body);
		return new statement_block(body, raw.front().back());
	}

	statement_base *method_namespace::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		std::deque<statement_base *> body;
		kill_action({raw.begin() + 1, raw.end()}, body);
		for (auto &ptr:body)
			if (ptr->get_type() != statement_types::var_ && ptr->get_type() != statement_types::function_ && ptr->get_type() != statement_types::namespace_ && ptr->get_type() != statement_types::struct_)
				throw syntax_error("Wrong grammar for namespace definition.");
		return new statement_namespace(dynamic_cast<token_expr *>(raw.front().at(1))->get_tree().root().data(), body, raw.front().back());
	}

	statement_base *method_if::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		bool have_else = false;
		std::deque<statement_base *> body;
		kill_action({raw.begin() + 1, raw.end()}, body);
		for (auto &ptr:body) {
			if (ptr->get_type() == statement_types::else_) {
				if (!have_else)
					have_else = true;
				else
					throw syntax_error("Multi Else Grammar.");
			}
		}
		if (have_else) {
			std::deque<statement_base *> body_true;
			std::deque<statement_base *> body_false;
			bool now_place = true;
			for (auto &ptr:body) {
				if (ptr->get_type() == statement_types::else_) {
					now_place = false;
					continue;
				}
				if (now_place)
					body_true.push_back(ptr);
				else
					body_false.push_back(ptr);
			}
			return new statement_ifelse(dynamic_cast<token_expr *>(raw.front().at(1))->get_tree(), body_true, body_false, raw.front().back());
		}
		else
			return new statement_if(dynamic_cast<token_expr *>(raw.front().at(1))->get_tree(), body, raw.front().back());
	}

	statement_base *method_else::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		return new statement_else;
	}

	statement_base *method_switch::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		std::deque<statement_base *> body;
		kill_action({raw.begin() + 1, raw.end()}, body);
		statement_block *dptr = nullptr;
		std::unordered_map<var, statement_block *> cases;
		for (auto &it:body) {
			if (it == nullptr)
				throw internal_error("Access Null Pointer.");
			else if (it->get_type() == statement_types::case_) {
				statement_case *scptr = dynamic_cast<statement_case *>(it);
				if (cases.count(scptr->get_tag()) > 0)
					throw syntax_error("Redefinition of case.");
				cases.emplace(scptr->get_tag(), scptr->get_block());
			}
			else if (it->get_type() == statement_types::default_) {
				statement_default *sdptr = dynamic_cast<statement_default *>(it);
				if (dptr != nullptr)
					throw syntax_error("Redefinition of default case.");
				dptr = sdptr->get_block();
			}
			else
				throw syntax_error("Wrong format of switch statement.");
		}
		return new statement_switch(dynamic_cast<token_expr *>(raw.front().at(1))->get_tree(), cases, dptr, raw.front().back());
	}

	statement_base *method_case::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		cov::tree<token_base *> &tree = dynamic_cast<token_expr *>(raw.front().at(1))->get_tree();
		if (tree.root().data()->get_type() != token_types::value)
			throw syntax_error("Case Tag must be a constant value.");
		std::deque<statement_base *> body;
		kill_action({raw.begin() + 1, raw.end()}, body);
		return new statement_case(dynamic_cast<token_value *>(tree.root().data())->get_value(), body, raw.front().back());
	}

	statement_base *method_default::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		std::deque<statement_base *> body;
		kill_action({raw.begin() + 1, raw.end()}, body);
		return new statement_default(body, raw.front().back());
	}

	statement_base *method_while::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		std::deque<statement_base *> body;
		kill_action({raw.begin() + 1, raw.end()}, body);
		return new statement_while(dynamic_cast<token_expr *>(raw.front().at(1))->get_tree(), body, raw.front().back());
	}

	statement_base *method_until::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		return new statement_until(dynamic_cast<token_expr *>(raw.front().at(1)), raw.front().back());
	}

	statement_base *method_loop::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		std::deque<statement_base *> body;
		kill_action({raw.begin() + 1, raw.end()}, body);
		if (!body.empty() && body.back()->get_type() == statement_types::until_) {
			token_expr *expr = dynamic_cast<statement_until *>(body.back())->get_expr();
			body.pop_back();
			return new statement_loop(expr, body, raw.front().back());
		}
		else
			return new statement_loop(nullptr, body, raw.front().back());
	}

	void method_for_step::preprocess(const std::deque<std::deque<token_base *>> &raw)
	{
		cov::tree<token_base *> &tree = dynamic_cast<token_expr *>(raw.front().at(1))->get_tree();
		define_var_profile dvp;
		parse_define_var(tree, dvp);
		runtime->storage.add_record(dvp.id);
	}

	statement_base *method_for_step::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		std::deque<statement_base *> body;
		kill_action({raw.begin() + 1, raw.end()}, body);
		return new statement_for(dynamic_cast<token_expr *>(raw.front().at(1))->get_tree(), dynamic_cast<token_expr *>(raw.front().at(3))->get_tree(), dynamic_cast<token_expr *>(raw.front().at(5))->get_tree(), body, raw.front().back());
	}

	void method_for::preprocess(const std::deque<std::deque<token_base *>> &raw)
	{
		cov::tree<token_base *> &tree = dynamic_cast<token_expr *>(raw.front().at(1))->get_tree();
		define_var_profile dvp;
		parse_define_var(tree, dvp);
		runtime->storage.add_record(dvp.id);
	}

	statement_base *method_for::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		std::deque<statement_base *> body;
		kill_action({raw.begin() + 1, raw.end()}, body);
		cov::tree<token_base *> tree_step;
		tree_step.emplace_root_left(tree_step.root(), new token_value(number(1)));
		return new statement_for(dynamic_cast<token_expr *>(raw.front().at(1))->get_tree(), dynamic_cast<token_expr *>(raw.front().at(3))->get_tree(), tree_step, body, raw.front().back());
	}

	void method_foreach::preprocess(const std::deque<std::deque<token_base *>> &raw)
	{
		cov::tree<token_base *> &t = dynamic_cast<token_expr *>(raw.front().at(1))->get_tree();
		if (t.root().data() == nullptr)
			throw internal_error("Null pointer accessed.");
		if (t.root().data()->get_type() != token_types::id)
			throw syntax_error("Wrong grammar(foreach)");
		runtime->storage.add_record(dynamic_cast<token_id *>(t.root().data())->get_id());
	}

	statement_base *method_foreach::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		cov::tree<token_base *> &t = dynamic_cast<token_expr *>(raw.front().at(1))->get_tree();
		if (t.root().data() == nullptr)
			throw internal_error("Null pointer accessed.");
		if (t.root().data()->get_type() != token_types::id)
			throw syntax_error("Wrong grammar(foreach)");
		const std::string &it = dynamic_cast<token_id *>(t.root().data())->get_id();
		std::deque<statement_base *> body;
		kill_action({raw.begin() + 1, raw.end()}, body);
		return new statement_foreach(it, dynamic_cast<token_expr *>(raw.front().at(3))->get_tree(), body, raw.front().back());
	}

	statement_base *method_break::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		return new statement_break(raw.front().back());
	}

	statement_base *method_continue::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		return new statement_continue(raw.front().back());
	}

	statement_base *method_function::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		cov::tree<token_base *> &t = dynamic_cast<token_expr *>(raw.front().at(1))->get_tree();
		if (t.root().data() == nullptr)
			throw internal_error("Null pointer accessed.");
		if (t.root().data()->get_type() != token_types::signal || dynamic_cast<token_signal *>(t.root().data())->get_signal() != signal_types::fcall_)
			throw syntax_error("Wrong grammar for function definition.");
		if (t.root().left().data() == nullptr)
			throw internal_error("Null pointer accessed.");
		if (t.root().left().data()->get_type() != token_types::id)
			throw syntax_error("Wrong grammar for function definition.");
		if (t.root().right().data() == nullptr)
			throw internal_error("Null pointer accessed.");
		if (t.root().right().data()->get_type() != token_types::arglist)
			throw syntax_error("Wrong grammar for function definition.");
		std::string name = dynamic_cast<token_id *>(t.root().left().data())->get_id();
		std::deque<std::string> args;
		for (auto &it:dynamic_cast<token_arglist *>(t.root().right().data())->get_arglist()) {
			if (it.root().data() == nullptr)
				throw internal_error("Null pointer accessed.");
			if (it.root().data()->get_type() != token_types::id)
				throw syntax_error("Wrong grammar for function definition.");
			const std::string &str = dynamic_cast<token_id *>(it.root().data())->get_id();
			for (auto &it:args)
				if (it == str)
					throw syntax_error("Redefinition of function argument.");
			args.push_back(str);
		}
		std::deque<statement_base *> body;
		kill_action({raw.begin() + 1, raw.end()}, body);
		return new statement_function(name, args, body, raw.front().back());
	}

	statement_base *method_return::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		return new statement_return(dynamic_cast<token_expr *>(raw.front().at(1))->get_tree(), raw.front().back());
	}

	statement_base *method_return_no_value::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		cov::tree<token_base *> tree;
		tree.emplace_root_left(tree.root(), new token_value(number(0)));
		return new statement_return(tree, raw.front().back());
	}

	statement_base *method_struct::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		cov::tree<token_base *> &t = dynamic_cast<token_expr *>(raw.front().at(1))->get_tree();
		if (t.root().data() == nullptr)
			throw internal_error("Null pointer accessed.");
		if (t.root().data()->get_type() != token_types::id)
			throw syntax_error("Wrong grammar for struct definition.");
		std::string name = dynamic_cast<token_id *>(t.root().data())->get_id();
		std::deque<statement_base *> body;
		kill_action({raw.begin() + 1, raw.end()}, body);
		for (auto &ptr:body) {
			switch (ptr->get_type()) {
			default:
				throw syntax_error("Wrong grammar for struct definition.");
			case statement_types::var_:
				break;
			case statement_types::function_:
				static_cast<statement_function *>(ptr)->set_mem_fn();
				break;
			}
		}
		return new statement_struct(name, body, raw.front().back());
	}

	statement_base *method_try::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		std::deque<statement_base *> body;
		kill_action({raw.begin() + 1, raw.end()}, body);
		std::string name;
		std::deque<statement_base *> tbody, cbody;
		bool founded = false;
		for (auto &ptr:body) {
			if (ptr->get_type() == statement_types::catch_) {
				name = static_cast<statement_catch *>(ptr)->get_name();
				founded = true;
				continue;
			}
			if (founded)
				cbody.push_back(ptr);
			else
				tbody.push_back(ptr);
		}
		if (!founded)
			throw syntax_error("Wrong grammar for try statement.");
		return new statement_try(name, tbody, cbody, raw.front().back());
	}

	statement_base *method_catch::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		cov::tree<token_base *> &t = dynamic_cast<token_expr *>(raw.front().at(1))->get_tree();
		if (t.root().data() == nullptr)
			throw internal_error("Null pointer accessed.");
		if (t.root().data()->get_type() != token_types::id)
			throw syntax_error("Wrong grammar for catch statement.");
		return new statement_catch(dynamic_cast<token_id *>(t.root().data())->get_id(), raw.front().back());
	}

	statement_base *method_throw::translate(const std::deque<std::deque<token_base *>> &raw)
	{
		return new statement_throw(dynamic_cast<token_expr *>(raw.front().at(1))->get_tree(), raw.front().back());
	}


}