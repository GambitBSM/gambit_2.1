// BOSS modifies this file in place.
#ifndef __CLASSES_HPP__
#define __CLASSES_HPP__

#include <iostream>


#include "abstract_U.hpp"
class U : public virtual Abstract_U
{
    public:
        U()
        {
            std::cout << "(Constructor of U)" << std::endl;
        }

        ~U()
        {
            std::cout << "(Destructor of U)" << std::endl;
        }

        void memberFunc()
        {
            std::cout << "This is memberFunc from class U" << std::endl;
        }

    public:
        Abstract_U* pointerCopy_GAMBIT()
        { 
            std::cout << "(pointerCopy_GAMBIT in class U)" << std::endl;
            return new U(*this); 
        }
        
        void pointerAssign_GAMBIT(Abstract_U* in)
        { 
            std::cout << "(pointerAssign_GAMBIT in class U)" << std::endl;
            *this = *dynamic_cast<U*>(in); 
        }

};


#include "abstract_T.hpp"
class T : public virtual Abstract_T, public U
{
    public:
        // Member variables
        int i;
        double d;

        // Constructor
        T() : i(1), d(3.14)
        {
            std::cout << "(Constructor of T)" << std::endl;
        }
        
        T(int i_in, double d_in) : i(i_in), d(d_in) 
        {
            std::cout << "(Constructor of T)" << std::endl;
        }

        // Destructor
        ~T()
        {
            std::cout << "(Destructor of T)" << std::endl;
        }

        // Class methods
        void printMe();

        //
        // Generated members:
        //

        Abstract_T* pointerCopy_GAMBIT()
        {
            std::cout << "(pointerCopy_GAMBIT in class T)" << std::endl;
            return new T(*this);
        }

        void pointerAssign_GAMBIT(Abstract_T *in)
        {
            std::cout << "(pointerAssign_GAMBIT in class T)" << std::endl;
            *this = *dynamic_cast<T*>(in);
        }        

        int& i_ref_GAMBIT()
        {
            return i;
        }

        double& d_ref_GAMBIT()
        {
            return d;
        }

};



#include "abstract_X.hpp"
class X : public virtual Abstract_X
{
    public:
        // Member variables
        T t;

        // Constructor
        X()
        {
            std::cout << "(Constructor of X)" << std::endl;
        }

        X(T t_in) : t(t_in)
        {
            std::cout << "(Constructor of X)" << std::endl;
        }

        // Destructor
        ~X()
        {
            std::cout << "(Destructor of X)" << std::endl;
        }


        // Class methods
        T getT();

        void setT(T t_in);

        void refTest(T& t_in, int& i_in)
        {
            T new_t;
            new_t.i = 123;
            new_t.d = 1.23;
            t_in = new_t;

            int new_i = 987;
            i_in = new_i;
        }

        int**& testFunc(T* t1, T t2, int**& ipp, double d)
        {
            **ipp += 1;
            return ipp;
        }


        //
        // Generated members:
        //

        Abstract_X* pointerCopy_GAMBIT()
        {
            return new X(*this);
        }

        void pointerAssign_GAMBIT(Abstract_X *in)
        {
            *this = *dynamic_cast<X*>(in);
        }             

        Abstract_T& t_ref_GAMBIT()
        {
            return t;
        }

        Abstract_T* getT_GAMBIT()
        {
            return new T(getT());
        }
    
        void setT_GAMBIT(Abstract_T& t_in)
        {
            setT( dynamic_cast<T&>(t_in) );
        }

        void refTest_GAMBIT(Abstract_T& t_in, int& i_in)
        {
            refTest( dynamic_cast<T&>(t_in), i_in );
        }

        int**& testFunc_GAMBIT(Abstract_T* t1, Abstract_T& t2, int**& ipp, double d)
        {
            return testFunc( dynamic_cast<T*>(t1), dynamic_cast<T&>(t2), ipp, d );
        }

};



#include "abstract_Container.hpp"
#include "abstract_Container_extra.hpp"
template <typename Type>
class Container : public virtual Abstract_Container<Type>
{
    public:
        // Member variables
        Type var;

        // Constructor
        Container() {}

        Container(Type in) : var(in) {}

        void printMsg()
        {
            std::cout << std::endl;
            std::cout << "A message from class 'Container'." << std::endl;
            std::cout << std::endl;
        }

        //
        // Generated members:
        //

        Abstract_Container<Type>* pointerCopy_GAMBIT()
        {
            return new Container(*this);
        }

        void pointerAssign_GAMBIT(Abstract_Container<Type> *in)
        {
            *this = *dynamic_cast<Container*>(in);
        }             

        Type& var_ref_GAMBIT()
        {
            return var;
        }

};


#endif  /* __CLASSES_HPP__ */
