/*
  DSM2_tx implements the serial communication protocol used for operating
  the RF modules that can be found in many DSM2-compatible transmitters.
  Copyrigt (C) 2012  Erik Elmore <erik@ironsavior.net>
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <stdarg.h>
#include <iostream>

class FakeSerial {
 public:
  void begin(unsigned long speed);
  void end();
  size_t write(const unsigned char* buf, size_t size);
  void print(const char* buf);
  void println();
  void println(const char* buf);

  // template <typename... Args>
  // int printf(const char* fmt, Args&&... args) {
  //   return std::printf(fmt, std::forward<Args>(args)...);
  // }

  template <typename... Args>
  int printf(const char* fmt, Args... args) {
    return std::printf(fmt, std::forward<Args>(args)...);
  }
};

extern FakeSerial Serial;
