// t. schneider tes@mit.edu 08.12.10
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
//
// see the readme file within this directory for information
// pertaining to usage and purpose of this script.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

#ifndef iLineBasedCommsH
#define iLineBasedCommsH

#include <string>

#include "goby/version.h"
#if GOBY_VERSION_MAJOR >= 2 
 #include "goby/moos/goby_moos_app.h"
#else
 #include "goby/moos/libmoos_util/tes_moos_app.h"
#endif

#include "goby/util/linebasedcomms.h"

#include "iLineBasedComms_config.pb.h"

class CiLineBasedComms :
#if GOBY_VERSION_MAJOR < 2
   public TesMoosApp {
    typedef TesMoosApp GobyMOOSApp;
#else
   public GobyMOOSApp  {       
#endif
  public:
    static CiLineBasedComms* get_instance();
    
  private:
    CiLineBasedComms();
    ~CiLineBasedComms() { }
    
    void loop();
    void handle_out(const CMOOSMsg& msg);
    
    goby::util::LineBasedInterface* modem_;

    static iLineBasedCommsConfig cfg_;
    static CiLineBasedComms* inst_;
    
};

#endif 
