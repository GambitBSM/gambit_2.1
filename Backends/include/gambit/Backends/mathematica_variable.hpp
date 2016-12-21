//   GAMBIT: Global and Modular BSM Inference Tool
//   *********************************************
///  \file
///
///  Class mathematica_variable, needed to overload
///  constructor and assignment operators to send
///  messages throught WSTP
///
///  ***********************************************
///
///  Authors
///  =======
///
///  (add name and date if you modify)
///
///  \author Tomas Gonzalo
///          (t.e.gonzalo@fys.uio.no)
///  \date 2016 Nov
///
///  ***********************************************

#include "gambit/Utils/util_types.hpp"
#include "gambit/Elements/ini_functions.hpp"
#include "gambit/cmake/cmake_variables.hpp"

#ifdef HAVE_MATHEMATICA
#include MATHEMATICA_WSTP_H

#ifndef __mathematica_variable_hpp__
#define __mathematica_variable_hpp__

namespace Gambit
{
  namespace Backends
  {
  

    // Overloaded functions to get data through WSTP
    inline int WSGetVariable(WSLINK WSlink, int* val) { return WSGetInteger(WSlink, val); }
    inline int WSGetVariable(WSLINK WSlink, float* val) { return WSGetReal32(WSlink, val); }
    inline int WSGetVariable(WSLINK WSlink, double* val) { return WSGetReal64(WSlink, val); } 
    inline int WSGetVariable(WSLINK WSlink, bool* val) 
    { 
      const char *val2;
      int ret = WSGetString(WSlink, &val2); 
      *val = (str(val2) == "True");
      return ret;
    }
    inline int WSGetVariable(WSLINK WSlink, char* val)
    { 
      const char *val2;
      int ret = WSGetString(WSlink, &val2);
      *val = val2[0];
      return ret;
    }
    inline int WSGetVariable(WSLINK WSlink, str* val) 
    { 
      const char *val2;
      int ret = WSGetString(WSlink, &val2);
      *val = str(val2);
      return ret;
    } 
 
    // Overloaded functions to put data through WSTP
    inline int WSPutVariable(WSLINK WSlink, int val) { return WSPutInteger32(WSlink, val); }
    inline int WSPutVariable(WSLINK WSlink, float val) { return WSPutReal32(WSlink, val); }
    inline int WSPutVariable(WSLINK WSlink, double val) { return WSPutReal64(WSlink, val); }
    inline int WSPutVariable(WSLINK WSlink, bool val) 
    { 
      if(val)
        return WSPutSymbol(WSlink, "True");
      else
        return WSPutSymbol(WSlink, "False");
    }
    inline int WSPutVariable(WSLINK WSlink, char val) { return WSPutString(WSlink, str(&val).c_str()); }
    inline int WSPutVariable(WSLINK WSlink, str val) { return WSPutString(WSlink, val.c_str()); }
 
    // Class mathematica_variable
    template <typename TYPE>
    class mathematica_variable
    {

      private:
        TYPE _var;
        WSLINK _WSlink;
        str _symbol;

      public:

        // Constructor
        mathematica_variable(WSLINK WSlink, str symbol) :  _WSlink(WSlink), _symbol(symbol)
        {

          try
          {
            /* If TYPE is a numeric type, send N first */
            if(boost::is_same<TYPE, int>::value or
               boost::is_same<TYPE, float>::value or
               boost::is_same<TYPE, double>::value)
              if(!WSPutFunction(WSlink, "N", 1))
              {
                math_error("Error sending packet through WSTP");
                return ;
              }

            /* Send the variable symbol */
            if(!WSPutSymbol(WSlink, symbol.c_str()))
            {
              math_error("Error sending packet through WSTP");
              return ;
            }

            /* Mark the end of the message */
            if(!WSEndPacket(WSlink))
            {
              math_error("Error sending packet through WSTP");
              return ;
            }

            /* Wait to receive a packet from the kernel */
            int pkt;
            while( (pkt = WSNextPacket(WSlink), pkt) && pkt != RETURNPKT)
            {
              WSNewPacket(WSlink);
              if (WSError(WSlink))
              {
                math_error("Error reading packet from WSTP");
                return ;
              }
            }

            /* Read the received packet into the return value, unless it's void */
            if(!boost::is_same<TYPE, void>::value)
            {
              if(!boost::is_same<TYPE, int>::value and
                 !boost::is_same<TYPE, float>::value and
                 !boost::is_same<TYPE, double>::value and
                 !boost::is_same<TYPE, bool>::value and
                 !boost::is_same<TYPE, char>::value and
                 !boost::is_same<TYPE, str>::value)
                backend_warning().raise(LOCAL_INFO, "Error, WSTP type nor recognised");
              else if(!WSGetVariable(WSlink, &_var))
                math_error("Error reading packet from WSTP");

            }
          }
          catch (std::exception& e) { ini_catch(e); }

        }


