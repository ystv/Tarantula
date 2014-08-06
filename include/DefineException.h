#ifndef DEFINE_EXCEPTION_H
#define DEFINE_EXCEPTION_H
#include <exception>
// DEFINE_EXCEPTION macro defines exceptions derived from the
// standard exception class.
//
// Copyright (c) 2006 by Curtis Krauskopf
//                       >>http://www.decompile.com
//                                                               |
// Permission to use, copy, modify, distribute and sell this 
//     software for any purpose is hereby granted without fee, 
//     provided that the above copyright notice appears in all 
//     copies of this code and in all modified versions of this 
//     code.
// The author makes no representations about the suitability of 
//     this software for any purpose. It is provided "as is" 
//     without express or implied warranty.
//
// Created:  July 20, 2006 by Curtis Krauskopf (cdk)
// Update: July 24, 2006: consolidate MSVC solution
//     with Borland C++ Builder and GNU solution.
//
// 
// For the declaration:  
//   DEFINE_EXCEPTION(FileNotFoundError, "File not found");
//
// the macro expands to:
//
//  class FileNotFoundError : public ExceptionHelper
//  {
//  public:
//      FileNotFoundError(const char *msg = "File not found")
//          : ExceptionHelper(msg)
//      { }
//  };
//
// In the parameter list:
//    ClassName is the exception being defined.  It should not 
//    have been previously defined in the current namespace.
//    Message is the message that will be returned by calling 
//    the .what() method.
//    Message can be "".
//    The exception's type is the ClassName.
//    
//    Class heirarchy:
//
//          std::exception
//                ^
//                |
//          ExceptionHelper
//                ^
//                |
//          ClassName (class being defined)
//          


// ExceptionHelper augments the standard exception class by
// allowing a const char * parameter in the constructor.
class ExceptionHelper : public std::exception
{
public:
  ExceptionHelper(const char *msg)
    : std::exception(), msg_(msg)
    { }
  virtual const char * what() const throw() { return msg_; }
private:
  const char *msg_;
};

#define DEFINE_EXCEPTION(ClassName,Message) \
    class ClassName : public ExceptionHelper \
    { \
    public: \
        ClassName(const char *msg = Message) \
            : ExceptionHelper(msg) \
        { } \
    };

#endif
