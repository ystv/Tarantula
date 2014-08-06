/******************************************************************************
*   Copyright (C) 2011 - 2014  York Student Television
*
*   Tarantula is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   Tarantula is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with Tarantula.  If not, see <http://www.gnu.org/licenses/>.
*
*   Contact     : tarantula@ystv.co.uk
*
*   File Name   : CustomExceptions.h
*   Version     : 1.0
*   Description : Defines some common exceptions
*
*****************************************************************************/


#pragma once

#include "DefineException.h"

DEFINE_EXCEPTION(FileNotFoundError, "The file could not be found")
DEFINE_EXCEPTION(InternalParserError, "An internal parser error occurred")
DEFINE_EXCEPTION(UnknownError, "An unknown error occurred")