        // Assignment operator with TYPE
        mathematica_variable& operator=(const TYPE& val)
        {
          try
          {
            /* Send the expression SYMBOL = val */
            if(!WSPutFunction(_WSlink, "Set", 2) or
               !WSPutSymbol(_WSlink, _symbol.c_str()))
            {
              math_error("Error sending packet through WSTP");
              return *this;
            }

            if(!boost::is_same<TYPE, void>::value)
            {
              if(!boost::is_same<TYPE, int>::value and
                 !boost::is_same<TYPE, float>::value and
                 !boost::is_same<TYPE, double>::value and
                 !boost::is_same<TYPE, bool>::value and
                 !boost::is_same<TYPE, char>::value and
                 !boost::is_same<TYPE, str>::value)
                backend_warning().raise(LOCAL_INFO, "Error, WSTP type nor recognised");
              else if(!WSPutVariable(_WSlink, val))
              {
                math_error("Error reading packet from WSTP");
                return *this;
              }

            }

            /* Mark the end of the message */
            if(!WSEndPacket(_WSlink))
            {
              math_error("Error sending packet through WSTP");
              return *this;
            }

            /* Wait to receive a packet from the kernel */
            int pkt;
            while( (pkt = WSNextPacket(_WSlink), pkt) && pkt != RETURNPKT)
            {
              WSNewPacket(_WSlink);
              if (WSError(_WSlink))
              {
                math_error("Error reading packet from WSTP");
                return *this;
              }
            }

            /* Read the received packet into the return value, unless it's void */
            if(!boost::is_same<TYPE, void>::value)
            {
              if(!boost::is_same<TYPE, int>::value and
                 !boost::is_same<TYPE, float>::value and
                 !boost::is_same<TYPE, double>::value and
                 !boost::is_same<TYPE, bool>::value and
                 !boost::is_same<TYPE, char>::value and
                 !boost::is_same<TYPE, str>::value)
                backend_warning().raise(LOCAL_INFO, "Error, WSTP type nor recognised");
              else if(!WSGetVariable(_WSlink, &_var))
              {
                math_error("Error reading packet from WSTP");
                return *this;
              }

            }

            _var = val;

          }

          catch (std::exception &e) { ini_catch(e); }
      
          return *this;

        }

        // Handle errors
        void math_error(str error)
        {
          backend_warning().raise(LOCAL_INFO, error);
          backend_warning().raise(LOCAL_INFO, WSErrorMessage(_WSlink));
          WSClearError(_WSlink);
          WSNewPacket(_WSlink);
        }


        // Cast operator for type TYPE
        operator TYPE const() 
        {
        
          try
          {
            /* If TYPE is a numeric type, send N first */
            if(boost::is_same<TYPE, int>::value or
               boost::is_same<TYPE, float>::value or
               boost::is_same<TYPE, double>::value)
              if(!WSPutFunction(_WSlink, "N", 1))
              {
                math_error("Error sending packet through WSTP");
                return _var;
              }

            /* Send the variable symbol */
            if(!WSPutSymbol(_WSlink, _symbol.c_str()))
            {
              math_error("Error sending packet through WSTP");
              return _var;
            }

            /* Mark the end of the message */
            if(!WSEndPacket(_WSlink))
            {
              math_error("Error sending packet through WSTP");
              return _var;
            }

            /* Wait to receive a packet from the kernel */
            int pkt;
            while( (pkt = WSNextPacket(_WSlink), pkt) && pkt != RETURNPKT)
            {
              WSNewPacket(_WSlink);
              if (WSError(_WSlink))
              {
                math_error("Error reading packet from WSTP");
                return _var;
              }
            }

            /* Read the received packet into the return value, unless it's void */
            if(!boost::is_same<TYPE, void>::value)
            {
              if(!boost::is_same<TYPE, int>::value and
                 !boost::is_same<TYPE, float>::value and
                 !boost::is_same<TYPE, double>::value and
                 !boost::is_same<TYPE, bool>::value and
                 !boost::is_same<TYPE, char>::value and
                 !boost::is_same<TYPE, str>::value)
                backend_warning().raise(LOCAL_INFO, "Error, WSTP type nor recognised");
              else if(!WSGetVariable(_WSlink, &_var))
                math_error("Error reading packet from WSTP");

            }
        
          }

          catch (std::exception &e) { ini_catch(e); }

          return _var; 
        }
    };


 }
}
#endif /* __mathematica_variable_hpp__ */

#endif /* HAVE_MATHEMATICA */
