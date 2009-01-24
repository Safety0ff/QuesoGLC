#!/usr/bin/python

# QuesoGLC
# A free implementation of the OpenGL Character Renderer (GLC)
# Copyright (c) 2002, 2004-2009, Bertrand Coconnier
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# $Id$

import urllib, string, sqlite3

print "Open URL..."
unicodata = urllib.urlopen("http://www.unicode.org/Public/UNIDATA/UnicodeData.txt")
print "Read data from URL..."
lignes = unicodata.readlines()
print "Close URL..."
unicodata.close()

print "Open SQLite DB..."
connection = sqlite3.connect('quesoglc.db')
db = connection.cursor()
db.execute('''create table unicode (rank INTEGER PRIMARY KEY, code INTEGER, name TEXT)''')

print "Write data into SQLite DB..."
for s in lignes:
    liste = string.split(s, ';')
    code = eval('0x'+liste[0])
    name = liste[1]
    if name == '<control>':
        continue
    db.execute('''insert into unicode (code,name) values (?,?)''',(code,name))
    connection.commit()

db.close()

print "Success !!!"
