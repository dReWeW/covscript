#pragma once
#include "../arglist.hpp"
static cov_basic::extension hash_map_ext;
namespace hash_map_cbs_ext {
	using namespace cov_basic;
	cov::any clear(array& args)
	{
		arglist::check<hash_map>(args);
		args.at(0).val<hash_map>(true).clear();
		return number(0);
	}
	cov::any insert(array& args)
	{
		if(args.size()!=3)
			throw syntax_error("Wrong size of arguments.");
		if(args.at(0).type()!=typeid(hash_map))
			throw syntax_error("Wrong type of arguments.(Request Hash Map)");
		hash_map& map=args.at(0).val<hash_map>(true);
		if(map.count(args.at(1))>0)
			map.at(args.at(1)).swap(copy(args.at(2)),true);
		else
			map.emplace(copy(args.at(1)),copy(args.at(2)));
		return number(0);
	}
	cov::any erase(array& args)
	{
		if(args.size()!=2)
			throw syntax_error("Wrong size of arguments.");
		if(args.at(0).type()!=typeid(hash_map))
			throw syntax_error("Wrong type of arguments.(Request Hash Map)");
		args.at(0).val<hash_map>(true).erase(args.at(1));
		return number(0);
	}
	cov::any exist(array& args)
	{
		if(args.size()!=2)
			throw syntax_error("Wrong size of arguments.");
		if(args.at(0).type()!=typeid(hash_map))
			throw syntax_error("Wrong type of arguments.(Request Hash Map)");
		return bool(args.at(0).val<hash_map>(true).count(args.at(1))>0);
	}
	cov::any at(array& args)
	{
		if(args.size()!=2)
			throw syntax_error("Wrong size of arguments.");
		if(args.at(0).type()!=typeid(hash_map))
			throw syntax_error("Wrong type of arguments.(Request Hash Map)");
		return args.at(0).val<hash_map>(true).at(args.at(1));
	}
	void init()
	{
		hash_map_ext.add_var("clear",cov::any::make_protect<native_interface>(clear,true));
		hash_map_ext.add_var("insert",cov::any::make_protect<native_interface>(insert,true));
		hash_map_ext.add_var("erase",cov::any::make_protect<native_interface>(erase,true));
		hash_map_ext.add_var("exist",cov::any::make_protect<native_interface>(exist,true));
		hash_map_ext.add_var("at",cov::any::make_protect<native_interface>(at,true));
	}
}