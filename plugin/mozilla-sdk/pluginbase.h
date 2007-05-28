// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifndef __PLUGININSTANCEBASE_H__
#define __PLUGININSTANCEBASE_H__

#include "npplat.h"
#include "tu_config.h"

struct nsPluginCreateData
{
  NPP instance;
  NPMIMEType type; 
  uint16 mode; 
  int16 argc; 
  char** argn; 
  char** argv; 
  NPSavedData* saved;
};

class DSOLOCAL nsPluginInstanceBase
{
 public:
  virtual ~nsPluginInstanceBase() { return; }
 
  // these three methods must be implemented in the derived
  // class platform specific way
  virtual NPBool init(NPWindow* aWindow) = 0;
  virtual void shut() = 0;
  virtual NPBool isInitialized() = 0;

  // implement all or part of those methods in the derived 
  // class as needed
  virtual NPError SetWindow(NPWindow* /*pNPWindow*/)                    { return NPERR_NO_ERROR; }
  virtual NPError WriteStatus(char* /*msg*/) const                     { return NPERR_NO_ERROR; }
  virtual NPError NewStream(NPMIMEType /*type*/, NPStream* /*stream*/, 
                            NPBool /*seekable*/, uint16* /*stype*/)         { return NPERR_NO_ERROR; }
  virtual NPError DestroyStream(NPStream* /*stream*/, NPError /*reason*/)   { return NPERR_NO_ERROR; }
  virtual void    StreamAsFile(NPStream* /*stream*/, const char* /*fname*/) { return; }
  virtual int32   WriteReady(NPStream* /*stream*/)                      { return 0x0fffffff; }
  virtual int32   Write(NPStream* /*stream*/, int32 /*offset*/, 
                        int32 len, void* /*buffer*/)                    { return len; }
  virtual void    Print(NPPrint* /*printInfo*/)                         { return; }
  virtual uint16  HandleEvent(void* /*event*/)                          { return 0; }
  virtual void    URLNotify(const char* /*url*/, NPReason /*reason*/, 
                            void* /*notifyData*/)                       { return; }
  virtual NPError GetValue(NPPVariable /*variable*/, void* /*value*/)       { return NPERR_NO_ERROR; }
  virtual NPError SetValue(NPNVariable /*variable*/, void* /*value*/)       { return NPERR_NO_ERROR; }
};

// functions that should be implemented for each specific plugin

// creation and destruction of the object of the derived class
DSOEXPORT nsPluginInstanceBase * NS_NewPluginInstance(nsPluginCreateData * aCreateDataStruct);
DSOEXPORT void NS_DestroyPluginInstance(nsPluginInstanceBase * aPlugin);

// global plugin initialization and shutdown
DSOEXPORT NPError NS_PluginInitialize();
DSOEXPORT void NS_PluginShutdown();

#ifdef XP_UNIX
// global to get plugins name & description 
DSOEXPORT NPError NS_PluginGetValue(NPPVariable aVariable, void *aValue);
#endif

#endif // __PLUGININSTANCEBASE_H__
