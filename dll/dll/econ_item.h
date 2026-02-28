/* Copyright (C) 2019 Mr Goldberg
   This file is part of the Goldberg Emulator

   The Goldberg Emulator is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   The Goldberg Emulator is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the Goldberg Emulator; if not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef __INCLUDED_ECON_ITEM_H__
#define __INCLUDED_ECON_ITEM_H__

struct Econ_Item_Attribute
{
    uint32 def;
    float value;
};

struct Econ_Item
{
    uint64 id;
    uint32 def;
    uint32 level;
    EItemQuality quality;
    uint32 inv_pos;
    uint32 quantity;
    std::vector<Econ_Item_Attribute> attributes;
};

#endif
