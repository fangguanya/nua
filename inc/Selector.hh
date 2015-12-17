#pragma once
#include <lua.hpp>
#include <string>
#include <vector>
#include <tuple>
#include "Ref.hh"
#include "stack.hh"
#include "Func.hh"
#include "Registry.hh"

namespace nua
{
    class Selector
    {
    private:
        lua_State* l_;
        std::string name_;
        Registry* registry_;
        RefPtr key_;

        std::vector<RefPtr> functor_arguments_;
        bool functor_active_ = false;

        void evaluate_retrieve(int num_results)
        {
            lua_pushglobaltable(l_);
            key_->push();
            lua_gettable(l_, -2);
            lua_remove(l_, lua_absindex(l_, -2));
            evaluate_function_call(num_results);
        }
        
        void evaluate_function_call(int num_results)
        {

            if(!functor_active_)
                return;
            functor_active_ = false;
    
            int handler_index = ErrorHandler::set_error_handler(l_);
            int func_index = handler_index - 1;
            lua_pushvalue(l_, func_index);
            lua_copy(l_, handler_index, func_index);
            lua_replace(l_, handler_index);
            
            for(const RefPtr& arg : functor_arguments_)
                arg->push();

            int status = lua_pcall(l_, functor_arguments_.size(), num_results, handler_index - 1);
            lua_remove(l_, handler_index - 1);
            
            if(status != LUA_OK)
                ErrorHandler::handle(l_, status);
        }

        template <typename FuncT>
        void evaluate_store(FuncT&& push_func)
        {
            stack::StackGuard sg{l_};
            lua_pushglobaltable(l_);
            key_->push();
            push_func();
            lua_settable(l_, -3);
            lua_pop(l_, 1);
        }

        template <typename... Args, size_t... Is>
        static std::tuple<Args...> get_n(lua_State* l, std::index_sequence<Is...>)
        {
            std::tuple<Args...> ret;
            (void)std::initializer_list<int>{(std::get<Is>(ret) = stack::get<Args>(l, int(Is - sizeof...(Is))), 0)...}; 
            lua_pop(l, int(sizeof...(Is)));
            return ret;  
        }

    public:
        Selector(lua_State* l, Registry* registry, const std::string& name)
            : l_{l}, name_{name}, registry_{registry}, key_{make_ref(l, name)}
        {}

        Selector(const Selector& other) = default;

        template <typename... Args>
        Selector operator()(Args&&... args)
        {
            Selector copy{*this};
            copy.functor_arguments_ = make_refs(l_, std::forward<Args>(args)...);
            copy.functor_active_ = true;
            return copy;
        }

        template <typename T>
        typename std::enable_if<is_primitive<T>::value, void>::type
        operator=(T v)
        {
            evaluate_store([this, v]
            {
                stack::push(l_, v);
            });
        }

        template <typename Ret, typename... Args>
        void operator=(std::function<Ret(Args...)> func)
        {
            evaluate_store([this, func]
            {
                auto func_ptr = registry_->registerFunction(func);
                lua_pushlightuserdata(l_, (void *)func_ptr);
                lua_pushcclosure(l_, &BaseFunc::dispatcher, 1);
            });
        }        

        void get()
        {
            stack::StackGuard sg{l_};
            evaluate_retrieve(0);
        }

        template <typename T>
        T get()
        {
            stack::StackGuard sg{l_};
            evaluate_retrieve(1);
            return stack::pop<T>(l_);
        }

        template <typename T1, typename T2, typename... Args>
        std::tuple<T1, T2, Args...> get()
        {
            stack::StackGuard sg{l_};
            evaluate_retrieve(sizeof...(Args) + 2);
            return get_n<T1, T2, Args...>(l_, std::make_index_sequence<sizeof...(Args) + 2>());
        }
    };
}