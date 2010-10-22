// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GLUE_PLUGINS_PEPPER_PLUGIN_MODULE_H_
#define WEBKIT_GLUE_PLUGINS_PEPPER_PLUGIN_MODULE_H_

#include <map>
#include <set>

#include "base/basictypes.h"
#include "base/native_library.h"
#include "base/ref_counted.h"
#include "base/weak_ptr.h"
#include "third_party/ppapi/c/pp_module.h"
#include "third_party/ppapi/c/ppb.h"

class FilePath;
typedef struct NPObject NPObject;
struct PPB_Core;
typedef void* NPIdentifier;

namespace pepper {

class ObjectVar;
class PluginDelegate;
class PluginInstance;
class PluginObject;

class PluginModule : public base::RefCounted<PluginModule>,
                     public base::SupportsWeakPtr<PluginModule> {
 public:
  typedef const void* (*PPP_GetInterfaceFunc)(const char*);
  typedef int (*PPP_InitializeModuleFunc)(PP_Module, PPB_GetInterface);
  typedef void (*PPP_ShutdownModuleFunc)();

  struct EntryPoints {
    EntryPoints()
        : get_interface(NULL),
          initialize_module(NULL),
          shutdown_module(NULL) {
    }

    PPP_GetInterfaceFunc get_interface;
    PPP_InitializeModuleFunc initialize_module;
    PPP_ShutdownModuleFunc shutdown_module;
  };

  ~PluginModule();

  static scoped_refptr<PluginModule> CreateModule(const FilePath& path);
  static scoped_refptr<PluginModule> CreateInternalModule(
      EntryPoints entry_points);

  // Converts the given module ID to an actual module object. Will return NULL
  // if the module is invalid.
  static PluginModule* FromPPModule(PP_Module module);

  static const PPB_Core* GetCore();

  PP_Module GetPPModule() const;

  PluginInstance* CreateInstance(PluginDelegate* delegate);

  // Returns "some" plugin instance associated with this module. This is not
  // guaranteed to be any one in particular. This is normally used to execute
  // callbacks up to the browser layer that are not inherently per-instance,
  // but the delegate lives only on the plugin instance so we need one of them.
  PluginInstance* GetSomeInstance() const;

  const void* GetPluginInterface(const char* name) const;

  // This module is associated with a set of instances. The PluginInstance
  // object declares its association with this module in its destructor and
  // releases us in its destructor.
  void InstanceCreated(PluginInstance* instance);
  void InstanceDeleted(PluginInstance* instance);

  // Tracks all live ObjectVar. This is so we can map between PluginModule +
  // NPObject and get the ObjectVar corresponding to it. This Add/Remove
  // function should be called by the ObjectVar when it is created and
  // destroyed.
  void AddNPObjectVar(ObjectVar* object_var);
  void RemoveNPObjectVar(ObjectVar* object_var);

  // Looks up a previously registered ObjectVar for the given NPObject and
  // module. Returns NULL if there is no ObjectVar corresponding to the given
  // NPObject for the given module. See AddNPObjectVar above.
  ObjectVar* ObjectVarForNPObject(NPObject* np_object) const;

  // Tracks all live PluginObjects.
  void AddPluginObject(PluginObject* plugin_object);
  void RemovePluginObject(PluginObject* plugin_object);

 private:
  PluginModule();

  bool InitFromEntryPoints(const EntryPoints& entry_points);
  bool InitFromFile(const FilePath& path);
  static bool LoadEntryPoints(const base::NativeLibrary& library,
                              EntryPoints* entry_points);

  bool initialized_;

  // Holds a reference to the base::NativeLibrary handle if this PluginModule
  // instance wraps functions loaded from a library.  Can be NULL.  If
  // |library_| is non-NULL, PluginModule will attempt to unload the library
  // during destruction.
  base::NativeLibrary library_;

  // Contains pointers to the entry points of the actual plugin
  // implementation.
  EntryPoints entry_points_;

  // Non-owning pointers to all instances associated with this module. When
  // there are no more instances, this object should be deleted.
  typedef std::set<PluginInstance*> PluginInstanceSet;
  PluginInstanceSet instances_;

  // Tracks all live ObjectVars used by this module so we can map NPObjects to
  // the corresponding object. These are non-owning references.
  typedef std::map<NPObject*, ObjectVar*> NPObjectToObjectVarMap;;
  NPObjectToObjectVarMap np_object_to_object_var_;

  typedef std::set<PluginObject*> PluginObjectSet;
  PluginObjectSet live_plugin_objects_;

  DISALLOW_COPY_AND_ASSIGN(PluginModule);
};

}  // namespace pepper

#endif  // WEBKIT_GLUE_PLUGINS_PEPPER_PLUGIN_MODULE_H_