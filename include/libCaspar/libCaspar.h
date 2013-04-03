/******************************************************************************
*   Copyright (C) 2011 - 2013  York Student Television
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
*   File Name   : libCaspar.h
*   Version     : 1.0
*   Description : libCaspar.h - libCaspar is a communications library to make
*                 talking to Caspar in C/C++ a little easier.
*
*****************************************************************************/


// Protocol definition
#ifndef AMCP_20
#define AMCP_18
#endif

#include <libCaspar/CasparCommand.h>
#include <libCaspar/CasparVideoCommand.h>
#include <libCaspar/CasparFlashCommand.h>
#include <libCaspar/CasparQueryResponseProcessor.h>
#include <libCaspar/CasparConnection.h>

