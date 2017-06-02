#pragma once
/*
* Covariant Mozart Utility Library: Any
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Copyright (C) 2017 Michael Lee(李登淳)
* Email: China-LDC@outlook.com
* Github: https://github.com/mikecovlee
* Website: http://ldc.atd3.cn
*
* Version: 17.5.2
*/
#include "./base.hpp"
#include "./memory.hpp"
#include <functional>
#include <iostream>

namespace std {
	template<typename T> std::string to_string(const T&)
	{
		throw cov::error("E000D");
	}
	template<> std::string to_string<std::string>(const std::string& str)
	{
		return str;
	}
	template<> std::string to_string<bool>(const bool& v)
	{
		if(v)
			return "true";
		else
			return "false";
	}
}
#define COV_ANY_POOL_SIZE 96
namespace cov {
	template<typename _Tp> class compare_helper {
		template<typename T,typename X=bool>struct matcher;
		template<typename T> static constexpr bool match(T*)
		{
			return false;
		}
		template<typename T> static constexpr bool match(matcher < T, decltype(std::declval<T>()==std::declval<T>()) > *)
		{
			return true;
		}
	public:
		static constexpr bool value = match < _Tp > (nullptr);
	};
	template<typename,bool> struct compare_if;
	template<typename T>struct compare_if<T,true> {
		static bool compare(const T& a,const T& b)
		{
			return a==b;
		}
	};
	template<typename T>struct compare_if<T,false> {
		static bool compare(const T& a,const T& b)
		{
			return &a==&b;
		}
	};
	template<typename T>bool compare(const T& a,const T& b)
	{
		return compare_if<T,compare_helper<T>::value>::compare(a,b);
	}
	template<typename _Tp> class hash_helper {
		template<typename T,decltype(&std::hash<T>::operator()) X>struct matcher;
		template<typename T> static constexpr bool match(T*)
		{
			return false;
		}
		template<typename T> static constexpr bool match(matcher<T,&std::hash<T>::operator()>*)
		{
			return true;
		}
	public:
		static constexpr bool value = match < _Tp > (nullptr);
	};
	template<typename,bool> struct hash_if;
	template<typename T>struct hash_if<T,true> {
		static std::size_t hash(const T& val)
		{
			static std::hash<T> gen;
			return gen(val);
		}
	};
	template<typename T>struct hash_if<T,false> {
		static std::size_t hash(const T& val)
		{
			throw cov::error("E000F");
		}
	};
	template<typename T>std::size_t hash(const T& val)
	{
		return hash_if<T,hash_helper<T>::value>::hash(val);
	}
	template<typename T>void detach(T& val)
	{
		// Do something if you want when data is copying.
	}
	class any final {
		class baseHolder {
		public:
			baseHolder() = default;
			virtual ~ baseHolder() = default;
			virtual const std::type_info& type() const = 0;
			virtual baseHolder* duplicate() = 0;
			virtual bool compare(const baseHolder *) const = 0;
			virtual std::string to_string() const = 0;
			virtual std::size_t hash() const = 0;
			virtual void detach() = 0;
			virtual void kill() = 0;
		};
		template < typename T > class holder:public baseHolder {
		protected:
			T mDat;
		public:
			static cov::allocator<holder<T>,COV_ANY_POOL_SIZE> allocator;
			holder() = default;
			template<typename...ArgsT>holder(ArgsT&&...args):mDat(std::forward<ArgsT>(args)...) {}
			virtual ~ holder() = default;
			virtual const std::type_info& type() const override
			{
				return typeid(T);
			}
			virtual baseHolder* duplicate() override
			{
				return allocator.alloc(mDat);
			}
			virtual bool compare(const baseHolder* obj) const override
			{
				if (obj->type()==this->type()) {
					const holder<T>* ptr=dynamic_cast<const holder<T>*>(obj);
					return ptr!=nullptr?cov::compare(mDat,ptr->data()):false;
				}
				return false;
			}
			virtual std::string to_string() const override
			{
				return std::to_string(mDat);
			}
			virtual std::size_t hash() const override
			{
				return cov::hash<T>(mDat);
			}
			virtual void detach() override
			{
				cov::detach(mDat);
			}
			virtual void kill() override
			{
				allocator.free(this);
			}
			T& data()
			{
				return mDat;
			}
			const T& data() const
			{
				return mDat;
			}
			void data(const T& dat)
			{
				mDat = dat;
			}
		};
		using size_t=unsigned long;
		struct proxy {
			bool constant=false;
			size_t refcount=1;
			baseHolder* data=nullptr;
			proxy()=default;
			proxy(size_t rc,baseHolder* d):refcount(rc),data(d) {}
			proxy(bool c,size_t rc,baseHolder* d):constant(c),refcount(rc),data(d) {}
			~proxy()
			{
				if(data!=nullptr)
					data->kill();
			}
		};
		static cov::allocator<proxy,COV_ANY_POOL_SIZE> allocator;
		proxy* mDat=nullptr;
		proxy* duplicate() const noexcept
		{
			if(mDat!=nullptr) {
				++mDat->refcount;
			}
			return mDat;
		}
		void recycle() noexcept
		{
			if(mDat!=nullptr) {
				--mDat->refcount;
				if(mDat->refcount==0) {
					allocator.free(mDat);
					mDat=nullptr;
				}
			}
		}
		any(proxy* dat):mDat(dat) {}
	public:
		void swap(any& obj,bool raw=false)
		{
			if(this->mDat!=nullptr&&obj.mDat!=nullptr&&raw) {
				if(this->mDat->constant||obj.mDat->constant)
					throw cov::error("E000G");
				baseHolder* tmp=this->mDat->data;
				this->mDat->data=obj.mDat->data;
				obj.mDat->data=tmp;
			}
			else {
				proxy* tmp=this->mDat;
				this->mDat=obj.mDat;
				obj.mDat=tmp;
			}
		}
		void swap(any&& obj,bool raw=false) noexcept
		{
			if(this->mDat!=nullptr&&obj.mDat!=nullptr&&raw) {
				if(this->mDat->constant||obj.mDat->constant)
					std::terminate();
				baseHolder* tmp=this->mDat->data;
				this->mDat->data=obj.mDat->data;
				obj.mDat->data=tmp;
			}
			else {
				proxy* tmp=this->mDat;
				this->mDat=obj.mDat;
				obj.mDat=tmp;
			}
		}
		void clone()
		{
			if(mDat!=nullptr) {
				if(mDat->constant)
					throw cov::error("E000H");
				proxy* dat=allocator.alloc(1,mDat->data->duplicate());
				recycle();
				mDat=dat;
			}
		}
		bool usable() const noexcept
		{
			return mDat!=nullptr;
		}
		template<typename T,typename...ArgsT>static any make(ArgsT&&...args)
		{
			return any(allocator.alloc(1,holder<T>::allocator.alloc(std::forward<ArgsT>(args)...)));
		}
		template<typename T,typename...ArgsT>static any make_constant(ArgsT&&...args)
		{
			return any(allocator.alloc(true,1,holder<T>::allocator.alloc(std::forward<ArgsT>(args)...)));
		}
		any()=default;
		template<typename T> any(const T & dat):mDat(allocator.alloc(1,holder<T>::allocator.alloc(dat))) {}
		any(const any & v):mDat(v.duplicate()) {}
		any(any&& v) noexcept
		{
			swap(std::forward<any>(v));
		}
		~any()
		{
			recycle();
		}
		const std::type_info& type() const
		{
			return this->mDat!=nullptr?this->mDat->data->type():typeid(void);
		}
		std::string to_string() const
		{
			if(this->mDat==nullptr)
				return "Null";
			return this->mDat->data->to_string();
		}
		std::size_t hash() const
		{
			if(this->mDat==nullptr)
				return cov::hash<void*>(nullptr);
			return this->mDat->data->hash();
		}
		void detach()
		{
			if(this->mDat!=nullptr) {
				if(this->mDat->constant)
					throw cov::error("E000I");
				this->mDat->data->detach();
			}
		}
		bool is_same(const any& obj) const
		{
			return this->mDat==obj.mDat;
		}
		bool is_constant() const
		{
			return this->mDat!=nullptr&&this->mDat->constant;
		}
		any& operator=(const any& var)
		{
			if(&var!=this) {
				recycle();
				mDat=var.duplicate();
			}
			return *this;
		}
		any& operator=(any&& var) noexcept
		{
			if(&var!=this)
				swap(std::forward<any>(var));
			return *this;
		}
		bool operator==(const any& var) const
		{
			return usable()?this->mDat->data->compare(var.mDat->data):!var.usable();
		}
		bool operator!=(const any& var)const
		{
			return usable()?!this->mDat->data->compare(var.mDat->data):var.usable();
		}
		template<typename T> T& val(bool raw=false)
		{
			if(typeid(T)!=this->type())
				throw cov::error("E0006");
			if(this->mDat==nullptr)
				throw cov::error("E0005");
			if(!raw)
				clone();
			return dynamic_cast<holder<T>*>(this->mDat->data)->data();
		}
		template<typename T> const T& val(bool raw=false) const
		{
			if(typeid(T)!=this->type())
				throw cov::error("E0006");
			if(this->mDat==nullptr)
				throw cov::error("E0005");
			return dynamic_cast<const holder<T>*>(this->mDat->data)->data();
		}
		template<typename T> const T& const_val() const
		{
			if(typeid(T)!=this->type())
				throw cov::error("E0006");
			if(this->mDat==nullptr)
				throw cov::error("E0005");
			return dynamic_cast<const holder<T>*>(this->mDat->data)->data();
		}
		template<typename T> operator const T&() const
		{
			return this->const_val<T>();
		}
		void assign(const any& obj,bool raw=false)
		{
			if(&obj!=this&&obj.mDat!=mDat) {
				if(mDat!=nullptr&&obj.mDat!=nullptr&&raw) {
					if(this->mDat->constant||obj.mDat->constant)
						throw cov::error("E000G");
					mDat->data->kill();
					mDat->data=obj.mDat->data->duplicate();
				}
				else {
					recycle();
					if(obj.mDat!=nullptr)
						mDat=allocator.alloc(1,obj.mDat->data->duplicate());
					else
						mDat=nullptr;
				}
			}
		}
		template<typename T> void assign(const T& dat,bool raw=false)
		{
			if(mDat!=nullptr&&raw) {
				if(this->mDat->constant)
					throw cov::error("E000G");
				mDat->data->kill();
				mDat->data=holder<T>::allocator.alloc(dat);
			}
			else {
				recycle();
				mDat=allocator.alloc(1,holder<T>::allocator.alloc(dat));
			}
		}
		template<typename T> any & operator=(const T& dat)
		{
			assign(dat);
			return *this;
		}
	};
	template<typename T> cov::allocator<any::holder<T>,COV_ANY_POOL_SIZE> any::holder<T>::allocator;
	cov::allocator<any::proxy,COV_ANY_POOL_SIZE> any::allocator;
	template<int N> class any::holder<char[N]>:public any::holder<std::string> {
	public:
		using holder<std::string>::holder;
	};
	template<> class any::holder<std::type_info>:public any::holder<std::type_index> {
	public:
		using holder<std::type_index>::holder;
	};
}

std::ostream& operator<<(std::ostream& out,const cov::any& val)
{
	out<<val.to_string();
	return out;
}

namespace std {
	template<> struct hash<cov::any> {
		std::size_t operator()(const cov::any& val) const
		{
			return val.hash();
		}
	};
}
