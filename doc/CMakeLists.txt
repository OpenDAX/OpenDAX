#  Copyright (c) 2024 Phil Birkelbach
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

set (ADOC ${ASCIIDOCTOR_EXE})


add_custom_command (
  OUTPUT ${PROJECT_BINARY_DIR}/doc/manual.html
  DEPDENDS ${PROJECT_SOURCE_DIR}/doc/manual/manual.asciidoc
  COMMAND ${ADOC} -o ${PROJECT_SOURCE_DIR}/doc/build/manual.html ${PROJECT_SOURCE_DIR}/doc/manual/manual.asciidoc
)

add_custom_target(manual ALL DEPENDS manual.html)

add_custom_command (
  OUTPUT ${PROJECT_BINARY_DIR}/doc/developer.html
  DEPDENDS ${PROJECT_SOURCE_DIR}/doc/developer/developer.asciidoc
  COMMAND ${ADOC} -o ${PROJECT_SOURCE_DIR}/doc/build/developer.html ${PROJECT_SOURCE_DIR}/doc/developer/developer.asciidoc
)

add_custom_target(developer ALL DEPENDS developer.html)